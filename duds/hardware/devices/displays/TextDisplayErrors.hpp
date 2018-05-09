/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/exception/info.hpp>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Base class for all errors specifically from a TextDisplay.
 */
struct TextDisplayError : virtual std::exception, virtual boost::exception { };
/**
 * The specified display size or location is unsupported.
 */
struct TextDisplayRangeError : TextDisplayError { };
/**
 * An attempt was made to use an uninitialized display object.
 */
struct TextDisplayUninitialized : TextDisplayError { };
/**
 * The index given for a definable glyph was outside the allowable range.
 */
struct TextDisplayGlyphIndexError : TextDisplayError { };
/**
 * The image given for a definable glyph was an unsupported size.
 */
struct TextDisplayGlyphSizeError : TextDisplayError { };

/**
 * Stores column and row data for display errors. The values may be for the
 * size of the display, or a location on it. Supports output to output
 * streams.
 */
struct Info_DisplayColRow {
	std::uint8_t col, row;
	Info_DisplayColRow(unsigned int c, unsigned int r) : col(c), row(r) { }
	template <class Char, class Traits>
	friend std::basic_ostream<Char, Traits> &operator << (
		std::basic_ostream<Char, Traits> &os, const Info_DisplayColRow &idrc
	) {
		return os << '(' << (int)idrc.col << ", " << (int)idrc.row << ')';
	}
};

/**
 * Column and row of a display position as part of an error.
 */
typedef boost::error_info<struct Info_DisplayPosition, Info_DisplayColRow>
	TextDisplayPositionInfo;
/**
 * Column and row size of a display as part of an error.
 */
typedef boost::error_info<struct Info_DisplaySize, Info_DisplayColRow>
	TextDisplaySizeInfo;
/**
 * Index used for a definable glyph.
 */
typedef boost::error_info<struct Info_DisplayGlyph, int>  TextDisplayGlyphIndex;

} } } }
