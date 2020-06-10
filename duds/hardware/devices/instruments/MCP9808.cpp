/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/MCP9808.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/hardware/interface/SmbusErrors.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

MCP9808::MCP9808(std::unique_ptr<duds::hardware::interface::Smbus> &c) {
	// attempt to confirm that this is a MCP9808
	std::int16_t chk = c->receiveWordBe(6);
	if (chk != 0x54) {
		// wrong response
		DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
			duds::hardware::interface::SmbusDeviceAddr(com->address())
		);
	}
	chk = c->receiveWordBe(7);
	if ((chk & 0xFF00) != 0x400) {
		// wrong response
		DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
			duds::hardware::interface::SmbusDeviceAddr(com->address())
		);
	}
	// keep revision
	rev = chk & 0xFF;
	// get the resolution
	res = c->receiveByte(8);
	// get the current configuration
	config = c->receiveWordBe(1);
	// only take the communicator if no exception is thrown
	com = std::move(c);
}

MCP9808::~MCP9808() {
	suspend();
}

void MCP9808::start() {
	config &= 0x6FF;
	com->transmitWordBe(1, config);
}

void MCP9808::suspend() {
	config |= 0x100;
	com->transmitWordBe(1, config);
}

void MCP9808::resolution(Resolution r) {
	res = r & 3;
	com->transmitByte(8, res);
}

double MCP9808::resolutionDegrees() const {
	switch (res) {
		case Half:
			return 0.5;
		case Quarter:
			return 0.25;
		case Eighth:
			return 0.125;
		case Sixteenth:
			return 0.0625;
	}
	// should never happen
	return 0;
}

void MCP9808::sample() {
	std::int16_t samp = com->receiveWordBe(5);
	temp = ((double)duds::general::SignExtend<13>(samp)) / 16.0 + 273.15;
}

duds::data::Quantity MCP9808::temperature() const {
	return duds::data::Quantity(temp, duds::data::units::Kelvin);
}

} } } }
