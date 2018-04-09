/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/AMG88xx.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>
#include <duds/general/SignedMagnitudeToTwosComplement.hpp>
#include <thread>

namespace duds { namespace hardware { namespace devices { namespace instruments {

AMG88xx::AMG88xx(std::unique_ptr<duds::hardware::interface::I2c> &c) :
com(std::move(c)), mode(Sleep), fps1Not10(0), misid(0) {
	try {
		duds::hardware::interface::Conversation reset;
		// normal operating mode
		reset.addOutputVector() << (std::int16_t)0;
		com->converse(reset);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		reset.clear();
		reset.addOutputVector() << (std::int8_t)0;
		reset.addInputVector(1);
		com->converse(reset);
		duds::hardware::interface::ConversationExtractor ex(reset);
		std::int8_t mode;
		ex >> mode;
		if (mode != 0) {
			DUDS_THROW_EXCEPTION(DeviceMisidentified());
		}
		reset.clear();
		// reset device
		reset.addOutputVector() << (std::int8_t)1 << (std::int8_t)0x3F;
		com->converse(reset);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		reset.clear();
		// sleep
		reset.addOutputVector() << (std::int8_t)0 << (std::int8_t)0x10;
		com->converse(reset);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	} catch (...) {
		c = std::move(com);
		throw;
	}
	read.addOutputVector() << (std::int8_t)0x0E;
	read.addInputVector(2);
	(read.addOutputVector() << (std::int8_t)0x80).breakBefore();
	read.addInputVector(128);
}

AMG88xx::~AMG88xx() {
	if (!misid) {
		suspend();
	}
}

void AMG88xx::configure(bool fps1) {
	// only act on a change
	if (fps1 != (fps1Not10 > 0)) {
		// value actually a bit, so this will toggle
		++fps1Not10;
		// if device is not sleeping . . .
		if (mode != Sleep) {
			// change the frame rate now
			duds::hardware::interface::Conversation frate;
			frate.addOutputVector() << (std::int8_t)2 << (std::int8_t)fps1Not10;
			com->converse(frate);
		}
	}
}

void AMG88xx::start() {
	duds::hardware::interface::Conversation go;
	// normal operating mode
	go.addOutputVector() << (std::int16_t)0;
	com->converse(go);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	go.clear();
	go.addOutputVector() << (std::int8_t)0;
	go.addInputVector(1);
	try {
		com->converse(go);
		duds::hardware::interface::ConversationExtractor ex(go);
		std::int8_t mode;
		ex >> mode;
		if (mode != 0) {
			misid = 1;
			DUDS_THROW_EXCEPTION(DeviceMisidentified());
		}
	} catch (...) {
		misid = 1;
		throw;
	}
	misid = 0;
	go.clear();
	// configure frame rate
	go.addOutputVector() << (std::int8_t)2 << (std::int8_t)fps1Not10;
	com->converse(go);
	//std::this_thread::sleep_for(std::chrono::milliseconds(50));
	mode = Normal;
}

void AMG88xx::suspend() {
	duds::hardware::interface::Conversation stop;
	stop.addOutputVector() << (std::int8_t)0 << (std::int8_t)0x10;
	com->converse(stop);
	mode = Sleep;
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void AMG88xx::sample() {
	com->converse(read);
	duds::hardware::interface::ConversationExtractor ex(read);
	std::int16_t t;
	ex >> t;
	temp = duds::general::SignedMagnitudeToTwosComplement<12>(t);
	double *pixel = &(img[0][0]);
	for (int c = 64; c > 0; --c, ++pixel) {
		std::int16_t val;
		ex >> val;
		val = duds::general::SignExtend<12>(val);
		*pixel = (double)val / 4.0 + 273.15;
	}
}

duds::data::Quantity AMG88xx::temperature() const {
	return duds::data::Quantity(
		(double)temp / 16.0 + 273.15,
		duds::data::units::Kelvin
	);
}

} } } }
