/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/APDS9301.hpp>
#include <duds/hardware/interface/SmbusErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Prevent Doxygen from mixing this up with other source-file specific items
 * from other files.
 * @internal
 */
namespace APDS9301_internal {

/**
 * The device's registers.
 */
enum Regs {
	/**
	 * Control register. It is 0 when suspended, and 3 when operating. It can
	 * be read to verify operation and communication.
	 */
	RegControl,
	/**
	 * What the documentation calls timing register; really a general
	 * configuration register.
	 */
	RegConfig,
	/**
	 * Interrupt control; set to zero if not using.
	 */
	RegInt =  6,
	/**
	 * Channel 0: sensitive to full visible and near IR spectrum.
	 */
	RegCh0 = 0xC,
	/**
	 * Channel 1: sensitive to near IR spectrum.
	 */
	RegCh1 = 0xE
};

/**
 * The flags refered to as commands in the documentation.
 */
enum CmdFlags {
	Cmd        = 0x80, /**< Always set in command byte. */
	ClearInt   = 0x40, /**< Clear interrupt flags. */
	Word       = 0x20, /**< Transfer two bytes instead of one. */
	AddrMask   = 0x0F  /**< Spot for the address of register to read or write. */
};

enum ConfigFlags {
	/**
	 * Use a 16x gain.
	 */
	HighGain           = 0x10,
	/**
	 * Set to start manual integration, and clear to stop.
	 * Documentation ommits time taken by analog to digital conversion.
	 */
	Intgrate           = 0x08,
	InegrateTimeManual =    3,
	InegrateTime402ms  =    2,
	InegrateTime101ms  =    1,
	InegrateTime13ms7  =    0
};

/**
 * Information on a particular sampling rate.
 */
struct PeriodData {
	/**
	 * A scalar applied to change the irradiance result based on the sampling
	 * period. This is useful because the documentation only has figures
	 * relating the results from the 101ms sampling period to physical values.
	 * Annoyingly, illuminance is specified for 402ms instead of keeping
	 * things consistent.
	 */
	float scale;
	/**
	 * The value given to the device to select the sampling period.
	 */
	int regval;
	/**
	 * The maximum value the device will report for the sampling period. If the
	 * result equals this value, the input was too bright to measure.
	 */
	int maxcnt;
};

/**
 * Integration period values.
 */
static const std::map<float, PeriodData> IntegPeriodVals = {
	{  13.7 * 0.001, { 101.0 / 13.7, 0, 5047 } },
	{ 101.0 * 0.001, { 1.0, 1, 37177 } },
	{ 402.0 * 0.001, { 101.0 / 402.0, 2, 65535 } }
};

}

using namespace APDS9301_internal;

APDS9301::APDS9301(std::unique_ptr<duds::hardware::interface::Smbus> &c) :
com(std::move(c)), actualPeriod(-1) {
	try {
		// suspend operation
		suspend();
		// clear and disable interrupts
		com->transmitByte(Cmd | ClearInt | RegInt, 0);
	} catch (...) {
		// return the communicator if exception is thrown
		c = std::move(com);
		throw;
	}
}

APDS9301::~APDS9301() {
	// suspend operation
	com->transmitByte(Cmd | RegControl, 0);
	// don't bother to check result; can't be throwing exceptions
	// from destructors
}

void APDS9301::init(float integration, bool hg) {
	std::map<float, PeriodData>::const_iterator iter =
		IntegPeriodVals.upper_bound(integration);
	if (iter == IntegPeriodVals.cend()) {
		DUDS_THROW_EXCEPTION(APDS9301BadIntegration());
	}
	actualPeriod = iter->first;
	integTime = iter->second.regval;
	scale = iter->second.scale;
	// documentation specifies values for high gain operation
	if (!(hGain = hg)) {
		// account for low gain
		scale *= 1.0f / 16.0f;
	}
	com->transmitByte(Cmd | RegConfig, (hGain << 4) | integTime);
}

void APDS9301::startOrStop(int val) {
	// start or stop operation
	com->transmitByte(Cmd | RegControl, val);
	// read back power value
	if (com->receiveByte(Cmd | RegControl) != val) {
		// wrong response
		DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
			duds::hardware::interface::SmbusDeviceAddr(com->address())
		);
	}
}

void APDS9301::suspend() {
	startOrStop(0);
}

void APDS9301::resume() {
	if (actualPeriod < 0) {
		// not yet initialized
		DUDS_THROW_EXCEPTION(DeviceUninitalized());
	}
	startOrStop(3);
}

void APDS9301::sample() {
	// get input
	broad = com->receiveWord(Cmd | Word | RegCh0);
	ir = com->receiveWord(Cmd | Word | RegCh1);
}

duds::data::Quantity APDS9301::maxIrradiance() const {
	std::map<float, PeriodData>::const_iterator iter =
		IntegPeriodVals.find(actualPeriod);
	assert(iter != IntegPeriodVals.end());
	return duds::data::Quantity(
		(double)iter->second.maxcnt * scale * 0.275,
		duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		)
	);
}

duds::data::Quantity APDS9301::irradiance() const {
	/** @todo  Avoid a map lookup. */
	std::map<float, PeriodData>::const_iterator iter =
		IntegPeriodVals.find(actualPeriod);
	assert(iter != IntegPeriodVals.end());
	double val;
	if (iter->second.maxcnt <= broad) {
		val = std::numeric_limits<double>::infinity();
	} else {
		val = (double)broad;
	}
	// datasheet uses uW/cm2, which is (0.01)W/m2
	// this channel has 27.5 counts per uW/cm2 for red (640um) light over 101ms
	return duds::data::Quantity(
		val * scale * 0.275,
		duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		)
	);
}

duds::data::Quantity APDS9301::irradianceIr() const {
	// datasheet uses uW/cm2, which is (0.01)W/m2
	// this channel has 5.5 counts per uW/cm2 for IR light over 101ms
	return duds::data::Quantity(
		(double)ir * scale * 0.055,
		duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		)
	);
}

duds::data::Quantity APDS9301::illuminance() const {
	// Implementation of an algorithim in the documentation that approximates
	// the illuminance "for the light source specified", which was subsequently
	// not specified. At least it wasn't in the file I got from a link on
	// Sparkfun's website. It does show test results with two light sources,
	// but that isn't singular.
	double ch0 = (double)broad;
	double ch1 = (double)ir;
	double chr = ch1 / ch0;  // channel ratio
	double result;
	if (chr <= 0.5) {
		result = 0.0304 * ch0 - 0.062 * ch0 * std::pow(chr, 1.4);
	} else if (chr <= 0.61) {
		result = 0.0224 * ch0 - 0.031 * ch1;
	} else if (chr <= 0.8) {
		result = 0.0128 * ch0 - 0.0153 * ch1;
	} else if (chr <= 1.3) {
		result = 0.00146 * ch0 - 0.00112 * ch1;
	} else {
		result = 0;
	}
	if (result != 0) {
		// Scale to account for periods other than 402ms and for gain. The scale
		// value is made to scale integration to 102ms, and includes gain, so
		// adjust integration only with a constant factor.
		result *= scale * (402.0 / 101.0);
	}
	return duds::data::Quantity(
		result * scale,
		duds::data::units::Lux
	);
}

} } } }
