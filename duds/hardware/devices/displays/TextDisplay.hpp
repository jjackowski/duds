/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef TEXTDISPLAY_HPP
#define TEXTDISPLAY_HPP

#include <duds/hardware/devices/displays/TextDisplayErrors.hpp>

namespace duds { namespace hardware { namespace devices {

/**
 * Support for display devices, both actual display units and ways of using
 * them. The focus here will be on non-graphical displays because there is
 * a lot of code for graphics and plenty of support elsewhere.
 */
namespace displays {

/**
 * A fairly generic interface to a character based display that lacks color.
 *
 * This class is @b not thread-safe because using text displays directly from
 * multiple threads makes little sense.
 *
 * @author  Jeff Jackowski
 */
class TextDisplay {
protected:
	/**
	 * Number of columns on the display.
	 */
	std::uint8_t columnsize;
	/**
	 * Number of rows on the display.
	 */
	std::uint8_t rowsize;
	/**
	 * Cursor column position.
	 */
	std::uint8_t cpos;
	/**
	 * Cursor row position.
	 */
	std::uint8_t rpos;
	/**
	 * Advances the column position, and if it goes off the visible portion of
	 * the display, updates the row position. Returns true if moveImpl() or
	 * similar function must be called to reposition the cursor onto a visible
	 * spot. The default implementation is for displays that do not keep the
	 * cursor on the visible part of the display when moving past the end of
	 * a row.
	 */
	virtual bool advance();
	/**
	 * Moves the display's cursor to the indicated position. The position has
	 * already passed a range check. The new position will be recorded by the
	 * caller, move(unsigned int, unsigned int), so there is no need for this
	 * function to record the new position.
	 */
	virtual void moveImpl(unsigned int c, unsigned int r) = 0;
	/**
	 * Writes a single character onto the display at the current cursor
	 * location. The cursor location is already set prior to the call. After
	 * the call, advance() is called to move the cursor.
	 */
	virtual void writeImpl(int c) = 0;
	/**
	 * Writes a string to the display. This function must handle advancing the
	 * cursor, and must reposition the cursor when @a repos is true.
	 * The default implementation calls writeImpl(int), advance(), and
	 * moveImpl() in a loop.
	 */
	virtual void writeImpl(const std::string &text);
	/**
	 * Writes a string to the display starting at the indicated location. This
	 * function must handle moving and advancing the cursor.
	 * The default implementation calls move(), then write(const std::string &).
	 */
	virtual void writeImpl(
		const std::string &text,
		unsigned int c,
		unsigned int r
	);
	/**
	 * Initializes the object with an invalid display size and cursor position.
	 */
	TextDisplay();
	/**
	 * Initializes the object with the given display size and an invalid
	 * cursor position.
	 * @param c  The number of columns on the display.
	 * @param r  The number of rows on the display.
	 */
	TextDisplay(
		unsigned int c,
		unsigned int r
	);
public:
	/**
	 * Allows destruction of a pointer of this base class to properly destruct
	 * the derived object.
	 */
	virtual ~TextDisplay() = 0;
	/**
	 * Moves the cursor to the given location.
	 * @param c  The destination column.
	 * @param r  The destination row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	void move(unsigned int c, unsigned int r);
	/**
	 * Writes a single character onto the display at the current cursor
	 * location and advances the cursor.
	 * @pre         initialize() has been successfully called.
	 * @param c  The character to write.
	 */
	void write(int c);
	/**
	 * Writes a string onto the display starting from the current cursor
	 * location. If the cursor moves off the visible portion of the display, it
	 * will be moved to a visible spot. The spot will be the start of the next
	 * row down, or if no such row exixts, the start of the first row.
	 * @pre         initialize() has been successfully called.
	 * @param text  The string to write.
	 */
	void write(const std::string &text);
	/**
	 * Writes a string onto the display starting from the given location. If
	 * the cursor moves off the visible portion of the display, it
	 * will be moved to a visible spot. The spot will be the start of the next
	 * row down, or if no such row exixts, the start of the first row.
	 * @pre         initialize() has been successfully called.
	 * @param text  The string to write.
	 * @param c     The starting column.
	 * @param r     The starting row.
	 */
	void write(const std::string &text, unsigned int c, unsigned int r);
	/**
	 * Removes all text from the display and moves the cursor to the upper left
	 * corner.
	 * @pre  initialize() has been successfully called.
	 */
	virtual void clear() = 0;
	/**
	 * Clear text from the current cursor position to the given position,
	 * inclusive. The cursor will be moved to the spot immediately after the
	 * given position. The implementation writes spaces while the cursor is not
	 * at the position, then writes one more.
	 * @param c     The end column.
	 * @param r     The end row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	void clearTo(unsigned int c, unsigned int r);
	/**
	 * Returns the number of columns on the display. This value is supplied to
	 * the object by the using program rather than by the display.
	 */
	unsigned int columns() const {
		return columnsize;
	}
	/**
	 * Returns the number of rows on the display. This value is supplied to
	 * the object by the using program rather than by the display.
	 */
	unsigned int rows() const {
		return rowsize;
	}
	/**
	 * The current column position of the cursor.
	 */
	unsigned int columnPos() const {
		return cpos;
	}
	/**
	 * The current row position of the cursor.
	 */
	unsigned int rowPos() const {
		return rpos;
	}
};

} } } }

#endif        //  #ifndef TEXTDISPLAY_HPP
