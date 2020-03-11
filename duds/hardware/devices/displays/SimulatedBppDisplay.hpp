/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/hardware/display/BppGraphicDisplay.hpp>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Writes text, expecting a typical Linux terminal, to simulate a bit-per-pixel
 * graphic display. Intended for testing.
 *
 * @author  Jeff Jackowski
 */
class SimulatedBppDisplay : public duds::hardware::display::BppGraphicDisplay {
	/**
	 * Writes out only the changed portions of the image to the display, and
	 * updates the image in @a frmbuf to match.
	 * @param img  The new image to show.
	 */
	virtual void outputFrame(const duds::ui::graphics::BppImage *img);
public:
	/**
	 * Creates the object with an invalid display size.
	 */
	SimulatedBppDisplay();
	/**
	 * Initializes the object to a usable state.
	 * @param id  The dimensions of the display in "pixels" (really characters).
	 *            It will work poorly if it doesn't fit on the display.
	 */
	SimulatedBppDisplay(const duds::ui::graphics::ImageDimensions &id);
	/**
	 * Initializes the object to a usable state.
	 * @param w          The width of the display in "pixels" (really
	 *                   characters). It will work poorly if it doesn't fit on
	 *                   the display.
	 * @param h          The height of the display in "pixels" (really
	 *                   characters). It will work poorly if it doesn't fit on
	 *                   the display.
	 */
	SimulatedBppDisplay(unsigned int w, unsigned int h) :
	SimulatedBppDisplay(duds::ui::graphics::ImageDimensions(w, h)) { }
	/**
	 * Positions cursor after expected end of display text.
	 */
	virtual ~SimulatedBppDisplay();
	/**
	 * Initializes the object to a usable state.
	 * @param id  The dimensions of the display in "pixels" (really characters).
	 *            It will work poorly if it doesn't fit on the display.
	 * @throw DisplaySizeError   Either the width or height is zero.
	 */
	void configure(const duds::ui::graphics::ImageDimensions &id);
	/**
	 * Initializes the object to a usable state.
	 * @param w          The width of the display in "pixels" (really
	 *                   characters). It will work poorly if it doesn't fit on
	 *                   the display.
	 * @param h          The height of the display in "pixels" (really
	 *                   characters). It will work poorly if it doesn't fit on
	 *                   the display.
	 * @throw DisplaySizeError   Either the width or height is zero.
	 */
	void configure(unsigned int w, unsigned int h) {
		configure(duds::ui::graphics::ImageDimensions(w, h));
	}
};

} } } }
