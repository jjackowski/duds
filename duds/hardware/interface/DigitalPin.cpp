/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPin.hpp>

namespace duds { namespace hardware { namespace interface {

DigitalPin::DigitalPin(
	const std::shared_ptr<DigitalPort> &port,
	unsigned int pin
) : DigitalPinBase(port), gid(pin) {
	// Have a port? Then check for pin non-existence.
	if (port && !port->exists(pin)) {
		// bad pin
		DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
			PinErrorId(gid) << DigitalPortAffected(port.get())
		);
	};
}

} } } // namespaces
