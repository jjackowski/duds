/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinSet.hpp>
#include <duds/hardware/interface/ChipSelect.hpp>
#include <duds/hardware/devices/displays/TextDisplay.hpp>
#include <chrono>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Implements text output to HD44780 and compatible displays. These displays
 * feature text output to a matrix that is typically 5 pixels wide by 8 tall
 * per character. The most common displays are LCDs, but some compatible
 * controllers are found on VFDs. They have a parallel interface with three
 * control lines. Only the 4-bit wide data interface is supported, and only
 * sending data to the display is supported, in order to minimize the number
 * of pins required.
 *
 * This class is @b not thread-safe because using it directly from multiple
 * threads makes little sense.
 *
 * @note  The one-way interface with the display used here makes it impossible
 *        to tell if there is a display on the other end, or if that display
 *        is functional.
 *
 * @todo  Support custom characters, preferably using a general interface that
 *        could work with other text displays using a 2D matrix.
 * @todo  Support the brightness control available on some VFDs.
 *
 * @author  Jeff Jackowski
 */
class HD44780 : public TextDisplay {
	/**
	 * Represents the 5 output lines, other than enable, that are needed to
	 * communicate with the LCD. The pins are, in order:
	 * -# Data bit 4
	 * -# Data bit 5
	 * -# Data bit 6
	 * -# Data bit 7
	 * -# Text flag; often labled "RS"
	 */
	duds::hardware::interface::DigitalPinSet outputs;
	/**
	 * Used to represent the enable line of the LCD. It must go low to make
	 * the display read data.
	 * @warning  Only one HD44780 object can be used with a
	 *           @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *           The other selectable item must not be a HD44780.
	 */
	duds::hardware::interface::ChipSelect enable;
	/**
	 * The best output configuration for the display bus given the port in use.
	 */
	std::vector<duds::hardware::interface::DigitalPinConfig> outcfg;
	/**
	 * The soonest time a new command can be sent to the display. The display
	 * requires some time to process incoming data. This value allows that time
	 * to elapse while the thread does something else.
	 */
	std::chrono::high_resolution_clock::time_point soonestSend;
	enum {
		/**
		 * General data mask for the display.
		 */
		dataMask   = 0xFF,
		/**
		 * Flag for sending text to the display rather than a command. Often
		 * labeled as "RS" in display documentation.
		 */
		textFlag   = 0x100,
		/**
		 * Unsupported display-side write flag.
		 */
		writeFlag  = 0x200,  // not presently supported; wire to ground
		/**
		 * Flag to send only a nibble rather than a whole byte; used in
		 * display initalization.
		 */
		nibbleFlag = 0x400
	};
	/**
	 * Waits until the time in @a soonestSend has passed. This is used to assure
	 * that the display isn't sent data too rapidly while allowing the thread
	 * a chance to do other work rather than always wait on the display. It is
	 * called in preparePins() so that functions needing to call sendByte() once
	 * do not need to call wait, and so that the wait occurs without hardware
	 * access to allow other threads a chance to use the hardware.
	 */
	void wait() const;
	/**
	 * Stores access objects together.
	 */
	struct Access {
		/**
		 * The set used for the 4 data pins and the text flag, more commonly
		 * referred to as "RS".
		 */
		duds::hardware::interface::DigitalPinSetAccess output;
		/**
		 * Used to assert the enable line of the LCD.
		 */
		duds::hardware::interface::ChipAccess enable;
	};
	/**
	 * Obtains access to the pins and configures them for output.
	 * Before obtaining access, a call to wait() is made. In the case of sending
	 * a single byte to the display, this allows at least 48us for other
	 * threads to use the hardware.
	 * @param acc  The object that contains the access objects to fill.
	 * @throw TextDisplayUninitialized  The display object has not been
	 *                                  given any pins to use.
	 */
	void preparePins(Access &acc);
	/**
	 * Sends a byte to the display a nibble at a time.
	 * @pre        A call to wait() has been made since the last call to this
	 *             function.
	 * @param acc  The access objects required to communicate with the display.
	 * @param val  The data to send. The value within @a dataMask is sent over
	 *             the four data lines. If @a textFlag is set, the text flag
	 *             line, more commonly known as "RS", will be set. Otherwise,
	 *             it will be cleared. If @a nibbleFlag is set, only the value
	 *             in the mask 0xF0 will be sent, and only one send will be
	 *             done instead of the normal two.
	 */
	void sendByte(Access &acc, int val);
	virtual void moveImpl(unsigned int c, unsigned int r);
	/**
	 * Writes a single character to the display. Using this function rather
	 * than one that writes a string allows the display's bus to be freed after
	 * each character to allow other uses for those lines.
	 */
	virtual void writeImpl(int c);
	void writeImpl(Access &acc, const std::string &text);
	virtual void writeImpl(const std::string &text);
	virtual void writeImpl(
		const std::string &text,
		unsigned int c,
		unsigned int r
	);
public:
	/**
	 * Initializes the object with an invalid display size and no pins to use.
	 */
	HD44780();
	/**
	 * Initializes the object with everything required to begin communicating
	 * with the display, but does not initalize the display.
	 * @post  The objects in @a outPins and @a enablePins are moved to internal
	 *        members. They will be the same as default-constructed objects
	 *        after this function returns.
	 * @param outPins    The set of pins used for 4-bit data output and the
	 *                   text flag. They are, in order:
	 *                    -# Data bit 4
	 *                    -# Data bit 5
	 *                    -# Data bit 6
	 *                    -# Data bit 7
	 *                    -# Text flag; often labeled "RS"
	 *                   The object is moved to an internal member.
	 * @param enablePin  The chip select used for the enable line on the
	 *                   display. It is often labeled "E".
	 *                   The object is moved to an internal member.
	 * @warning          Only one HD44780 can be used with a
	 *                   @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *                   The other selectable item must not be a HD44780.
	 * @param c          The number of columns on the display. The value must
	 *                   be between 1 and 20, and will almost always be either
	 *                   16 or 20.
	 * @param r          The number of rows on the display. The value must be
	 *                   between 1 and 4.
	 * @throw TextDisplayRangeError  Either the column or row size is
	 *                               outside the supported range.
	 * @throw duds::hardware::interface::PinDoesNotExist
	 * @throw duds::hardware::interface::PinRangeError
	 * @throw duds::hardware::interface::DigitalPinCannotOutputError
	 */
	HD44780(
		duds::hardware::interface::DigitalPinSet &outPins,
		duds::hardware::interface::ChipSelect &enablePin,
		unsigned int c,
		unsigned int r
	);
	/**
	 * Calls off().
	 */
	virtual ~HD44780();
	/**
	 * Sets the pins to use for communicating with the display.
	 * @post  The objects in @a outPins and @a enablePins are moved to internal
	 *        members. They will be the same as default-constructed objects
	 *        after this function returns.
	 * @param outPins    The set of pins used for 4-bit data output and the
	 *                   text flag. They are, in order:
	 *                    -# Data bit 4
	 *                    -# Data bit 5
	 *                    -# Data bit 6
	 *                    -# Data bit 7
	 *                    -# Text flag; often labeled "RS"
	 *                   The object is moved to an internal member.
	 * @param enablePin  The chip select used for the enable line on the
	 *                   display. It is often labeled "E".
	 *                   The object is moved to an internal member.
	 * @warning          Only one HD44780 can be used with a
	 *                   @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *                   The other selectable item must not be a HD44780.
	 * @param c          The number of columns on the display. The value must
	 *                   be between 1 and 20, and will almost always be either
	 *                   16 or 20.
	 * @param r          The number of rows on the display. The value must be
	 *                   between 1 and 4.
	 * @throw TextDisplayRangeError  Either the column or row size is
	 *                               outside the supported range.
	 * @throw duds::hardware::interface::PinDoesNotExist
	 * @throw duds::hardware::interface::PinRangeError
	 * @throw duds::hardware::interface::DigitalPinCannotOutputError
	 */
	void configure(
		duds::hardware::interface::DigitalPinSet &outPins,
		duds::hardware::interface::ChipSelect &enablePin,
		unsigned int c,
		unsigned int r
	);
	/**
	 * Initializes the display for use. This function must be called before
	 * sending text or any other commands to the display. It may be called
	 * more than once.
	 * @pre   The pins to use have already been configured, and the display
	 *        size has been set.
	 * @post  Functions that send data to the display may be used.
	 * @post  The display is blank; it has no text.
	 * @post  The cursor is positioned at the upper left corner.
	 * @throw TextDisplayUninitialized  The display object has not been
	 *                                  given any pins to use.
	 */
	void initialize();
	/**
	 * Commands the display to turn off. This should prevent any text from being
	 * visible, but may not appear to do anything else. The text displayed
	 * prior to calling this function should remain in the display's buffer.
	 * @pre  initialize() has been successfully called.
	 * @bug  Name isn't technically correct; change it.
	 */
	void off();
	/**
	 * Commands the display to turn on. This is done inside initialize() so it
	 * is only needed if off() is called. The contents of the displays buffer
	 * should become visible after this function.
	 * @pre  initialize() has been successfully called.
	 * @bug  Name isn't technically correct; change it.
	 */
	void on();
	/**
	 * Removes all text from the display and moves the cursor to the upper left
	 * corner.
	 * @pre  initialize() has been successfully called.
	 */
	virtual void clear();
};

} } } }
