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
#include <duds/hardware/display/TextDisplay.hpp>
#include <duds/general/Spinlock.hpp>
#include <iostream>
#include <cstring>

namespace duds { namespace hardware { namespace display {

/**
 * Moves output from an output stream to a TextDisplay one character at a time.
 * The characters from the stream are immediately sent to the display. This
 * works well for HD44780 type displays since they need time to process the
 * data and the implementation will relinquish hardware access between
 * characters.
 *
 * The way this class uses the functions in TextDisplay allows other code to
 * output using TextDisplay directly and interchangeably with this class.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicStreambuf : public std::basic_streambuf<Char, Traits> {
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
			case '\n':
				disp->clearTo(disp->columns() - 1, disp->rowPos());
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
	TextDisplayBasicStreambuf(const std::shared_ptr<TextDisplay> &d) : disp(d) {
		assert(d);
	}
	/**
	 * Returns the output display.
	 */
	const std::shared_ptr<TextDisplay> &display() const {
		return disp;
	}
	/**
	 * Returns the cursos's column position.
	 */
	virtual unsigned int column() const {
		return disp->columnPos();
	}
	/**
	 * Returns the cursos's row position.
	 */
	virtual unsigned int row() const {
		return disp->rowPos();
	}
	/**
	 * Moves the display's cursor to the given location.
	 * @param c  The destination column.
	 * @param r  The destination row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	virtual void moveCursor(unsigned int c, unsigned int r) {
		disp->move(c, r);
	}
	/**
	 * Remove all text from the display and place the cursor in the upper
	 * left corner.
	 */
	virtual void clearDisplay() {
		disp->clear();
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
	virtual void clearTo(unsigned int c, unsigned int r) {
		disp->clearTo(c, r);
	}
	/**
	 * Moves the cursor to the start of a line clearing text along the way. If
	 * the cursor is already at the start of a line, it will not move and no
	 * text will be cleared.
	 */
	virtual void startLine() {
		TextDisplay *td = disp.get();
		if (td) {
			if (td->columnPos() > 0) {
				td->clearTo(td->columns() - 1, td->rowPos());
			}
		}
	}
};

/**
 * Writes output from a stream into an internal buffer, and writes that buffer
 * to the display when sync() is called. Only changes are written to the
 * display, and the display is only updated when sync() is called. Using
 * std::endl with the output stream, or calling flush on the stream, will
 * cause a call to sync().
 * @par Thread safety
 * Objects of this type are thread-safe for the case of exactly two threads:
 * one writing new text, and another thread that flushes the stream
 * (calls sync()). The thread that flushes will handle outputing text changes
 * to the display. The other thread that writes new text will not be stalled by
 * the display output. Using two threads is not a requirement.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicBufferedStreambuf :
public TextDisplayBasicStreambuf<Char, Traits> {
	using base = TextDisplayBasicStreambuf<Char, Traits>;
	/**
	 * The buffer currently shown by the display.
	 */
	std::vector<Char> shown;
	/**
	 * The buffer that is being written to the display.
	 */
	std::vector<Char> update;
	/**
	 * The buffer that accepts new data.
	 */
	std::vector<Char> working;
	/**
	 * Write block.
	 */
	duds::general::Spinlock wblock;
	/**
	 * Number of columns on the display.
	 * Sizes included here because they reduce calls to the display through a
	 * shared pointer, and 4 byte memory alignment prevents their inclusion
	 * from needing more memory.
	 */
	std::uint8_t columnsize;
	/**
	 * Number of rows on the display.
	 * Sizes included here because they reduce calls to the display through a
	 * shared pointer, and 4 byte memory alignment prevents their inclusion
	 * from needing more memory.
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
	 * Internal function to write a printable character to the buffer.
	 */
	void write(Char c) {
		// write out the character
		*base::pptr() = c;
		// advance to the next character
		if (++cpos >= columnsize) {
			cpos = 0;
			if (++rpos >= rowsize) {
				rpos = 0;
				// reposition to start of buffer
				base::setp(base::pbase(), base::epptr());
				return;
			}
		}
		base::pbump(1);
	}
	/**
	 * Clears a portion of the buffer with spaces from the current cursor
	 * position to the indicated position. The cursor will be moved to the spot
	 * immediately after the given position.
	 * @pre         The given position is within the bounds of the display.
	 * @param c     The end column.
	 * @param r     The end row.
	 */
	void clearToImpl(unsigned int c, unsigned int r) {
		while ((cpos != c) || (rpos != r)) {
			write(' ');
		}
		write(' ');
	}
	/**
	 * Handles writing a character into the buffer or moving the cursor for
	 * new lines and carriage returns.
	 */
	void bufWrite(Char c) {
		// no locking; must already be locked
		switch (c) {
			// carriage return
			case '\r':
				base::pbump(-(int)cpos);
				cpos = 0;
				break;
			// new line
			case '\n':
				clearToImpl(columnsize - 1, rpos);
				break;
			// something else
			default:
				// write out the character
				write((int)c);
		}
	}
protected:
	/**
	 * Writes a character to the start of the display after wrapping around from
	 * the end.
	 */
	virtual typename Traits::int_type overflow(
		typename Traits::int_type c = Traits::eof()
	) {
		// there is no end to the data the display can take
		if (c == Traits::eof()) {
			return Traits::not_eof(c);
		}
		duds::general::SpinLockGuard lock(wblock);
		bufWrite((Char)c);
	}
	virtual std::streamsize xsputn(const Char* s, std::streamsize count) {
		duds::general::SpinLockGuard lock(wblock);
		for (std::streamsize cnt = count; cnt > 0; ++s, --cnt) {
			bufWrite(*s);
		}
		return count;
	}
	virtual int sync() {
		// copy data to write to the display; allows another thread to keep
		{ // writing more text
			duds::general::SpinLockGuard lock(wblock);
			update = working;
		}
		// compare update and shown buffer to output what is needed
		typename std::vector<Char>::iterator siter = shown.begin();
		typename std::vector<Char>::iterator uiter = update.begin();
		bool cont = false; // contiguous flag
		for (int r = 0; r < rowsize; ++r) {
			for (int c = 0; c < columnsize; ++siter, ++uiter, ++c) {
				// difference?
				if (*siter != *uiter) {
					// non-contiguous?
					if (!cont) {
						// reposition cursor
						base::display()->move(c, r);
						cont = true;
					}
					assert(base::display()->columnPos() == c);
					assert(base::display()->rowPos() == r);
					// write data
					base::display()->write((int)*uiter);
					// update shown buffer
					*siter = *uiter;
				} else {
					// no longer contiguos
					cont = false;
				}
			}
		}
		return 0; // success code
	}
	virtual typename base::pos_type seekoff(
		typename base::off_type off,
		std::ios_base::seekdir dir,
		std::ios_base::openmode which = std::ios_base::out
	) {
		// only output seeking is supported
		if (which & std::ios_base::out) {
			duds::general::SpinLockGuard lock(wblock);
			// figure out new absolute position
			typename base::pos_type cp = base::pptr() - base::pbase();
			typename base::pos_type op;
			switch (dir) {
				case std::ios_base::beg:
					op = off;
					break;
				case std::ios_base::cur:
					op = cp + off;
					break;
				case std::ios_base::end:
					op = working.size() + off;
					break;
				default:
					return typename base::pos_type(-1);
			}
			// do not seek out of bounds
			if ((op > 0) && (op < working.size())) {
				// reposition to start
				base::setp(base::pbase(), base::epptr());
				// seek to requested position
				base::pbump(op);
				// compute row & column positions
				rpos = op / columnsize;
				cpos = op % columnsize;
				return op;
			}
		}
		return typename base::pos_type(-1);
	}
	virtual typename base::pos_type seekpos(
		typename base::pos_type pos,
		std::ios_base::openmode which = std::ios_base::out
	) {
		return seekoff(typename base::off_type(pos), std::ios_base::beg, which);
	}
public:
	/**
	 * Makes the stream buffer with a display for output.
	 * @param d  The display that will take the output.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBasicBufferedStreambuf(const std::shared_ptr<TextDisplay> &d) :
	TextDisplayBasicStreambuf<Char, Traits>(d),
	columnsize(d->columns()),
	rowsize(d->rows()),
	cpos(0),
	rpos(0),
	shown(d->columns() * d->rows(), ' '),
	update(d->columns() * d->rows(), ' '),
	working(d->columns() * d->rows(), ' ') {
		base::setp(&(working[0]), &(working[0]) + (columnsize * rowsize));
	}
	virtual unsigned int column() const {
		return cpos;
	}
	virtual unsigned int row() const {
		return rpos;
	}
	virtual void moveCursor(unsigned int c, unsigned int r) {
		// range check
		if ((c >= columnsize) || (r >= rowsize)) {
			DUDS_THROW_EXCEPTION(DisplayBoundsError() <<
				TextDisplayPositionInfo(Info_DisplayColRow(c, r)) <<
				TextDisplaySizeInfo(Info_DisplayColRow(columnsize, rowsize))
			);
		}
		duds::general::SpinLockGuard lock(wblock);
		base::setp(base::pbase(), base::epptr());
		cpos = c;
		rpos = r;
		base::pbump(r * columnsize + c);
	}
	virtual void clearDisplay() {
		duds::general::SpinLockGuard lock(wblock);
		// make the work-in-progress all spaces
		std::memset(&(working[0]), ' ', working.size());
		// reposition to start of buffer
		base::setp(base::pbase(), base::epptr());
		cpos = rpos = 0;
	}
	virtual void clearTo(unsigned int c, unsigned int r) {
		// range check
		if ((c >= columnsize) || (r >= rowsize)) {
			DUDS_THROW_EXCEPTION(DisplayBoundsError() <<
				TextDisplayPositionInfo(Info_DisplayColRow(c, r)) <<
				TextDisplaySizeInfo(Info_DisplayColRow(columnsize, rowsize))
			);
		}
		duds::general::SpinLockGuard lock(wblock);
		clearToImpl(c, r);
	}
	virtual void startLine() {
		duds::general::SpinLockGuard lock(wblock);
		if (cpos > 0) {
			clearToImpl(columnsize - 1, rpos);
		}
	}
};

/**
 * The base class for output streams that write to TextDisplay objects.
 * The stream manipulators made for text displays all work with this class
 * so there isn't a need for multiple sets of the stream manipulators, and the
 * manipulators do not need a run-time check for multiple stream types. This
 * class can be used directly, but it is easier to use one of the derived
 * classes.
 * @author  Jeff Jackowski
 */
template <class Char = char, class Traits = std::char_traits<Char> >
class TextDisplayBaseStream : public std::basic_ostream<Char, Traits> {
	using base = std::basic_ostream<Char, Traits>;
	/**
	 * The buffer handling the output.
	 */
	TextDisplayBasicStreambuf<Char, Traits> *tdbb;
	/**
	 * The value from xalloc() used for stream manipulators to identify this
	 * stream type.
	 */
	static const int xidx;
public:
	/**
	 * Makes an output stream that writes to the given display.
	 * @param tbuf  The stream buffer that will handle output to the display.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBaseStream(TextDisplayBasicStreambuf<Char, Traits> *tbuf) :
	tdbb(tbuf) {
		base::init(tdbb);
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
		return tdbb->display();
	}
	/**
	 * Moves the display's cursor to the given location.
	 * @param c  The destination column.
	 * @param r  The destination row.
	 * @throw TextDisplayRangeError  The requested position is beyond the
	 *                               display's boundries.
	 */
	void moveCursor(unsigned int c, unsigned int r) {
		tdbb->moveCursor(c, r);
	}
	/**
	 * Remove all text from the display and place the cursor in the upper
	 * left corner.
	 */
	void clearDisplay() {
		tdbb->clearDisplay();
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
		tdbb->clearTo(c, r);
	}
	/**
	 * Moves the cursor to the start of a line clearing text along the way. If
	 * the cursor is already at the start of a line, it will not move and no
	 * text will be cleared.
	 */
	void startLine() {
		tdbb->startLine();
	}
};


// convineince classes; could use TextDisplayBasicBuffer or
// TextDisplayStoredBuffer without them
/**
 * An output stream for immediately writing data to TextDisplay objects.
 * It supports the use of stream modifiers intended specifically for use
 * with TextDisplay. Stream seeking is not supported, but repositioning the
 * display's cursor is supported.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicStream : public TextDisplayBaseStream<Char, Traits> {
	/**
	 * The buffer handling the output.
	 */
	TextDisplayBasicStreambuf<Char, Traits> buff;
public:
	/**
	 * Makes an output stream that immediately writes to the given display.
	 * @param d  The display that will receive output.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBasicStream(const std::shared_ptr<TextDisplay> &d) :
	buff(d), TextDisplayBaseStream<Char, Traits>(&buff) { }
};

/**
 * An output stream for buffering writes to TextDisplay objects.
 * It supports the use of stream modifiers intended specifically for use
 * with TextDisplay. Stream seeking is supported, but repositioning the
 * display's cursor may be easier to manage. Data is not written to the display
 * until the stream is flushed.
 * @note  Output to this stream may be incorrect unless all output to the
 *        display is done through this stream. None of the
 *        TextDisplay::write() functions should be called on the display
 *        except through this stream's TextDisplayBufferedStreambuf object.
 * @par Thread safety (from TextDisplayBufferedStreambuf)
 *      Objects of this type are thread-safe for the case of exactly two
 *      threads: one writing new text, and another thread that flushes the
 *      stream. The thread that flushes will handle outputing text changes
 *      to the display. The other thread that writes new text will not be
 *      stalled by the display output. Using two threads is not a requirement.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits = std::char_traits<Char> >
class TextDisplayBasicBufferedStream : public TextDisplayBaseStream<Char, Traits> {
	/**
	 * The buffer handling the output.
	 */
	TextDisplayBasicBufferedStreambuf<Char, Traits> buff;
public:
	/**
	 * Makes an output stream that writes to a buffer, and writes that buffer
	 * to the given display when flushed.
	 * @param d  The display that will receive output.
	 * @pre   The display is initialzied and ready to accept data.
	 */
	TextDisplayBasicBufferedStream(const std::shared_ptr<TextDisplay> &d) :
	buff(d), TextDisplayBaseStream<Char, Traits>(&buff) { }
};

/**
 * The value from xalloc() used for stream manipulators to identify this
 * stream type. Needed for each template instantiation.
 */
template <class Char, class Traits>
const int TextDisplayBaseStream<Char, Traits>::xidx = std::ios_base::xalloc();

/**
 * Display stream manipulator that clears all text from the display and places
 * the cursor in the upper left corner. The modifer will do nothing if the
 * stream is not a TextDisplayBasicStream type.
 * @author  Jeff Jackowski
 */
template <class Char, class Traits>
std::basic_ostream<Char, Traits> &clear(std::basic_ostream<Char, Traits> &os) {
	if (os.pword(TextDisplayBaseStream<Char, Traits>::xallocIndex()) == &os) {
		static_cast<TextDisplayBaseStream<Char, Traits>&>(os).clearDisplay();
	}
	return os;
}

/**
 * Moves the cursor to the start of a line clearing text along the way. If
 * the cursor is already at the start of a line, it will not move and no
 * text will be cleared.
 */
template <class Char, class Traits>
std::basic_ostream<Char, Traits> &startLine(std::basic_ostream<Char, Traits> &os) {
	if (os.pword(TextDisplayBaseStream<Char, Traits>::xallocIndex()) == &os) {
		static_cast<TextDisplayBaseStream<Char, Traits>&>(os).startLine();
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
		if (os.pword(TextDisplayBaseStream<Char, Traits>::xallocIndex()) == &os) {
			static_cast<TextDisplayBaseStream<Char, Traits>&>(os).moveCursor(
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
		if (os.pword(TextDisplayBaseStream<Char, Traits>::xallocIndex()) == &os) {
			static_cast<TextDisplayBaseStream<Char, Traits>&>(os).clearTo(
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
 * Most common type for the TextDisplayBasicStreambuf.
 */
typedef TextDisplayBasicStreambuf<char>  TextDisplayStreambuf;
/**
 * Most common type for the TextDisplayBasicBufferedStreambuf.
 */
typedef TextDisplayBasicBufferedStreambuf<char>  TextDisplayBufferedStreambuf;
/**
 * Most common type for the TextDisplayBasicStream.
 */
typedef TextDisplayBasicStream<char>  TextDisplayStream;
/**
 * Most common type for the TextDisplayBasicBufferedStream.
 */
typedef TextDisplayBasicBufferedStream<char>  TextDisplayBufferedStream;

} } }
