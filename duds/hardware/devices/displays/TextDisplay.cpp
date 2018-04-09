/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/displays/TextDisplay.hpp>
#include <duds/general/Errors.hpp>
#include <thread>

namespace duds { namespace hardware { namespace devices { namespace displays {

TextDisplay::TextDisplay() :
columnsize(-1), rowsize(-1), cpos(-1), rpos(-1) { }

TextDisplay::TextDisplay(unsigned int c, unsigned int r) :
columnsize(c), rowsize(r), cpos(-1), rpos(-1) { }

TextDisplay::~TextDisplay() { }

bool TextDisplay::advance() {
	// next line?
	if (++cpos >= columnsize) {
		cpos = 0;
		if (++rpos >= rowsize) {
			rpos = 0;
		}
		return true;
	}
	return false;
}

void TextDisplay::move(unsigned int c, unsigned int r) {
	// range check
	if ((c >= columnsize) || (r >= rowsize)) {
		DUDS_THROW_EXCEPTION(TextDisplayRangeError() <<
			TextDisplayPositionInfo(Info_DisplayColRow(c, r)) <<
			TextDisplaySizeInfo(Info_DisplayColRow(columnsize, rowsize))
		);
	}
	// check for a change
	if ((c != cpos) || (r != rpos)) {
		moveImpl(c, r);
		// record new position
		cpos = c;
		rpos = r;
	}
}

void TextDisplay::write(int c) {
	writeImpl(c);
	// advance position; may require repositioning
	if (advance()) {
		moveImpl(cpos, rpos);
	}
}

void TextDisplay::writeImpl(const std::string &text) {
	// loop through characters
	std::string::const_iterator iter = text.begin();
	do {
		write(*iter);
	} while (++iter != text.end());
}

void TextDisplay::write(const std::string &text) {
	if (text.empty()) {
		// already done
		return;
	}
	writeImpl(text);
}

void TextDisplay::writeImpl(
	const std::string &text,
	unsigned int c,
	unsigned int r
) {
	move(c, r);
	// do the write
	write(text);
}

void TextDisplay::write(
	const std::string &text,
	unsigned int c,
	unsigned int r
) {
	// range check
	if ((c >= columnsize) || (r >= rowsize)) {
		DUDS_THROW_EXCEPTION(TextDisplayRangeError() <<
			TextDisplayPositionInfo(Info_DisplayColRow(c, r)) <<
			TextDisplaySizeInfo(Info_DisplayColRow(columnsize, rowsize))
		);
	}
	if (text.empty()) {
		// already done
		return;
	}
	writeImpl(text, c, r);
}

void TextDisplay::clearTo(unsigned int c, unsigned int r) {
	// range check
	if ((c >= columnsize) || (r >= rowsize)) {
		DUDS_THROW_EXCEPTION(TextDisplayRangeError() <<
			TextDisplayPositionInfo(Info_DisplayColRow(c, r)) <<
			TextDisplaySizeInfo(Info_DisplayColRow(columnsize, rowsize))
		);
	}
	while ((cpos != c) || (rpos != r)) {
		write(' ');
	}
	write(' ');
}

} } } }
