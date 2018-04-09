/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#ifndef APDS9301_HPP
#define APDS9301_HPP

#include <duds/hardware/interface/Smbus.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/data/Quantity.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Base class for all APDS9301 specific errors.
 */
struct APDS9301Error : DeviceError { };
/**
 * An invalid integration time was specified.
 */
struct APDS9301BadIntegration : APDS9301Error { };

/**
 * A quick try at supporting APDS9301 brightness sensor; will change
 * significantly in the future.
 *
 * Address can be set to 0x29, 0x39, or 0x49.
 *
 * @author  Jeff Jackowski
 */
class APDS9301 : boost::noncopyable {
	/**
	 * The SMBus communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::Smbus> com;
	/**
	 * Configured integration time.
	 */
	float actualPeriod;
	/**
	 * Multiplier applied to the sample results to account for different
	 * integration times and gain setting than what the documentation used
	 * to relate the sample values to irradiance.
	 */
	float scale;
	/**
	 * The values supplied by the device.
	 */
	std::uint16_t broad, ir;
	struct {
		/**
		 * High gain (16x) flag.
		 */
		unsigned int hGain  : 1;
		/**
		 * Integration time value used in device configuration register.
		 */
		unsigned int integTime : 2;
	};
	/**
	 * Writes a value to the control register, reads it back, and checks for
	 * equality.
	 * @param val  The value to write and to expect on read.
	 * @throw DeviceMisidentified  The value read back did not match @a val.
	 */
	void startOrStop(int val);
public:
	/**
	 * Attempts to identify the device, then performs a reset. This should leave
	 * it in a low-power state where it does not sample.
	 * Default device address is 0x39. Possible device addresses are:
	 * 0x29, 0x39, and 0x49.
	 * @param c  The Smbus interface to the device. It must not use PEC.
	 * @throw DeviceMisidentified  The reported device ID is incorrect.
	 */
	APDS9301(std::unique_ptr<duds::hardware::interface::Smbus> &c);
	/**
	 * Attempts to suspend the device's operation.
	 * Does not call suspend to avoid an exception.
	 */
	~APDS9301();
	/**
	 * Configures the device.
	 * @param integration  The maximum integration time. It may be specified in
	 *                     seconds with values between 13.7 and 402ms. If the
	 *                     value is too low, APDS9301BadIntegration will be
	 *                     thrown.
	 * @param hGain        True to use 16x gain.
	 * @throw APDS9301BadIntegration
	 */
	void init(float integration, bool hGain);
	/**
	 * Suspends operation by putting the device into a low-power mode.
	 * @throw DeviceMisidentified  The device did not respond properly to the
	 *                             request.
	 */
	void suspend();
	/**
	 * Resumes operation after a call to suspend().
	 * @throw DeviceUninitalized   init() hasn't been called.
	 * @throw DeviceMisidentified  The device did not respond properly to the
	 *                             request.
	 */
	void resume();
	/**
	 * Returns the sampling period configured for the device.
	 * The value will be -1 if init() has not been called.
	 * @todo  Offer a way to report potential error on the remote clock. Maybe
	 *        a maximum period function.
	 */
	float period() const {
		return actualPeriod;
	}
	/**
	 * Returns true if the device has been configured to use its 16x gain
	 * function by a previous call to init().
	 */
	bool highGain() const {
		return hGain;
	}
	/**
	 * The device will update samples after it has completed integration.
	 * Sampling faster than that will read old values, but not bad data.
	 */
	void sample();
	/**
	 * The maximum possible irradiance value that the device can report based
	 * on its configuration set by the last call to init().
	 */
	duds::data::Quantity maxIrradiance() const;
	/**
	 * Broad spectrum irradiance. Includes visible and infrared.
	 * The value can vary greatly from what proper laboratory equipment will
	 * report under the same conditions. In particular, the documentation shows
	 * that different light sources can cause the result to vary by as much as
	 * a factor of 4, and gave few examples, so greater differences are likely
	 * possible.
	 */
	duds::data::Quantity irradiance() const;
	/**
	 * The value reported by the device for the broad spectrum brightness.
	 */
	std::uint16_t brightnessCount() const {
		return broad;
	}
	/**
	 * Irradiance in infrared light.
	 * The value can vary greatly from what proper laboratory equipment will
	 * report under the same conditions. In particular, the documentation shows
	 * that different light sources can cause the result to vary by more than
	 * a factor of 10, and gave few examples, so greater differences are likely
	 * possible.
	 */
	duds::data::Quantity irradianceIr() const;
	/**
	 * The value reported by the device for the infrared brightness.
	 */
	std::uint16_t brightnessIrCount() const {
		return ir;
	}
	/**
	 * Computes a highly approximate value for illuminance based on the broad
	 * spectrum and IR samples using the method documented in the device's
	 * data sheet. The documentation shows how well the approximation matches
	 * with only two light sources, and with those the result may be as far off
	 * as -67% to +40%.
	 */
	duds::data::Quantity illuminance() const;
};

} } } }

#endif        //  #ifndef APDS9301_HPP
