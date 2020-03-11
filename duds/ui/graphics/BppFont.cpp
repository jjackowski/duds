/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppFont.hpp>
#include <duds/ui/graphics/BppImageErrors.hpp>
#include <duds/ui/graphics/BppImageArchiveSequence.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

constexpr BppFont::Flags BppFont::FixedWidth;
constexpr BppFont::Flags BppFont::FixedWidthPerLine;
constexpr BppFont::Flags BppFont::VariableHeight;
constexpr BppFont::Flags BppFont::AlignLeft;
constexpr BppFont::Flags BppFont::AlignCenter;
constexpr BppFont::Flags BppFont::AlignRight;
constexpr BppFont::Flags BppFont::AlignMask;

BppImageSptr BppFont::renderGlyph(char32_t gc) {
	DUDS_THROW_EXCEPTION(GlyphNotFoundError() << Character(gc));
}

void BppFont::load(const std::string &path) {
	std::ifstream is(path);
	if (!is.good()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamError() <<
			ImageArchiveFileName(path)
		);
	}
	try {
		load(is);
	} catch (boost::exception &be) {
		// add the file name to the error metadata
		be << ImageArchiveFileName(path);
		throw;
	}
}

void BppFont::load(std::istream &is) {
	BppImageArchiveSequence bias(is);
	bias.readHeader();
	BppImageArchiveSequence::iterator iter = bias.begin();
	for (; iter != bias.end(); ++iter) {
		/** @todo  Make this work for characters larger than a byte. */
		if (iter->first.length() == 1) {
			std::lock_guard<duds::general::Spinlock> lock(block);
			glyphs[iter->first[0]] = iter->second;
		}
	}
}

void BppFont::add(char32_t gc, const BppImageSptr &img) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	glyphs[gc] = img;
}

void BppFont::add(char32_t gc, BppImageSptr &&img) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	glyphs[gc] = std::move(img);
}

const BppImageSptr &BppFont::get(char32_t gc) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<char32_t, BppImageSptr>::const_iterator
		iter = glyphs.find(gc);
	if (iter != glyphs.end()) {
		BppImageSptr bis = renderGlyph(gc);
		glyphs[gc] = std::move(bis);
		return glyphs[gc];
	}
	return iter->second;
}

BppImageSptr BppFont::tryGet(char32_t gc) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<char32_t, BppImageSptr>::const_iterator
		iter = glyphs.find(gc);
	if (iter != glyphs.end()) {
		BppImageSptr bis;
		try {
			renderGlyph(gc);
			glyphs[gc] = bis;
		} catch (...) { }
		return bis;
	}
	return iter->second;
}

ImageDimensions BppFont::estimatedMaxCharacterSize() {
	//const static char32_t checkDim[] = { 'M', 'q' };
	ImageDimensions res(0, 0);
	for (char32_t check : { 'M', 'q' }) {
		BppImageSptr img = tryGet(check);
		if (img) {
			res = res.maxExtent(img->dimensions());
		}
	}
	return res;
}

/**
 * Used to hold all the glyphs needed to render a string.
 */
typedef std::vector<BppImageSptr>  ImageVec;
/**
 * Information on a line.
 */
struct LineDimensions {
	/**
	 * Number of characters in the line.
	 */
	int chars;
	/**
	 * Minimum dimensions for the line.
	 */
	ImageDimensions dim;
	/**
	 * Ensure all fields start with zeros.
	 */
	constexpr LineDimensions() : chars(0), dim(0, 0) { }
};
/**
 * Information on each line of text to render.
 */
typedef std::vector<LineDimensions>  DimVec;

