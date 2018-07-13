/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */

#include <duds/hardware/devices/displays/ST7920.hpp>
#include <duds/hardware/display/DisplayErrors.hpp>
#include <duds/general/ReverseBits.hpp>
#include <duds/general/YieldingWait.hpp>
#include <thread>

namespace duds { namespace hardware { namespace devices { namespace displays {

ST7920::ST7920() : outcfg(5) { }

ST7920::ST7920(
	duds::hardware::interface::DigitalPinSet &&outPins,
	duds::hardware::interface::ChipSelect &&enablePin,
	unsigned int w,
	unsigned int h,
	std::chrono::nanoseconds delay
) :
	duds::hardware::display::BppGraphicDisplay(
		duds::hardware::display::ImageDimensions(w, h)
	),
	outcfg(5),
	soonestSend(std::chrono::high_resolution_clock::now())
{
	configure(std::move(outPins), std::move(enablePin), w, h, delay);
}

ST7920::~ST7920() {
	if (outputs.havePins()) {
		off();
	}
}

void ST7920::configure(
	duds::hardware::interface::DigitalPinSet &&outPins,
	duds::hardware::interface::ChipSelect &&enablePin,
	unsigned int w,
	unsigned int h,
	std::chrono::nanoseconds delay
) {
	// range check on display size
	if ((w > 256) || (w < 16) || (h > 64) || (h < 16)) {
		DUDS_THROW_EXCEPTION(duds::hardware::display::DisplaySizeError() <<
			duds::hardware::display::ImageErrorFrameDimensions(
				duds::hardware::display::ImageDimensions(w, h)
			)
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
	frmbuf.resize(w, h);
	nibblePeriod = delay;
}


void ST7920::wait() const {
	auto remain = soonestSend - std::chrono::high_resolution_clock::now();
	if (remain.count() > 0) {
		duds::general::YieldingWait(remain);
	}
}

void ST7920::preparePins(ST7920::Access &acc) {
	if (!outputs.havePins()) {
		DUDS_THROW_EXCEPTION(duds::hardware::display::DisplayUninitialized());
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

void ST7920::sendByte(ST7920::Access &acc, int val) {
	// write out the text flag as the MSb along with the high-order nibble
	acc.output.write((val & 0x1F0) >> 4);  // 5-bit output
	// wait
	duds::general::YieldingWait(200);
	// tell LCD to read
	acc.enable.select();
	// another wait
	duds::general::YieldingWait(nibblePeriod);
	// LCD should be done reading
	acc.enable.deselect();
	// sending a whole byte?
	if (~val & nibbleFlag) {
		// write out the low-order nibble; leave command flag alone
		acc.output.write(val & 0xF, 4);  // 4-bit output
		// wait
		duds::general::YieldingWait(200);
		// tell LCD to read
		acc.enable.select();
		// wait again
		duds::general::YieldingWait(nibblePeriod);
		// LCD should be done reading
		acc.enable.deselect();
	}
	// record time when more data can be sent
	soonestSend = std::chrono::high_resolution_clock::now();
	if (val < 2) {
		soonestSend += std::chrono::milliseconds(2);
	} else {
		soonestSend += std::chrono::microseconds(78);
	}
}

void ST7920::initialize() {
	// LCD intialization commands in reverse order they are sent
	static const std::uint8_t initdata[10] = {
		0x26,            // use graphic output
		0x24,            // use extended commands
		0x6,             // increment cursor, no display shift
		0xC,             // turn on display w/o cursor
		0x1,             // clear display
		0x8,             // turn off display
		0x20,            // 4 bit bus mode
		0x30,            // 8-bit bus mode; sync nibble reception
		0x30,            // 8-bit bus mode; sync nibble reception
		0x30             // 8-bit bus mode; sync nibble reception
	};
	Access acc;
	preparePins(acc);
	acc.output.output(false);
	acc.enable.select();
	std::this_thread::sleep_for(std::chrono::milliseconds(4));
	acc.enable.deselect();
	int loop = 9;
	for (; loop >= 6; --loop) {
		sendByte(acc, nibbleFlag | initdata[loop]);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	for (; loop >= 0; --loop) {
		sendByte(acc, initdata[loop]);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

void ST7920::off() {
	Access acc;
	preparePins(acc);
	sendByte(acc, 1);  // suspend command
}

void ST7920::on() {
	Access acc;
	preparePins(acc);
	sendByte(acc, 0x26);  // graphic display mode; previously set
}

void ST7920::writeBlock(
	ST7920::Access &acc,
	const std::uint16_t *start,
	const std::uint16_t *end,
	int pos
) {
	// set location
	wait();
	sendByte(acc, (pos & 0x3F) | 0x80);
	wait();
	sendByte(acc, ((pos >> 8) & 0x3F) | 0x80);
	do {
		// BppImage and the display use the opposite ordering of bits
		std::uint16_t out = duds::general::ReverseBits(*start);
		// write out the image data
		wait();
		sendByte(acc, textFlag | (out >> 8));
		wait();
		sendByte(acc, textFlag | (out & 0xFF));
	} while (start++ != end);
}

void ST7920::outputFrame(const duds::hardware::display::BppImage *img) {
	Access acc;
	preparePins(acc);
	// width in 16-bit ints
	int wblk = width() / 16 + (((width() % 16) > 0) ? 1 : 0);
	// loop through the images
	for (int h = 0; h < height(); ++h) {
		// pointers to two-byte ints of image data; display works with 2 bytes
		std::uint16_t *dpix = (std::uint16_t*)frmbuf.bufferLine(h);     // dest
		const std::uint16_t *spix = (std::uint16_t*)img->bufferLine(h); // src
		std::uint16_t *cx = nullptr, *lx; // first and last changed X coordinate
		int spos; // starting location of change
		for (int w = 0; w < wblk; ++dpix, ++spix, ++w) {
			// find differences between frame buffer and new image
			if (*dpix != *spix) {
				// record new value
				*dpix = *spix;
				// no change pending?
				if (!cx) {
					// mark this change
					cx = lx = dpix;
					spos = w;
				}
				// more than 1 spot (two bytes) since last change?
				else if (lx < (dpix - 1)) {
					// send pending change
					writeBlock(acc, cx, lx, h | (spos << 8));
					// mark new change
					cx = lx = dpix;
					spos = w;
				}
				// add to pending change
				else {
					lx = dpix;
				}
			}
		}
		// have pending change?
		if (cx) {
			// send pending change
			writeBlock(acc, cx, lx, h | (spos << 8));
		}
	}
}

} } } }
