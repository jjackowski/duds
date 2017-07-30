/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * Defines output stream and related items for use with TextDisplay objects.
 */
#include <duds/hardware/devices/displays/TextDisplay.hpp>
#include <iostream>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Moves output from an output stream to a TextDisplay. Presently works a
 * character at a time, but this works well for HD44780 type displays since
 * they need time to process the data and the implementation will relinquish
 * hardware access between characters.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicBuffer : public std::basic_streambuf<Char, Traits> {
	/**
	 * The display that will receive the output.
	 */
	std::shared_ptr<TextDisplay> disp;
protected:
	/**
	 * Writes a character to the display.
	 */
	virtual typename Traits::int_type overflow(
		typename Traits::int_type c = Traits::eof()
	) {
		// nowhere to go; return an error
		if (!disp) {
			return Traits::eof();
		}
		// there is no end to the data the display can take
		if (c == Traits::eof()) {
			return Traits::not_eof(c);
		}
		switch (c) {
			// carriage return
			case '\r':
				disp->move(0, disp->rowPos());
				break;
			// new line
			case '\n': { /*
				unsigned int r = disp->rowPos() + 1;
				if (r >= disp->rows()) {
					r = 0;
				}
				disp->move(0, r); */
				disp->clearTo(disp->columns() - 1, disp->rowPos());
				}
				break;
			// something else
			default:
				// write out the character
				disp->write((int)c);
		}
		return c;
	}
public:
	/**
	 * Makes the stream buffer with a display for output.
	 * @param d  The display that will take the output.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBasicBuffer(const std::shared_ptr<TextDisplay> &d) : disp(d) { }
	/**
	 * Returns the output display.
	 */
	const std::shared_ptr<TextDisplay> &display() const {
		return disp;
	}
};

/**
 * An output stream specifically for writing data to TextDisplay objects.
 * It supports the use of stream modifiers intended specifically for use
 * with TextDisplay.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicStream : public std::basic_ostream<Char, Traits> {
	/**
	 * The buffer handling the output.
	 */
	TextDisplayBasicBuffer<Char, Traits> tdbb;
	/**
	 * The value from xalloc() used for stream manipulators to identify this
	 * stream type.
	 */
	static const int xidx;
public:
	/**
	 * Makes an output stream that writes to the given display.
	 * @param d  The display that will receive output.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBasicStream(const std::shared_ptr<TextDisplay> &d) :
	std::basic_ostream<Char, Traits>(&tdbb), tdbb(d) {
		this->pword(xidx) = this;
	}
	/**
	 * Returns the index from xalloc(); needed by the stream manipulators.
	 */
	static int xallocIndex() {
		return xidx;
	}
	/**
	 * Returns the output display.
	 */
	const std::shared_ptr<TextDisplay> &display() const {
		return tdbb.display();
	}
	/**
	 * Moves the display's cursor to the given location.
	 * @param c  The destination column.
	 * @param r  The destination row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	void moveCursor(unsigned int c, unsigned int r) {
		if (display()) {
			display()->move(c, r);
		}
	}
	/**
	 * Remove all text from the display and place the cursor in the upper
	 * left corner.
	 */
	void clearDisplay() {
		if (display()) {
			display()->clear();
		}
	}
	/**
	 * Clear text from the current cursor position to the given position,
	 * inclusive. The cursor will be moved to the spot immediately after the
	 * given position.
	 * @param c     The end column.
	 * @param r     The end row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	void clearTo(unsigned int c, unsigned int r) {
		if (display()) {
			display()->clearTo(c, r);
		}
	}
};

/**
 * The value from xalloc() used for stream manipulators to identify this
 * stream type. Needed for each template instantiation.
 */
template <class Char, class Traits>
const int TextDisplayBasicStream<Char, Traits>::xidx = std::ios_base::xalloc();

/**
 * Display stream manipulator that clears all text from the display and places
 * the cursor in the upper left corner. The modifer will do nothing if the
 * stream is not a TextDisplayBasicStream type.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits>
std::basic_ostream<Char, Traits> &clear(std::basic_ostream<Char, Traits> &os) {
	if (os.pword(TextDisplayBasicStream<Char, Traits>::xallocIndex()) == &os) {
		static_cast<TextDisplayBasicStream<Char, Traits>&>(os).clearDisplay();
	}
	return os;
}

/**
 * Part of the move() display stream manipulator. Stores the parameters of the
 * manipulator and provides an insertion operator.
 * @author  Jeff Jackowski
 */
struct move_impl {
	unsigned int col;
	unsigned int row;
	move_impl(unsigned int c, unsigned int r) : col(c), row(r) { }
	template <class Char, class Traits>
	friend std::basic_ostream<Char, Traits> &operator << (
		std::basic_ostream<Char, Traits> &os, const move_impl &mi
	) {
		if (os.pword(TextDisplayBasicStream<Char, Traits>::xallocIndex()) == &os) {
			static_cast<TextDisplayBasicStream<Char, Traits>&>(os).moveCursor(
				mi.col, mi.row
			);
		}
		return os;
	}
};

/**
 * Display stream manipulator that moves the display cursor to the given
 * location.
 * @param c  The destination column.
 * @param r  The destination row.
 * @throw TextDisplayRangeError  The requested position is beyond the
 *                               display's boundries.
 * @author  Jeff Jackowski
 */
inline move_impl move(unsigned int c, unsigned int r) {
	return move_impl(c, r);
}

/**
 * Part of the clearTo() display stream manipulator. Stores the parameters of
 * the manipulator and provides an insertion operator.
 * @author  Jeff Jackowski
 */
struct clearTo_impl {
	unsigned int col;
	unsigned int row;
	clearTo_impl(unsigned int c, unsigned int r) : col(c), row(r) { }
	template <class Char, class Traits>
	friend std::basic_ostream<Char, Traits> &operator << (
		std::basic_ostream<Char, Traits> &os, const clearTo_impl &mi
	) {
		if (os.pword(TextDisplayBasicStream<Char, Traits>::xallocIndex()) == &os) {
			static_cast<TextDisplayBasicStream<Char, Traits>&>(os).clearTo(
				mi.col, mi.row
			);
		}
		return os;
	}
};

/**
 * Display stream manipulator that clears the display from the current cursor
 * location to the given location. The cursor will be moved to the spot
 * immediately after the given position.
 * @param c  The end column.
 * @param r  The end row.
 * @throw TextDisplayRangeError  The requested position is beyond the
 *                               display's boundries.
 * @author  Jeff Jackowski
 */
inline clearTo_impl clearTo(unsigned int c, unsigned int r) {
	return clearTo_impl(c, r);
}

/**
 * Most common type for the TextDisplayBasicBuffer.
 */
typedef TextDisplayBasicBuffer<char>  TextDisplayBuffer;
/**
 * Most common type for the TextDisplayBasicStream.
 */
typedef TextDisplayBasicStream<char>  TextDisplayStream;

} } } }