BppImageSptr BppFont::render(const std::string &text, Flags flags)
try {
	// Will store all glyphs needed for output in the same order as text. All
	// lines are included. Line breaks are not.
	ImageVec gv;
	gv.reserve(text.size());
	// stores dimensions for each line
	DimVec lineDims(1);
	// output image dimesions
	ImageDimensions id(0, 0);
	// maximum glyph dimesions
	ImageDimensions md(0, 0);
	// greatest number of characters on a line
	int maxline = 0;
	// find all the needed glyphs
	std::string::const_iterator titer = text.cbegin();
	std::unordered_map<char32_t, BppImageSptr>::const_iterator giter;
	for (; titer != text.cend(); ++titer) {
		/** @todo  Make this work for characters larger than a byte. */
		// handle line breaks
		if (*titer == '\n') {
			// update maximum characters per line
			maxline = std::max(maxline, lineDims.back().chars);
			// update image size
			if (flags & FixedWidthPerLine) {
				// work out fixed width size for the line now
				lineDims.back().dim.w = md.w * lineDims.back().chars;
				// clear the maximum glyph width for the next line
				md.w = 0;
			} else if (!(flags & FixedWidth)) {
				// image width updated now for variable width; later for fixed
				id.w = std::max(id.w, lineDims.back().dim.w);
			}
			if (flags & VariableHeight) {
				// image height updated now for variable height; later for fixed
				id.h += lineDims.back().dim.h;
			}
			// create a new line entry
			lineDims.emplace_back(LineDimensions());
			// process the next character
			continue;
		}
		{ // get the next glyph
			std::lock_guard<duds::general::Spinlock> lock(block);
			giter = glyphs.find(*titer);
			// no glyph?
			if (giter == glyphs.end()) {
				// try to get it again; may throw
				gv.emplace_back(renderGlyph(*titer));
				// store it in the cache
				glyphs[*titer] = gv.back();
			} else {
				// have glyph
				gv.push_back(giter->second);
			}
		}
		// record dimension stats
		const ImageDimensions &gd = gv.back()->dimensions();
		lineDims.back().dim.w += gd.w;
		md.w = std::max(md.w, gd.w);
		lineDims.back().dim.h = std::max(lineDims.back().dim.h, gd.h);
		md.h = std::max(md.h, gd.h);
		++lineDims.back().chars;
	}
	// update maximum characters per line
	maxline = std::max(maxline, lineDims.back().chars);
	// update image size
	if (flags & FixedWidthPerLine) {
		// enusre the FixedWidth flag is not also set
		flags |= ~FixedWidth;
		// work out fixed width size for the line
		lineDims.back().dim.w = md.w * lineDims.back().chars;
	} else if (!(flags & FixedWidth)) {
		// update image width to fit line
		id.w = std::max(id.w, lineDims.back().dim.w);
	} else {
		// fixed width for all characters
		id.w = maxline * md.w;
		// width is not set per line in lineDims
	}
	if (flags & VariableHeight) {
		// update image height for variable height
		id.h += lineDims.back().dim.h;
	} else {
		// height is the same for all lines
		id.h = md.h * lineDims.size();
		// height is not set per line in lineDims
	}

	// gv now has all the needed glyphs in the order they must be rendered,
	// id has the final image dimensions, and lineDims has details on each
	// line.

	// make the destination image
	BppImageSptr bis = BppImage::make(id);
	// glyphs might not fill the whole image, so clear the image
	bis->clearImage();
	// iterate over the lines and the glyphs
	DimVec::iterator diter = lineDims.begin();
	ImageVec::iterator viter = gv.begin();
	ImageLocation il(0, 0);
	int tpos = 0;
	for (; diter != lineDims.end(); ++diter) {
		// update width & height if fixed across all lines
		if (flags & FixedWidth) {
			diter->dim.w = md.w * diter->chars;
		}
		if (!(flags & VariableHeight)) {
			diter->dim.h = md.h;
		}
		// set the left location for the first glyph of this line
		if (flags & AlignCenter) {
			il.x = (id.w - diter->dim.w) / 2;
		} else if (flags & AlignRight) {
			il.x = id.w - diter->dim.w;
		} else {
			il.x = 0;
		}
		// render each glyph
		for (int loop = diter->chars; loop > 0; ++viter, --loop) {
			const ImageDimensions &gd = (*viter)->dimensions();
			int adv = gd.w;
			// glyph may be short
			ImageLocation loc(il.x, il.y + diter->dim.h - gd.h);
			// glyph may be narrow
			if (flags & FixedWidth) {
				loc.x += (md.w - gd.w) / 2;
				adv = md.w;
			} else if (flags & FixedWidthPerLine) {
				adv = diter->dim.w / diter->chars;
				loc.x += (adv - gd.w) / 2;
			}
			bis->write(*viter, loc, gd);
			// advance  position to the left
			il.x += adv;
		}
		// set the upper location for the first glyph of the next line
		il.y += diter->dim.h;
	}

	return bis;
} catch (boost::exception &be) {
	be << String(text);
	throw;
}

} } }
