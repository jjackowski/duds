/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */

#include <duds/hardware/devices/displays/SimulatedBppDisplay.hpp>
#include <duds/hardware/display/DisplayErrors.hpp>
#include <duds/general/Errors.hpp>
#include <iostream>

namespace duds { namespace hardware { namespace devices { namespace displays {

SimulatedBppDisplay::SimulatedBppDisplay() { }

SimulatedBppDisplay::SimulatedBppDisplay(
	const duds::ui::graphics::ImageDimensions &id
) : duds::hardware::display::BppGraphicDisplay(id)
{ }  // all done above

SimulatedBppDisplay::~SimulatedBppDisplay() {
	for (int h = height() + 3; h > 0; --h) {
		std::cout << '\n';
	}
}

void SimulatedBppDisplay::configure(const duds::ui::graphics::ImageDimensions &id) {
	// range check on display size
	if (id.empty()) {
		DUDS_THROW_EXCEPTION(duds::hardware::display::DisplaySizeError() <<
			duds::ui::graphics::ImageErrorDimensions(id)
		);
	}
	frmbuf.resize(id);
}

void SimulatedBppDisplay::outputFrame(const duds::ui::graphics::BppImage *img) {
	// output border
	std::cout << "\n*";
	for (int w = width(); w > 0; --w) {
		std::cout << '-';
	}
	std::cout << "*\n";
	// output image
	for (int h = 0; h < height(); ++h) {
		// start border
		std::cout << '|';
		// initialize pointer to the start of the line
		const duds::ui::graphics::BppImage::PixelBlock *pb = img->bufferLine(h);
		// setup the mask for the bit to check
		duds::ui::graphics::BppImage::PixelBlock mask = 1;
		// go through the whole width
		for (int w = width(); w > 0; --w) {
			// check for set pixel
			if (*pb & mask) {
				std::cout << 'X';
			} else {
				std::cout << ' ';
			}
			// advance to next pixel
			mask <<= 1;
			// gone past the end of the pixel block?
			if (!mask) {
				// advance to the next block
				mask = 1;
				++pb;
			}
		}
		// end border
		std::cout << "|\n";
	}
	// output border
	std::cout << '*';
	for (int w = width(); w > 0; --w) {
		std::cout << '-';
	}
	std::cout << '*' << std::endl;
	// return cursor to top
	std::cout << "\033[" << height() + 3 << 'A';
}

} } } }
