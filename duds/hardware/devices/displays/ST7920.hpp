/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinSet.hpp>
#include <duds/hardware/interface/ChipSelect.hpp>
#include <duds/hardware/display/BppGraphicDisplay.hpp>
#include <chrono>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Implements graphic output to the ST7920 LCD controller using a 4-bit
 * parallel interface.
 *
 * @note  If a darkend pixel has no adjacent darkened pixel in the same column,
 *        it will have low contrast. Two or more contiguous darkened pixels
 *        in the same column have much better contrast.
 *
 * The controller manages a display up to 256 by 64 pixels with an internal
 * frame buffer. This driver maintains its own frame buffer so that it can
 * update only the portions of the frame that change. The controller has a
 * parallel interface that is almost identical to the HD44780's. It has three
 * control lines and 4 or 8 data lines. Only the 4-bit wide data
 * interface is supported, and only sending data to the display is supported.
 * This limits the number of digital I/O lines required.
 *
 * This class is @b not thread-safe because using it directly from multiple
 * threads makes little sense.
 *
 * @note  The one-way interface with the display used here makes it impossible
 *        to tell if there is a display on the other end, or if that display
 *        is functional.
 *
 * @author  Jeff Jackowski
 */
class ST7920 : public duds::hardware::display::BppGraphicDisplay {
	/**
	 * Represents the 5 output lines, other than enable, that are needed to
	 * communicate with the LCD. The pins are, in order:
	 * -# Data bit 4
	 * -# Data bit 5
	 * -# Data bit 6
	 * -# Data bit 7
	 * -# Image data flag; often labled "RS"
	 */
	duds::hardware::interface::DigitalPinSet outputs;
	/**
	 * Used to represent the enable line of the LCD. It must go low to make
	 * the display read data.
	 * @warning  Only one ST7920 object can be used with a
	 *           @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *           The other selectable item must not be a ST7920.
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
	/**
	 * The amount of time to allow the display to read data.
	 */
	std::chrono::nanoseconds nibblePeriod;
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
	 * a single byte to the display, this allows at least 78us for other
	 * threads to use the hardware.
	 * @param acc  The object that contains the access objects to fill.
	 * @throw DisplayUninitialized  The display object has not been
	 *                              given any pins to use.
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
	/**
	 * Writes a contiguous block of pixel data to the display after setting the
	 * next location to write.
	 * @param acc    The access objects required to communicate with the
	 *               display.
	 * @param start  The starting address of the pixel data to send.
	 * @param end    The ending address of the pixel data to send. May be the
	 *               same as @a start, but should not extend past the end of
	 *               the current horizontal line.
	 * @param pos    The starting position to send to the display. The
	 *               Y-coordinate is in the LSB, and the X-coordinate is
	 *               shifted to lose the low order 4 bits and placed in the
	 *               next significant byte.
	 */
	void writeBlock(
		ST7920::Access &acc,
		const std::uint16_t *start,
		const std::uint16_t *end,
		int pos
	);
	/**
	 * Writes out only the changed portions of the image to the display, and
	 * updates the image in @a frmbuf to match.
	 * @param img  The new image to show.
	 */
	virtual void outputFrame(const duds::hardware::display::BppImage *img);
public:
	/**
	 * Initializes the object with an invalid display size and no pins to use.
	 */
	ST7920();
	/**
	 * Initializes the object with everything required to begin communicating
	 * with the display, but does not initalize the display.
	 * @post  The objects in @a outPins and @a enablePins are moved to internal
	 *        members. They will be the same as default-constructed objects
	 *        after this function returns.
	 * @param outPins    The set of pins used for 4-bit data output and the
	 *                   text flag. This object is moved to an internal member.
	 *                   The pins are, in order:
	 *                    -# Data bit 4
	 *                    -# Data bit 5
	 *                    -# Data bit 6
	 *                    -# Data bit 7
	 *                    -# Text flag; often labeled "RS"
	 * @param enablePin  The chip select used for the enable line on the
	 *                   display. It is often labeled "E".
	 *                   The object is moved to an internal member.
	 * @warning          Only one ST7920 can be used with a
	 *                   @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *                   The other selectable item must not be a ST7920. A
	 *                   logic inverter will not work around this issue.
	 * @param w          The width of the display in pixels. It must not exceed
	 *                   256.
	 * @param h          The height of the display in pixels. It must not exceed
	 *                   64.
	 * @param delay      The number of nanoseconds to delay while the display
	 *                   reads in data. Delays as short as 1000ns should be
	 *                   possible. Specific circuits, especially non-permanent
	 *                   prototypes like ones on breadboards, may require longer
	 *                   delays. The default value seems to work well for such
	 *                   prototypes.
	 * @throw DisplaySizeError   Either the width or height is beyond the
	 *                           supported range.
	 * @throw duds::hardware::interface::PinDoesNotExist
	 * @throw duds::hardware::interface::PinRangeError
	 * @throw duds::hardware::interface::DigitalPinCannotOutputError
	 */
	ST7920(
		duds::hardware::interface::DigitalPinSet &&outPins,
		duds::hardware::interface::ChipSelect &&enablePin,
		unsigned int w,
		unsigned int h,
		std::chrono::nanoseconds delay = std::chrono::nanoseconds(8000)
	);
	/**
	 * Calls off().
	 */
	virtual ~ST7920();
	/**
	 * Sets the pins to use for communicating with the display. After calling
	 * this, initialize() must be called before using the display.
	 * @post  The objects in @a outPins and @a enablePins are moved to internal
	 *        members. They will be the same as default-constructed objects
	 *        after this function returns.
	 * @param outPins    The set of pins used for 4-bit data output and the
	 *                   text flag. This object is moved to an internal member.
	 *                   The pins are, in order:
	 *                    -# Data bit 4
	 *                    -# Data bit 5
	 *                    -# Data bit 6
	 *                    -# Data bit 7
	 *                    -# Text flag; often labeled "RS"
	 * @param enablePin  The chip select used for the enable line on the
	 *                   display. It is often labeled "E".
	 *                   The object is moved to an internal member.
	 * @warning          Only one ST7920 can be used with a
	 *                   @ref duds::hardware::interface::ChipBinarySelectManager "ChipBinarySelectManager".
	 *                   The other selectable item must not be a ST7920. A
	 *                   logic inverter will not work around this issue.
	 * @param w          The width of the display in pixels. It must not exceed
	 *                   256.
	 * @param h          The height of the display in pixels. It must not exceed
	 *                   64.
	 * @param delay      The number of nanoseconds to delay while the display
	 *                   reads in data. Delays as short as 1000ns should be
	 *                   possible. Specific circuits, especially non-permanent
	 *                   prototypes like ones on breadboards, may require longer
	 *                   delays. The default value seems to work well for such
	 *                   prototypes.
	 * @throw DisplaySizeError   Either the width or height is beyond the
	 *                           supported range.
	 * @throw duds::hardware::interface::PinDoesNotExist
	 * @throw duds::hardware::interface::PinRangeError
	 * @throw duds::hardware::interface::DigitalPinCannotOutputError
	 */
	void configure(
		duds::hardware::interface::DigitalPinSet &&outPins,
		duds::hardware::interface::ChipSelect &&enablePin,
		unsigned int w,
		unsigned int h,
		std::chrono::nanoseconds delay = std::chrono::nanoseconds(8000)
	);
	/**
	 * Initializes the display for use. This function must be called before
	 * sending text or any other commands to the display. It may be called
	 * more than once.
	 * @pre   The pins to use have already been configured, and the display
	 *        size has been set.
	 * @post  The display is in the "on" state; on() has been called.
	 * @post  Functions that send data to the display may be used.
	 * @post  The display is blank.
	 * @throw DisplayUninitialized  The display object has not been
	 *                              given any pins to use.
	 */
	void initialize();
	/**
	 * Commands the display to turn off. This should make the display appear
	 * blank, but it does not clear the display's frame buffer. Sending any
	 * command or data to the display will cause it to start displaying its
	 * image again.
	 * @pre  initialize() has been successfully called.
	 * @bug  Name isn't technically correct; change it.
	 */
	void off();
	/**
	 * Commands the display to turn on. There is no need to use this with the
	 * ST7920 LCD controller; any display command will cause the display to
	 * show the contents of its frame buffer. The function exists for
	 * consistency with other display classes.
	 * @pre  initialize() has been successfully called.
	 * @bug  Name isn't technically correct; change it.
	 */
	void on();
};

} } } }
