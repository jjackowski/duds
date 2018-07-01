/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinSetAccess.hpp>

namespace duds { namespace hardware { namespace interface {

void DigitalPinSetAccess::reserveAdditional(unsigned int len) {
	unsigned int total = (unsigned int)pinvec.size() + len;
	if (total > pinvec.capacity()) {
		pinvec.reserve(total);
	}
}

DigitalPinSetAccess &DigitalPinSetAccess::operator=(
	DigitalPinSetAccess &&old
) noexcept {
	// lose access to pin
	retire();
	// update the port object of the old object
	if (old.havePins()) {
		old.port()->updateAccess(old, this);
	}
	// take the old object's data
	DigitalPinAccessBase::operator=(std::move(old));
	pinvec = std::move(old.pinvec);
	return *this;
}

void DigitalPinSetAccess::retire() noexcept {
	if (havePins()) {
		port()->updateAccess(*this, nullptr);
		pinvec.clear();
		reset();
	}
}

std::vector<unsigned int> DigitalPinSetAccess::subset(
	const std::vector<unsigned int> &pos
) const {
	std::vector<unsigned int> pins;
	pins.reserve(pos.size());
	std::vector<unsigned int>::const_iterator iter = pos.cbegin();
	for (; iter != pos.cend(); ++iter) {
		if ((*iter != -1) && (*iter >= pinvec.size())) {
			DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
				PinErrorId(globalId(*iter))
			);
		}
		pins.push_back(pinvec[*iter]);
	}
	return pins;
}

void DigitalPinSetAccess::modifyConfig(const DigitalPinConfig &conf) const {
	std::vector<DigitalPinConfig> c(pinvec.size(), conf);
	port()->modifyConfig(pinvec, c, &portdata);
}

void DigitalPinSetAccess::output(bool state) const {
	std::vector<bool> s(pinvec.size(), state);
	port()->output(pinvec, s, &portdata);
}

} } }
