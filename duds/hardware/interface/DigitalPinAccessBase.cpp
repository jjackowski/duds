/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinAccessBase.hpp>

namespace duds { namespace hardware { namespace interface {

DigitalPinAccessBase &DigitalPinAccessBase::operator=(
	DigitalPinAccessBase &&old
) noexcept {
	// grab the port pointer
	dp = old.dp;
	// clear the port pointer from the source
	old.dp = nullptr;
	// grab the port specific data
	portdata = old.portdata;
	return *this;
}

} } }
