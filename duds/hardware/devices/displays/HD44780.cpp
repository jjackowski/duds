/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <thread>

namespace duds { namespace hardware { namespace devices { namespace displays {

HD44780::HD44780() : outcfg(5) { }

HD44780::HD44780(
	duds::hardware::interface::DigitalPinSet &outPins,
	duds::hardware::interface::ChipSelect &enablePin,
	unsigned int c,
	unsigned int r
) :
	TextDisplay(c, r),
	outcfg(5),
	soonestSend(std::chrono::high_resolution_clock::now())
{
	configure(outPins, enablePin, c, r);
}

HD44780::~HD44780() {
	off();
}

void HD44780::configure(
	duds::hardware::interface::DigitalPinSet &outPins,
	duds::hardware::interface::ChipSelect &enablePin,
	unsigned int c,
	unsigned int r
) {
	// range check on display size
	if ((c > 20) || (c < 1) || (r > 4) || (r < 1)) {
		DUDS_THROW_EXCEPTION(TextDisplayRangeError() <<
			TextDisplaySizeInfo(Info_DisplayColRow(c, r))
		);
	}
	if (!outPins.havePins() || !enablePin) {
		DUDS_THROW_EXCEPTION(duds::hardware::interface::PinDoesNotExist());
	}
	// requires 5 pins: 4 data, 1 control, all output
	if (outPins.size() != 5) {
		DUDS_THROW_EXCEPTION(duds::hardware::interface::PinRangeError());
	}
	// get the capabilities for inspection
	std::vector<duds::hardware::interface::DigitalPinCap> caps =
		outPins.capabilities();
	// iteratate over all the pins
	std::vector<duds::hardware::interface::DigitalPinCap>::const_iterator iter =
		caps.cbegin();
	unsigned int pos = 0;
	for (; iter != caps.cend(); ++pos, ++iter) {
		// check for no output ability
		if (!iter->canOutput()) {
			DUDS_THROW_EXCEPTION(
				duds::hardware::interface::DigitalPinCannotOutputError() <<
				duds::hardware::interface::PinErrorId(outPins.globalId(pos))
			);
		}
		// work out the actual output config
		outcfg[pos] = duds::hardware::interface::DigitalPinConfig(
			iter->firstOutputDriveConfigFlags()
		);
	}
	// store this access object; seems good if it got this far without
	// an exception
	outputs = std::move(outPins);
	enable = std::move(enablePin);
	columnsize = c;
	rowsize = r;
}

void HD44780::wait() const {
	auto remain = soonestSend - std::chrono::high_resolution_clock::now();
	if (remain.count() > 0) {
		std::this_thread::sleep_for(remain);
	}
}

void HD44780::preparePins(HD44780::Access &acc) {
	if (!outputs.havePins()) {
		DUDS_THROW_EXCEPTION(TextDisplayUninitialized());
	}
	// Wait until the display can take more data. Do this before getting
	// hardware access so that the hadrware remains availble for other
	// threads during this initial wait.
	wait();
	// request hardware access
	outputs.access(acc.output);
	enable.access(acc.enable);
	// make all pins outputs
	acc.output.modifyConfig(outcfg);
}

void HD44780::sendByte(HD44780::Access &acc, int val) {
	// write out the text flag as the MSb along with the high-order nibble
	acc.output.write((val & 0x1F0) >> 4);  // 5-bit output
	// wait
	std::this_thread::sleep_for(std::chrono::nanoseconds(250));
	// tell LCD to read
	acc.enable.select();
	// another wait
	std::this_thread::sleep_for(std::chrono::nanoseconds(250));
	// LCD should be done reading
	acc.enable.deselect();
	// sending a whole byte?
	if (~val & nibbleFlag) {
		// write out the low-order nibble; leave command flag alone
		acc.output.write(val & 0xF, 4);  // 4-bit output
		// wait
		std::this_thread::sleep_for(std::chrono::nanoseconds(250));
		// tell LCD to read
		acc.enable.select();
		// wait again
		std::this_thread::sleep_for(std::chrono::nanoseconds(250));
		// LCD should be done reading
		acc.enable.deselect();
	}
	// record time when more data can be sent
	soonestSend = std::chrono::high_resolution_clock::now();
	if (val < 4) {
		soonestSend += std::chrono::milliseconds(4);
	} else {
		soonestSend += std::chrono::microseconds(48);
	}
}

void HD44780::initialize() {
	// LCD intialization commands in reverse order they are sent
	static const std::uint8_t initdata[8] = {
		0x6,             // increment cursor, no display shift
		0xC,             // turn on display w/o cursor
		0x1,             // clear display
		0x8,             // turn off display
		0x20,            // 4 bit bus mode, 1 row
		0x30,            // 8-bit bus mode; sync nibble reception
		0x30,            // 8-bit bus mode; sync nibble reception
		0x30             // 8-bit bus mode; sync nibble reception
	};
	Access acc;
	preparePins(acc);
	acc.output.output(false);
	acc.enable.select();
	std::this_thread::sleep_for(std::chrono::milliseconds(8));
	acc.enable.deselect();
	int loop = 7;
	for (; loop >= 4; --loop) {
		sendByte(acc, nibbleFlag | initdata[loop]);
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
	}
	// now using 4-bit bus mode for one row only; are more rows in use?
	if (rowsize > 1) {
		// configure for multiple rows; use 4-bit bus mode
		sendByte(acc, 0x28);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(4));
	for (; loop >= 0; --loop) {
		sendByte(acc, initdata[loop]);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	// cursor should now be at the upper left corner
	cpos = rpos = 0;
}

void HD44780::off() {
	Access acc;
	preparePins(acc);
	sendByte(acc, 8);  // display off command
}

void HD44780::on() {
	Access acc;
	preparePins(acc);
	sendByte(acc, 12);  // display on command
}

/**
 * The address used by the display for the start of each row.
 */
static std::uint8_t rowStartAddr[4] = {
	0, 0x40, 0x14, 0x54
};

void HD44780::moveImpl(unsigned int c, unsigned int r) {
	Access acc;
	preparePins(acc);
	// send command to change display address or'd with the address
	sendByte(acc, 0x80 | (rowStartAddr[r] + c));
}

void HD44780::writeImpl(Access &acc, const std::string &text) {
	// loop through characters
	std::string::const_iterator iter = text.begin();
	do {
		wait();
		// send the lower byte of the next character; discard the rest
		sendByte(acc, textFlag | (*iter & 0xFF));
		// need to reposition cursor?
		if (advance()) {
			wait();
			// send command to change display address or'd with the address
			// of the current position
			sendByte(acc, 0x80 | (rowStartAddr[rpos] + cpos));
		}
	} while (++iter != text.end());
}

void HD44780::writeImpl(int c) {
	Access acc;
	preparePins(acc);
	// send the lower byte of the character; discard the rest
	sendByte(acc, textFlag | (c & 0xFF));
}

void HD44780::writeImpl(const std::string &text) {
	Access acc;
	preparePins(acc);
	writeImpl(acc, text);
}

void HD44780::writeImpl(
	const std::string &text,
	unsigned int c,
	unsigned int r
) {
	Access acc;
	preparePins(acc);
	// move cursor
	sendByte(acc, 0x80 | (rowStartAddr[r] + c));
	cpos = c;
	rpos = r;
	// do the write
	writeImpl(acc, text);
}

void HD44780::clear() {
	Access acc;
	preparePins(acc);
	// send clear command
	sendByte(acc, 1);
	// display moves the cursor; update the position to match
	cpos = rpos = 0;
}

} } } }
