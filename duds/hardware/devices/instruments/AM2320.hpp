/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/I2c.hpp>
#include <duds/hardware/interface/Conversation.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/data/Quantity.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Base class for all AM2320 specific errors.
 */
struct AM2320Error : DeviceError { };

/**
 * The received CRC value was not consistent with the message data.
 */
struct AM2320CrcError : virtual AM2320Error, virtual duds::general::CrcError { };

/**
 * Support for the AM2320, a temperature and relative humidity sensor with
 * poorly written English documentation.
 *
 * This part seems to fail with a read error on about 1.2% of sample attempts.
 *
 * @bug   The CRC value is not checked because the calculated value never
 *        matches what the device sends. Even using the code from the
 *        datasheet produces the same non-matching result computed here.
 *
 * @author  Jeff Jackowski
 */
class AM2320  : boost::noncopyable {
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> com;
	/**
	 * Used to awaken the device; needed initially and after 3 or more seconds
	 * of not talking to the device.
	 */
	duds::hardware::interface::Conversation wake;
	/**
	 * Used to read in sampled data from the device.
	 */
	duds::hardware::interface::Conversation read;
	/**
	 * Relative humidity
	 */
	std::uint16_t rh;
	/**
	 * Temperature.
	 */
	std::int16_t t;
public:
	/**
	 * Only address is 0x5C.
	 * Calls sample(); the received data will be either old or invalid.
	 */
	AM2320(std::unique_ptr<duds::hardware::interface::I2c> &c);
	/**
	 * Reads in the last sample and causes the device to start another sample.
	 * About two seconds after this function is done, the new sample will be
	 * available for reading by another call to sample(). This function's
	 * results are always a sample behind the most current data.
	 * Sampling takes two seconds to complete, so calling this function more
	 * often is not helpful.
	 * @note  Sampling is documented to cause some internal heating in the
	 *        device that skews the sampled values. This does seem to occur at
	 *        the maximum sampling rate (0.5Hz). It seems that the data is
	 *        better when the sampling rate is no faster than once every four
	 *        seconds. Longer times between sampling may require two samples in
	 *        two seconds to deal with the fact that the first sample in the
	 *        pair will be old.
	 *
	 * @throw duds::hardware::interface::I2cError  This seems to occur on about
	 *                                             1.2% of the calls to sample().
	 * @throw DeviceMisidentified  The device's response did not properly echo
	 *                             the command byte and requested data length.
	 *                             This likely means the device is not an
	 *                             AM2320.
	 * @throw AM2320CrcError       The data and the CRC received are not
	 *                             consistent with each other.
	 *
	 * @todo  Record time of sample to assign to data read by the next call
	 *        to sample(). Skip using wake if the last call to sample() was
	 *        less than 3 seconds (maybe 2.5 to avoid issues if system running
	 *        slow).
	 * @bug   The CRC value is not checked because the calculated value never
	 *        matches what the device sends. Even using the code from the
	 *        datasheet produces the same non-matching result computed here.
	 */
	void sample();
	/**
	 * Ten times precentage.
	 */
	std::uint16_t rawRelHumid() const {
		return rh;
	}
	/**
	 * Ten times Celsius.
	 */
	std::int16_t rawTemp() const {
		return t;
	}
	/**
	 * Returns the unitless relative humidity quantity.
	 */
	duds::data::Quantity relHumidity() const;
	/**
	 * Returns the temperature in Kelvin.
	 */
	duds::data::Quantity temperature() const;
};

} } } }
