/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/ISL29125.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

ISL29125::ISL29125(std::unique_ptr<duds::hardware::interface::I2c> &c) :
com(std::move(c)) {
	// make conversation to read input so it can be reused
	// address -- green
	input.addOutputVector() << (std::int8_t)9;
	// read in 6 bytes
	input.addInputVector(6);
}

ISL29125::~ISL29125() {
	suspend();
}

void ISL29125::init(bool wide) {
	// remake the initialization conversation
	initialize.clear();
	initialize.addOutputVector() <<
		// address -- config
		(std::int8_t)1 <<
		// config byte -- run, set range
		(std::int8_t)(0x5 | (wide ? 0x8 : 0));
	com->converse(initialize);
}

void ISL29125::suspend() {
	duds::hardware::interface::Conversation conv;
	conv.addOutputVector() <<
		// address -- config
		(std::int8_t)1 <<
		// config byte -- stop
		(std::int8_t)0;
	com->converse(conv);
}

void ISL29125::resume() {
	if (initialize.empty()) {
		DUDS_THROW_EXCEPTION(DeviceUninitalized());
	}
	com->converse(initialize);
}

void ISL29125::sample() {
	// get input
	com->converse(input);
	// parse input
	duds::hardware::interface::ConversationExtractor ex(input);
	// the device uses a peculiar ordering
	ex >> g >> r >> b;
}

} } } }
