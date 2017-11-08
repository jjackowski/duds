/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/INA219.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

INA219::INA219(
	std::unique_ptr<duds::hardware::interface::Smbus> &c,
	double s
) : shunt(s) {
	// reset device
	c->transmitWordBe(0, 0x8000);
	c->transmitWordBe(0, 0x1FFF);  // 128 samples, 16v bus
	// only take the communicator if no exception is thrown
	com = std::move(c);
}

INA219::~INA219() {
	if (com) {
		// put device to sleep; set widest ranges in case that is good
		com->transmitWordBe(0, 0x3998);
	}
}

duds::data::Quantity INA219::maxCurrent() const {
	return duds::data::Quantity(0.32 / shunt, duds::data::units::Ampere);
}

duds::data::Quantity INA219::shuntResistance() const {
	return duds::data::Quantity(shunt, duds::data::units::Ohm);
}

duds::data::Quantity INA219::shuntVoltage() const {
	return duds::data::Quantity(((double)shuntV) * 1e-5, duds::data::units::Volt);
}

duds::data::Quantity INA219::busVoltage() const {
	return duds::data::Quantity(((double)busV) * 4e-3, duds::data::units::Volt);
}

duds::data::Quantity INA219::busCurrent() const {
	return shuntVoltage() / shuntResistance();
}

void INA219::sample() {
	shuntV = com->receiveWordBe(1);
	busV = (std::int16_t)(com->receiveWordBe(2)) >> 3;
}

} } } }
