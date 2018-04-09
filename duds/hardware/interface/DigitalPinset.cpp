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

namespace duds { namespace hardware { namespace interface {

DigitalPinSet::DigitalPinSet(
	const std::shared_ptr<DigitalPort> &port,
	const std::vector<unsigned int> pvec
) : DigitalPinBase(port), pinvec(pvec) {
	// Have a port?
	if (port) {
		// check for pin non-existence
		std::vector<unsigned int>::const_iterator iter = pinvec.cbegin();
		for (; iter != pinvec.cend(); ++iter) {
			if ((*iter != -1) && !port->exists(*iter)) {
				// bad pin
				DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
					PinErrorId(*iter) << DigitalPortAffected(port.get())
				);
			}
		}
	}
}

} } } // namespaces
