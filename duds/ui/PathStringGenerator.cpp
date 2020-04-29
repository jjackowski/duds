/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/PathStringGenerator.hpp>
#include <duds/general/Errors.hpp>
#include <assert.h>

namespace duds { namespace ui {

PathStringGenerator::PathStringGenerator(
	const std::string &separator,
	const std::string &ellips,
	unsigned int mlen,
	unsigned int mtitle
) : sep(separator), maxLen(mlen), maxPageLen(mtitle) {
	ellipsis(ellips);
}

void PathStringGenerator::ellipsis(const std::string &e) {
	// must be shorter than maxPageLen
	if (e.size() >= maxPageLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxTitleLength(maxPageLen) << StringLength(e.size())
		);
	}
	ellip = e;
}

void PathStringGenerator::currentHeader(const std::string &h) {
	// must be shorter than maxLen
	std::string::size_type len = h.size() + ellip.size() + postCur.size();
	if (len >= maxLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxLength(maxLen) << StringLength(len)
		);
	}
	preCur = h;
}

void PathStringGenerator::currentFooter(const std::string &f) {
	// must be shorter than maxLen
	std::string::size_type len = f.size() + ellip.size() + preCur.size();
	if (len >= maxLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxLength(maxLen) << StringLength(len)
		);
	}
	postCur = f;
}

void PathStringGenerator::maxLength(unsigned int max) {
	// must be greater than (length of sep) * maxPages + preCur + postCur
	if (max != -1) {
		std::string::size_type len = preCur.size() + postCur.size() + ellip.size();
		if (maxPages != -1) {
			len += sep.size() * maxPages;
		}
		if ((max <= len) || ((maxPageLen != -1) && (max <= maxPageLen))) {
			DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
				PathMaxLength(max) << PathMaxTitleLength(maxPageLen) <<
				StringLength(len)
			);
		}
	}
	maxLen = max;
}

void PathStringGenerator::maxTitleLength(unsigned int max) {
	// must be greater than length of ellip
	if (max <= ellip.size()) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxTitleLength(max) << StringLength(ellip.size())
		);
	} else if (max > maxLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxLength(maxLen) << PathMaxTitleLength(max)
		);
	} else if (max < minPageLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMinTitleLength(minPageLen) << PathMaxTitleLength(max)
		);
	}
	maxPageLen = max;
}

void PathStringGenerator::minTitleLength(unsigned int min) {
	if (min >= maxLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMaxLength(maxLen) << PathMinTitleLength(min)
		);
	} else if (min > maxPageLen) {
		DUDS_THROW_EXCEPTION(PathStringGeneratorParameterError() <<
			PathMinTitleLength(min) << PathMaxTitleLength(maxPageLen)
		);
	}
	minPageLen = min;
}

std::string::size_type PathStringGenerator::titleLength(
	const std::string &title,
	int &total,
	unsigned int max
) const {
	if (title.size() > max) {
		if (!abruptSplit) {
			// search for a space
			std::string::size_type space =
				title.find_last_of(' ', max - ellip.size());
			// check for finding space & not too short
			if ((space != std::string::npos) && (space >= minPageLen)) {
				// total increases up to space plus ellipsis
				total += space + ellip.size();
				// length of title does not include ellipsis
				return space;
			}
		}
		// avoid having a space as the last character
		while ((max > ellip.size()) && (title[max - ellip.size() - 1] == ' ')) {
			--max;
		}
		// total increases by the maximum
		total += max;
		// length of title does not include ellipsis
		return max - ellip.size();
	}
	// include whole title
	total += title.size();
	return title.size();
}

void PathStringGenerator::decTitleLen(const TrucatedTitle &tt, int &total) const {
	total -= tt.len + sep.size();
	if (tt.len < tt.title->size()) {
		total -= ellip.size();
	}
}

int PathStringGenerator::averageLength(
	int numtitles,
	int totalLen,
	int currLen
) {
	return (totalLen - currLen) / (numtitles - 1) +
		(((totalLen - currLen) % (numtitles - 1) > 0) ? 1 : 0);
}

