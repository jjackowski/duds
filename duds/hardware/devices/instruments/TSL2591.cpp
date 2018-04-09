/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/TSL2591.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Prevent Doxygen from mixing this up with other source-file specific items
 * from other files.
 * @internal
 */
namespace TSL2591_internal {

/**
 * The device's registers. These really do not fill a byte.
 */
enum Regs {
	RegEnable,
	RegControl, // called config in the doc's list of regs, but control in detail
	RegDeviceId = 0x12,
	RegStatus,
	RegCh0,
	RegCh1      = 0x16
};

/**
 * The flags refered to as commands in the documentation.
 */
enum CmdFlags {
	Cmd        = 0x80,  // used for all command bytes
	TransNorm  = 0x20,
	TransSpec  = 0x60,
	AddrMask   = 0x1F
};

/**
 * Flags used to enable various features.
 */
enum EnableFlags {
	OscOn      = 1,
	Sample     = 2,
	IntEnable  = 0x10,
	SleepOnInt = 0x40,
	NonPersistantInterrutEnable = 0x80  // really don't know what to call it
};

/**
 * More configuration flags.
 */
enum ControlFlags {
	IntgrTimeShift = 0,
	IntgrTimeMask  = 0x7,
	GainShift      = 4,
	GainMask       = 0x30,
	Reset          = 0x80
};

/**
 * The set of selectable gain factors.
 */
static std::uint16_t gainSettings[4] = {
	1,
	25,
	428,
	9876
};

}

using namespace TSL2591_internal;

TSL2591::TSL2591(std::unique_ptr<duds::hardware::interface::I2c> &c) :
com(std::move(c)), scale(0) {
	try {
		duds::hardware::interface::Conversation firstcon;
		// verify ID
		firstcon.addOutputVector() << (std::int8_t)(Cmd|TransNorm|RegDeviceId);
		firstcon.addInputVector(1);
		com->converse(firstcon);
		duds::hardware::interface::ConversationExtractor ex(firstcon);
		std::int8_t id;
		ex >> id;
		if (id != 0x50) {
			DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
				duds::hardware::interface::I2cDeviceAddr(com->address())
			);
		}
		// attempt reset
		firstcon.clear();
		firstcon.addOutputVector() << (std::int8_t)(Cmd|TransNorm|RegControl) <<
			(std::int8_t)(Reset);
		try {
			// the reset causes the device to not ack the message
			com->converse(firstcon);
		} catch (duds::hardware::interface::I2cErrorNoDevice &) {
			// bother; ignore it
		} catch (...) {
			// could be a real issue, but should be unlikely to occur on the second
			// conversation with the device
			throw;
		}
		// make conversation to read input so it can be reused
		// address -- green
		input.addOutputVector() << (std::int8_t)(Cmd|TransNorm|RegCh0);
		// read in 4 bytes
		input.addInputVector(4);
	} catch (...) {
		// move the I2C communicator back
		c = std::move(com);
		throw;
	}
}

TSL2591::~TSL2591() {
	suspend();
}

void TSL2591::init(int gain, int integration) {
	if ((gain < 0) || (gain > 3)) {
		DUDS_THROW_EXCEPTION(TSL2591BadGain());
	}
	if (integration >= 100) {
		integration = (integration / 100) - 1;
	}
	if ((integration < 0) || (integration > 5)) {
		DUDS_THROW_EXCEPTION(TSL2591BadIntegration());
	}
	// remake the initialization conversation
	initialize.clear();
	initialize.addOutputVector() <<
		// command to start writing at enable register
		(std::int8_t)(Cmd|TransNorm|RegEnable) <<
		// make it go
		(std::int8_t)(OscOn|Sample) <<
		// set gain and integration time
		(std::int8_t)(integration | (gain << GainShift));
	com->converse(initialize);
	// datasheet says the values are for the 100ms integration period
	scale = (double)(integration + 1);
	// datasheet says the values are for maximum gain
	if (gain < 3) {
		scale *= (double)gainSettings[gain] / (double)gainSettings[3];
	}
}

void TSL2591::suspend() {
	duds::hardware::interface::Conversation conv;
	conv.addOutputVector() <<
		// command to start writing at enable register
		(std::int8_t)(Cmd|TransNorm|RegEnable) <<
		// make it stop
		(std::int8_t)(0);
	com->converse(conv);
}

void TSL2591::resume() {
	if (initialize.empty()) {
		DUDS_THROW_EXCEPTION(DeviceUninitalized());
	}
	com->converse(initialize);
}

void TSL2591::sample() {
	// get input
	com->converse(input);
	// parse input
	duds::hardware::interface::ConversationExtractor ex(input);
	ex >> broad >> ir;
}

duds::data::Quantity TSL2591::brightness() const {
	// datasheet uses uW/cm2, which is (0.01)W/m2
	// this channel has 6024 counts per uW/cm2 for white light
	return duds::data::Quantity(
		(double)broad * scale * 60.24,
		duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		)
	);
}

duds::data::Quantity TSL2591::brightnessIr() const {
	// datasheet uses uW/cm2, which is (0.01)W/m2
	// this channel has 3474 counts per uW/cm2 for IR light
	return duds::data::Quantity(
		(double)ir * scale * 34.74,
		duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		)
	);
}

} } } }
