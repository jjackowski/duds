/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <exception>
#include <duds/ui/Path.hpp>

namespace duds { namespace ui {

/**
 * Error that signifies a PathStringGenerator object was given a parameter
 * value that conflicts with another parameter value and would prevent the
 * algorithm from functioning properly.
 */
struct PathStringGeneratorParameterError :
virtual std::exception, virtual boost::exception { };

/**
 * Error attribute used by PathStringGenerator to denote a string length that
 * is important in the context of the error.
 */
typedef boost::error_info<struct Info_StringLength, std::string::size_type>
	StringLength;

/**
 * Error attribute used by PathStringGenerator to hold the maximum length of
 * its string output.
 */
typedef boost::error_info<struct Info_PathMaxLength, unsigned int>
	PathMaxLength;

/**
 * Error attribute used by PathStringGenerator to hold the maximum length of
 * page titles in its output.
 */
typedef boost::error_info<struct Info_PathMaxTitleLength, unsigned int>
	PathMaxTitleLength;

/**
 * Error attribute used by PathStringGenerator to hold the minimum length of
 * page titles in its output.
 */
typedef boost::error_info<struct Info_PathMinTitleLength, unsigned int>
	PathMinTitleLength;

/**
 * Produces a string showing the path through pages as tracked by a Path
 * object.
 * This ojbect has a number of parameters that affect its output:
 * | Function               | Default value     | Short description
 * |------------------------|-------------------|---------------------------------
 * | separator()            | empty string      | put between page titles
 * | ellipsis()             | empty string      | addrd to end of shortened titles
 * | currentHeader()        | empty string      | precedes current title
 * | currentFooter()        | empty string      | follows current title
 * | maxLength()            | -1 (huge maximum) | maximum output length
 * | maxTitles()            | -1 (huge maximum) | maximum number of titles
 * | maxTitleLength()       | -1 (huge maximum) | maximum length of a title
 * | minTitleLength()       | 4                 | minimum length of a shortened title
 * | showForwardPage()      | true              | allow title in forward direction
 * | showWholeCurrentPage() | false             | allow current title longer than maxTitleLength()
 *
 * The algorithm, invoked by calling generate(), is an attempt to show a few
 * page titles along the path, with priority given first to the current page,
 * then to the page in the back (past) direction, then to the forward page,
 * and finally to pages further in the back direction.
 *
 * The current page title is bracketed with the currentHeader() and
 * currentFooter() strings. These strings do not count against the title's
 * maximum length. The separator() string is placed between each title, and
 * also does not count toward a title's maximum. Any title that is shortened
 * will have the ellipsis() string appended, which does count toward the
 * title's maximum.
 *
 * No more than maxTitles() page titles will be included in the path string.
 * Each page title will not be longer than maxTitleLength(), including the
 * ellipsis() string if shortened. The page titles will also not be shorter
 * than minTitleLength() except when the whole title is shorter. The end
 * result will not be longer than maxLength().
 *
 * The algorithm first gathers all titles that could possibly be included and
 * shortens them to fit within maxTitleLength() characters. It then removes
 * titles until the remainder will more likely be able to fit. This is done
 * considering the average length of the page titles, other than the current
 * title, and removes starting from the farthest in the backwards (past)
 * direction until either the titles may fit or there is only one ahead of the
 * current title. If the titles are still too long, the title in the forward
 * direction, if present, is removed.
 *
 * After this, if the title string will still be too long, the algorithm will
 * attempt to shorten or remove the remaining titles until they fit. Shortening
 * is done with a preference to show whole words by breaking the title string
 * at spaces. There is a stronger preference to keep the title longer than
 * minTitleLength() so that the title will still offer enough context. Should
 * the shortening method not be able to fit at least the minimum length for
 * the title furthest in the back direction, the title in the opposite
 * direction past the current page is removed, or if that page doesn't exist,
 * the title furthest in the back direction is removed.
 *
 * Finally, the output string is produced. During the above steps, no strings
 * are copied or altered. Internal data references the strings and tracks the
 * length of each string that will be included.
 *
 * @note  The code is likely more complicated than required for decent results,
 *        and different results could simplify the algorithm.
 *
 * @author  Jeff Jackowski
 */
class PathStringGenerator {
	/**
	 * Separator between page titles.
	 */
	std::string sep;
	/**
	 * Last character(s) to use when part of a title is not shown.
	 */
	std::string ellip;
	/**
	 * Marker string that preceds the current page title.
	 */
	std::string preCur;
	/**
	 * Marker string that follows the current page title.
	 */
	std::string postCur;
	/**
	 * The maximum length of the output string.
	 */
	unsigned int maxLen = -1;
	/**
	 * The maximum number of titles to show.
	 */
	unsigned int maxPages = -1;
	/**
	 * The maximum length of any single title, with the possibile exception of
	 * the current page. The length includes the ellipsis if shown.
	 */
	unsigned int maxPageLen = -1;
	/**
	 * The minimum length of any title; if fewer characters are availble, fewer
	 * titles will be shown.
	 */
	unsigned int minPageLen = 4;
	/**
	 * True to show one page forward past the current page if such a page
	 * exits.
	 */
	bool showFwd = true;
	/**
	 * True to show the entire title of the current page if it will fit within
	 * @a maxLen, and use more than @a maxTitleLen to fit it.
	 */
	bool wholeCurrent = false;
	/**
	 * Break long titles without considering spaces within the title.
	 */
	bool abruptSplit = false;
	/**
	 * Finds the usable length of the given title and adjusts the total length
	 * of the path string. If the title fits within the given maximum, the
	 * whole string is included. If it is too long and @a abruptSplit is set,
	 * it is as shortened to the maximum length. If @a abruptSplit is clear
	 * (default), then the rightmost space in the title that fits within the
	 * given maximum will be the length if such a space exists and does not make
	 * the title shorter than the minimum. Otherwise it is shortened to the
	 * maximum length.
	 *
	 * In all cases, the length of the title (returned value) does not include
	 * the length of the ellipsis, but the total length adjustment does. If the
	 * title is shortened, its length plus the length of the ellipsis will fit
	 * within the maximum.
	 *
	 * This function ignores the length of the separator and the pre & post
	 * current page markers.
	 *
	 * @param title  The title string to consider.
	 * @param total  The total path length. This will be incremented to include
	 *               the title length. If the whole title does not fit, it will
	 *               also be incremented by the ellipsis length.
	 * @param max    The maximum length of the title plus ellipsis.
	 * @return       The length of the title to use.
	 */
	std::string::size_type titleLength(
		const std::string &title,
		int &total,
		unsigned int max
	) const;
	/**
	 * An internal data structure used to track the titles to include in the
	 * path string and the length of each string to use. It allows the length
	 * of the titles to be adjusted without copying or modifying strings.
	 */
	struct TrucatedTitle {
		/**
		 * The title string.
		 */
		const std::string *title;
		/**
		 * The length of the title. This does not include the ellipsis.
		 */
		std::string::size_type len;
		TrucatedTitle() = default;
		TrucatedTitle(const std::string &t) :
			title(&t),
			len(t.size())
		{ }
		TrucatedTitle(const std::string &t, std::string::size_type l) :
			title(&t),
			len(l)
		{ }
	};
	/**
	 * Removes the given title from the total path length, along with the length
	 * of the separator and ellipsis if used.
	 * @param tt     The title record.
	 * @param total  The total path length.
	 */
	void decTitleLen(const TrucatedTitle &tt, int &total) const;
	/**
	 * Computes the average length of the titles, excluding the current page
	 * title, and rounds up.
	 * @param numtitles  Total number of titles being considered.
	 * @param totalLen   The total path string length.
	 * @param currLen    The current page title length.
	 * @return           The average length of the page titles other than the
	 *                   current page.
	 */
	static int averageLength(int numtitles, int totalLen, int currLen);
public:
	/**
	 * Makes a PathStringGenerator with default values.
	 */
	PathStringGenerator() = default;
	/**
	 * Makes a PathStringGenerator with the given title separator and maximum
	 * path length.
	 * @param separator  The string used in between each title.
	 * @param mlen       The maximum length of the generated path string.
	 */
	PathStringGenerator(
		const std::string &separator,
		unsigned int mlen = -1
	) : sep(separator), maxLen(mlen) { }
	/**
	 * Makes a PathStringGenerator with the given title separator, ellipsis,
	 * and maximum path length.
	 * @param separator  The string used in between each title.
	 * @param ellipsis   The string put at the end of any title that is
	 *                   shortened to fit in the path string.
	 * @param mlen       The maximum length of the generated path string.
	 * @param mtitle     The maximum length of a single title.
	 * @throw PathStringGeneratorParameterError  The ellipsis is as long as or
	 *                                           longer than the maximum title
	 *                                           length.
	 */
	PathStringGenerator(
		const std::string &separator,
		const std::string &ellipsis,
		unsigned int mlen = -1,
		unsigned int mtitle = -1
	);
	/**
	 * Returns the string used to separate the titles.
	 */
	const std::string &separator() const {
		return sep;
	}
	/**
	 * Changes the string used to separate the titles.
	 */
	void separator(const std::string &s) {
		sep = s;
	}
	/**
	 * Returns the string appended to the end of page titles that are shortened
	 * to fit.
	 */
	const std::string &ellipsis() const {
		return ellip;
	}
	/**
	 * Changes the string appended to the end of page titles that are shortened
	 * to fit.
	 * @param e  The new ellipsis.
	 * @throw PathStringGeneratorParameterError  The ellipsis is as long as or
	 *                                           longer than the maximum title
	 *                                           length.
	 */
	void ellipsis(const std::string &e);
	/**
	 * Returns the string prepended to the current page title.
	 */
	const std::string &currentHeader() const {
		return preCur;
	}
	/**
	 * Changes the string prepended to the current page title. Its length is not
	 * counted as part of the title's length.
	 * @param h  The new header.
	 * @throw PathStringGeneratorParameterError  The header and footer are as
	 *                                           long as or longer than the
	 *                                           maximum path string length.
	 */
	void currentHeader(const std::string &h);
	/**
	 * Returns the string appended to the end of the current page title.
	 */
	const std::string &currentFooter() const {
		return postCur;
	}
	/**
	 * Changes the string appended to the end of the current page title. Its
	 * length is not counted as part of the title's length.
	 * @param f  The new footer.
	 * @throw PathStringGeneratorParameterError  The header and footer are as
	 *                                           long as or longer than the
	 *                                           maximum path string length.
	 */
	void currentFooter(const std::string &f);
	/**
	 * Returns the maximum length of the generated path strings.
	 */
	unsigned int maxLength() const {
		return maxLen;
	}
	/**
	 * Changes the maximum total length of the generated path strings.
	 * @param max  The new maximum length.
	 * @throw PathStringGeneratorParameterError  The new maximum is too small to
	 *                                           accommodate the current header,
	 *                                           footer, and separator strings.
	 */
	void maxLength(unsigned int max);
	/**
	 * Returns the maximum number of page titles that may be included in the
	 * output path string.
	 */
	unsigned int maxTitles() const {
		return maxPages;
	}
	/**
	 * Changes the maximum number of page titles that may be included in the
	 * output path string.
	 * @param max  The new maximum number of page titles.
	 */
	void maxTitles(unsigned int max) {
		maxPages = max;
	}
	/**
	 * Returns the maximum length allocated to a page title in the output path
	 * string. This length includes the ellipsis string for titles that are
	 * shortened.
	 */
	unsigned int maxTitleLength() const {
		return maxPageLen;
	}
	/**
	 * Changes the maximum length allocated to a page title in the output path
	 * string. This length includes the ellipsis string for titles that are
	 * shortened.
	 * @param max  The new maximum title length.
	 * @throw PathStringGeneratorParameterError  The new maximum is too small to
	 *                                           accommodate the ellipsis
	 *                                           string, or is larger than the
	 *                                           maximum output length, or is
	 *                                           less than the minimum title
	 *                                           length.
	 */
	void maxTitleLength(unsigned int max);
	/**
	 * Returns the minimum length for a shortened page title in the output path
	 * string. Full titles that are shorter will still be included. The length
	 * does not include the ellipsis string for titles that are shortened.
	 */
	unsigned int minTitleLength() const {
		return minPageLen;
	}
	/**
	 * Changes the minimum length for a shortened page title in the output path
	 * string. Full titles that are shorter will still be included. The length
	 * does not include the ellipsis string for titles that are shortened.
	 * @param min  The new minimum shortened title length.
	 * @throw PathStringGeneratorParameterError  The new minimum is larger than
	 *                                           either the maximum output
	 *                                           length or the maximum title
	 *                                           length.
	 */
	void minTitleLength(unsigned int min);
	/**
	 * True if the page in the forward direction may be included in the output
	 * path string.
	 */
	bool showForwardPage() const {
		return showFwd;
	}
	/**
	 * Changes if the page in the forward direction may be included in the output
	 * path string. The inclusion is a lower priority than for pages in the
	 * back direction, and it requires at least 3 page titles to be shown.
	 * @param show  True to allow showing a page title in the forward direction.
	 */
	void showForwardPage(bool show = true) {
		showFwd = show;
	}
	/**
	 * True when the length of the current page title is not bound by the
	 * maximum title length (maxTitleLength()). The length is still bound by
	 * the maximum output length (maxLength()), in which the current page
	 * header and footer strings must also fit. The ellipsis string will also be
	 * included if the title must still be shortened to fit.
	 */
	bool showWholeCurrentPage() const {
		return wholeCurrent;
	}
	/**
	 * Changes if the length of the current page title is bound by the maximum
	 * output length (true) or by the maximum title length (false; default).
	 * If bound by the maximum output length (maxLength()), that length will
	 * also include the current page header and footer strings, and if the title
	 * must be shortened, the ellipsis string as well.
	 * @param show  True to allow the current page title length to be longer
	 *              than the maximum title length.
	 */
	void showWholeCurrentPage(bool show = true) {
		wholeCurrent = show;
	}
	/**
	 * Generates the path string for the given Path object. This object retains
	 * no state information from the Path object. Details on the operation are
	 * in this class's documentation.
	 * @pre  The Path object will not be altered, nor will Page titles be
	 *       changed, by another thread while this function is running.
	 * @param path  The Path object who's path will be represented in the string.
	 * @return      A string representing the path using titles from Page
	 *              objects.
	 */
	std::string generate(const Path &path) const;
};

} }