std::string PathStringGenerator::generate(const Path &path) const {
	// avoid throwing exception if there is no path
	if (path.empty()) {
		return std::string();
	}
	// storage for page titles; may need to be altered prior to generating the
	// final string
	std::vector<TrucatedTitle> titles;
	// first page to show; end iterator
	Path::PageStack::const_reverse_iterator eiter = path.rend();
	// current page
	Path::PageStack::const_reverse_iterator citer = path.rcurrent();
	// start iterator
	Path::PageStack::const_reverse_iterator iter = citer;
	// should a page past the current be shown, and does such a page exist?
	int haveFwd;
	if (showFwd && (maxPages > 2) && (iter != path.rbegin())) {
		// start one more page past the current page
		--iter;
		haveFwd = 1;
	} else {
		haveFwd = 0;
	}
	// find the number of pages to show
	int pcount = eiter - iter;
	// too many pages?
	if (pcount > maxPages) {
		// advance start so maximum pages includes current page
		eiter -= pcount - maxPages;
		pcount = maxPages;
	}
	// first pass at making title strings
	int tlen = -(int)sep.size();
	// current page title length
	int clen = preCur.size() + postCur.size();
	titles.reserve(pcount);
	// Visit pages in reverse order to tabulate titles.
	// Stop when the total length exceeds about twice the maximum. After this
	// loop, the results will be shortened to fit.
	for (; (iter != eiter) && ((tlen < 0) || ((tlen >> 1) < maxLen)); ++iter) {
		int &len = (iter == citer) ? clen : tlen;
		// truncate titles to maxium length for single title
		if (!wholeCurrent || (iter != citer)) {
			titles.emplace_back(TrucatedTitle(
				(*iter)->title(),
				titleLength((*iter)->title(), len, maxPageLen)
			));
		}
		// truncate current page title to maximum path string length
		else {
			titles.emplace_back(TrucatedTitle(
				(*iter)->title(),
				titleLength(
					(*iter)->title(),
					clen,
					maxLen - preCur.size() - postCur.size()
				)
			));
		}
		// add current item size to total
		if (iter == citer) {
			tlen += clen;
		}
		// add seperator length
		tlen += sep.size();
	}
	// far too long? only the current page title will fit?
	if (clen > (maxLen - minPageLen - sep.size())) {
		// remove all titles before current
		titles.resize(haveFwd + 1);
		// title after current?
		if (haveFwd) {
			// remove it, too
			titles.erase(titles.begin());
		}
		assert(titles.size() == 1);
		// reset the length counters
		tlen = clen = titles.front().len + preCur.size() + postCur.size();
		assert((unsigned int)tlen <= maxLen);
	}
	// too long?
	else if ((unsigned int)tlen > maxLen) {
		// reduce number of titles until few are left, or average length is above
		// the requested minimum length or the string will fit
		int avgLen = averageLength(titles.size(), tlen, clen);
		while (
			// fit check; needed to avoid average check removing extra titles
			// when some titles are shorter than the minimum
			((unsigned int)tlen > maxLen) &&
			// Average check & remaining titles check.
			// The remaining titles check avoids removing the current page title.
			(avgLen >= minPageLen) && (titles.size() > (2 + haveFwd))
		) {
			decTitleLen(titles.back(), tlen);
			titles.pop_back();
			avgLen = averageLength(titles.size(), tlen, clen);
		}
		// still need some more length and there is a title in the forward direction?
		if (haveFwd && (titles.size() == 3) && ((unsigned int)tlen > maxLen)) {
			// remove forward title
			haveFwd = 0;
			decTitleLen(titles.front(), tlen);
			titles.erase(titles.begin());
		}
	}
	// still too long?
	int overacc = 0;  // over length accumulator
	while ((unsigned int)tlen > maxLen) {
		// compute a projected maximum length based on available average, and
		// shorten it by overacc and the a
		int pmax = averageLength(titles.size(), maxLen, clen) - overacc;
		int poveracc = overacc;
		assert(pmax >= minPageLen);
		// visit stored titles in reverse (forward order when output)
		std::vector<TrucatedTitle>::reverse_iterator titer = titles.rbegin();
		std::vector<TrucatedTitle>::reverse_iterator etiter = titles.rend();
		std::vector<TrucatedTitle>::reverse_iterator ctiter =
			titles.rend() - (1 + haveFwd);
		for (; titer != etiter; ++titer) {
			// skip current page title
			if (titer == ctiter) continue;
			// if title, including ellipsis, is more than pmax long . . .
			std::string::size_type len = titer->len;
			if (len != titer->title->size()) {
				len += ellip.size();
			}
			if (len > pmax) {
				// attempt to shorten
				// remove title from total length
				decTitleLen(*titer, tlen);
				int plen = titer->len;
				titer->len = titleLength(*titer->title, tlen, pmax);
				tlen += sep.size();
				// no change or (no space and not the last item other than the current)?
				if ((plen == titer->len) || // no change, or . . .
					// length below minimum, or . . .
					(titer->len < minPageLen) || (
						// not using abrupt splits; favor splits at spaces
						!abruptSplit &&
						// more than two pages, or length below minimum
						(titles.size() > 2) &&
						// did not shorten to a space
						((*titer->title)[titer->len] != ' ')
					)
				) {
					// if there is a title in the forward direction and the current
					// title is either the last or first in the titles vector . . .
					if (haveFwd && (
						(titer == titles.rbegin()) ||
						(titer == (etiter - 1))
					)) {
						// Remove the last (forward) title from the path.
						// The vector is in reverse order.
						haveFwd = 0;
						decTitleLen(titles.front(), tlen);
						titles.erase(titles.begin());
						overacc = 0;
						// leave for loop; go back to while loop
						break;
					}
					// if this is the first item of the path . . .
					else if (titer == titles.rbegin()) {
						// remove the first title from the path
						decTitleLen(titles.back(), tlen);
						titles.pop_back();
						overacc = 0;
						// leave for loop; go back to while loop
						break;
					} else {
						// WARNING: no test runs this code block; can it be removed?
						// leave title as-is
						if (plen != titer->len) {
							decTitleLen(*titer, tlen);
							// revert lengths
							titer->len = plen;
							tlen += plen + sep.size();
							if (titer->len < titer->title->size()) {
								tlen += ellip.size();
							}
						}
						// alter over length accumulator to account for the length
						// later
						overacc += len - pmax;
						// this was a problem in a test case, but solution prevents
						// this code from running in that test case, so not sure
						// problem is solved
						assert(((int)len - pmax) > 0);
					}
				}
			}
		}
		// if still too long, but no attempt to shorten the projected maximum
		if ((overacc == poveracc) && ((unsigned int)tlen > maxLen)) {
			// shorten the maximum
			overacc += std::max(
				//(tlen - maxLen) / (unsigned int)titles.size(),
				//(unsigned int)1
				averageLength(titles.size(), tlen, clen) * (int)titles.size() - (int)maxLen,
				1
			);
		}
	}
	assert((unsigned int)tlen <= maxLen);
	// build path string
	std::string res;
	res.reserve(tlen);
	std::vector<TrucatedTitle>::reverse_iterator titer = titles.rbegin();
	std::vector<TrucatedTitle>::reverse_iterator etiter = titles.rend();
	std::vector<TrucatedTitle>::reverse_iterator ctiter =
			titles.rend() - (1 + haveFwd);
	for (; titer != etiter; ++titer) {
		// at current page title?
		if (titer == ctiter) {
			// add header
			res += preCur;
		}
		// put in title
		res.insert(res.size(), *titer->title, 0, titer->len);
		// not whole title?
		if (titer->title->size() != titer->len) {
			// add ellipsis
			res += ellip;
		}
		// at current page title?
		if (titer == ctiter) {
			// add footer
			res += postCur;
		}
		// not at last title?
		if (titer != (etiter - 1)) {
			// add separator
			res += sep;
		}
	}
	assert(res.size() == tlen);
	return res;
}

} }
