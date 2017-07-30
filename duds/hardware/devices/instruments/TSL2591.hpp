/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/I2c.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/data/Quantity.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Base class for all TSL2591 specific errors.
 */
struct TSL2591Error : DeviceError { };
/**
 * An invalid gain value was specified.
 */
struct TSL2591BadGain : TSL2591Error { };
/**
 * An invalid integration time was specified.
 */
struct TSL2591BadIntegration : TSL2591Error { };

/**
 * A quick try at supporting TSL2591 brightness sensor; will change
 * significantly in the future. Seems to work, except the Quantity values
 * are wrong.
 *
 * @author  Jeff Jackowski
 */
class TSL2591 {
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> com;
	/**
	 * The conversation used to initialize the device. It is created in init()
	 * and held for later use by resume().
	 */
	duds::hardware::interface::Conversation initialize;
	/**
	 * The conversation used to query the brightness values. It is created once
	 * in the constructor to avoid recreating it.
	 */
	duds::hardware::interface::Conversation input;
	/**
	 * A scalar value used to partially convert the counts supplied by the
	 * device into a value in Watts per square meter. It takes into account the
	 * gain and integration settings.
	 * @bug  The value is almost certainly wrong.
	 */
	double scale;
	/**
	 * The values supplied by the device.
	 */
	std::uint16_t broad, ir;
public:
	/**
	 * Attempts to identify the device, then performs a reset. This should leave
	 * it in a low-power state where it does not sample.
	 * Default device address is 0x29.
	 * @throw DeviceMisidentified  The reported device ID is incorrect.
	 */
	TSL2591(std::unique_ptr<duds::hardware::interface::I2c> &c);
	/**
	 * Calls suspend().
	 */
	~TSL2591();
	/**
	 * Configures the device.
	 * @param gain         The gain mode to use. 0 is the lowest gain, and
	 *                     3 is the highest. The factors are listed as:
	 *                      -# 1
	 *                      -# 25
	 *                      -# 428
	 *                      -# 9876
	 * @param integration  The integration time. It may be specified in
	 *                     milliseconds with values between 100 and 600. Only
	 *                     increments of 100ms can be used; values between
	 *                     these increments will be reduced to the next lowest
	 *                     value. The time may also be specified as a value
	 *                     between 0 (100ms) and 5 (600ms).
	 * @throw TSL2591BadGain
	 * @throw TSL2591BadIntegration
	 */
	void init(int gain, int integration);
	/**
	 * Suspends operation by putting the device into a low-power mode.
	 */
	void suspend();
	/**
	 * Resumes operation after a call to suspend().
	 * @throw DeviceUninitalized  init() hasn't been called.
	 */
	void resume();
	/**
	 * The device will update samples after it has completed integration.
	 * Sampling faster than that will read old values, but not bad data.
	 */
	void sample();
	/**
	 * Broad spectrum brightness. Includes visible and infrared.
	 * @bug  The value is wrong. I'm not sure which numbers on the
	 *       datasheet to use to generate the number, or even if any of them
	 *       are the ones needed. Changing the gain will result in very
	 *       different results for the same light.
	 */
	duds::data::Quantity brightness() const;
	/**
	 * The value reported by the device for the broad spectrum brightness.
	 */
	std::uint16_t brightnessCount() const {
		return broad;
	}
	/**
	 * Brightness mostly in infrared.
	 * @bug  The value is wrong. I'm not sure which numbers on the
	 *       datasheet to use to generate the number, or even if any of them
	 *       are the ones needed. Changing the gain will result in very
	 *       different results for the same light.
	 */
	duds::data::Quantity brightnessIr() const;
	/**
	 * The value reported by the device for the infrared brightness.
	 */
	std::uint16_t brightnessIrCount() const {
		return ir;
	}
};

} } } }
