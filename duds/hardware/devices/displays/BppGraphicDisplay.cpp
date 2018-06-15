/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/displays/BppGraphicDisplay.hpp>
#include <duds/hardware/devices/displays/DisplayErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace devices { namespace displays {

void BppGraphicDisplay::write(const BppImage *img) {
	// supplied image must match the frame buffer's size
	if (img->dimensions() != frmbuf.dimensions()) {
		DUDS_THROW_EXCEPTION(DisplaySizeError() <<
			ImageErrorFrameDimensions(frmbuf.dimensions()) <<
			ImageErrorDimensions(img->dimensions())
		);
	}
	outputFrame(img);
}

} } } }
