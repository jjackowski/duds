/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef ISL29125_HPP
#define ISL29125_HPP

#include <duds/hardware/interface/I2c.hpp>
#include <duds/hardware/interface/Conversation.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * A quick try at supporting ISL29125 RGB sensor; will change significantly
 * in the future. Uses I2C to read six bytes in one call rather than three
 * calls with SMBus. Seems to work well.
 * @author  Jeff Jackowski
 */
class ISL29125 : boost::noncopyable {
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> com;
	/**
	 * Output used to initialize the device.
	 */
	duds::hardware::interface::Conversation initialize;
	/**
	 * Used to read in the sampled data.
	 */
	duds::hardware::interface::Conversation input;
	/**
	 * Red brightness.
	 */
	std::uint16_t r;
	/**
	 * Green brightness.
	 */
	std::uint16_t g;
	/**
	 * Blue brightness.
	 */
	std::uint16_t b;
public:
	/**
	 * Prepares to communicate with the device, but does not initalize the
	 * device.
	 * Default device address is 0x44.
	 */
	ISL29125(std::unique_ptr<duds::hardware::interface::I2c> &c);
	/**
	 * Calls suspend().
	 */
	~ISL29125();
	/**
	 * Configures for continuous 16-bit sampling of all colors.
	 * All other options are left at default settings.
	 * @param wide  True for 10000 lux range, false for 375 lux range.
	 */
	void init(bool wide = true);
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
	 * The device takes about 101ms to produce a 16-bit sample per color.
	 * Sampling faster than that will read old values, but not bad data.
	 */
	void sample();

	// Not sure what the units are; may change with lux range and ADC sample
	// size. With the narrow, 375lux, range and 16-bit samples:
	//    red may be   (20/65536) uW/cm^2
	//    green may be (18/65536) uW/cm^2
	//    blue may be  (30/65536) uW/cm^2

	std::uint16_t red() const {
		return r;
	}
	std::uint16_t green() const {
		return g;
	}
	std::uint16_t blue() const {
		return b;
	}
};

} } } }

#endif        //  #ifndef ISL29125_HPP
