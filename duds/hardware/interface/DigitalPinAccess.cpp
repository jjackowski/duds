/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinAccess.hpp>

namespace duds { namespace hardware { namespace interface {

DigitalPinAccess &DigitalPinAccess::operator=(
	DigitalPinAccess &&old
) {
	// lose access to pin
	retire();
	// update the port object of the old object
	if (old.havePin()) {
		old.port()->updateAccess(old, this);
	}
	// take the old object's data
	DigitalPinAccessBase::operator=(std::move(old));
	gid = old.gid;
	return *this;
}

void DigitalPinAccess::retire() noexcept {
	if (havePin()) {
		port()->updateAccess(*this, nullptr);
		reset();
	}
}

} } }
