/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef MCP9808_HPP
#define MCP9808_HPP

#include <duds/hardware/interface/Smbus.hpp>
#include <duds/data/Quantity.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Preliminary support for Microchip's MCP9808 temperature sensor.
 * The device continually samples temperature and can provide interrupt or
 * thermostat-like output.
 *
 * Normally uses an address formed by a bit-wise OR of 0x18 and the state of
 * of three address lines hardwired to the device that supply the 3 LSb (0x7).
 * Some variation of the device uses 0x48 to OR with the hardwired line bits.
 *
 * Does not use PEC; not quite a SMBus device.
 *
 * @author  Jeff Jackowski
 */
class MCP9808 : boost::noncopyable {
	/**
	 * Communication bus.
	 */
	std::unique_ptr<duds::hardware::interface::Smbus> com;
	/**
	 * Last temperature sample.
	 */
	double temp;
	/**
	 * The configuration word.
	 */
	std::uint16_t config;
	/**
	 * The raw data for the configured resolution.
	 */
	char res;
	/**
	 * Device's revision byte.
	 */
	char rev;
public:
	enum Resolution {
		Half,
		Quarter,
		Eighth,
		Sixteenth
	};
	/**
	 * Prepares to use a MCP9808 by identifying the device and reading, but not
	 * changing, its current configuration.
	 *
	 * While sampling is a power-on default, this does not prevent a previously
	 * running process from suspending sampling. The constructor does not
	 * change the device's configuration, so do not assume the device is
	 * sampling after construction.
	 *
	 * @todo  Find way to reject bad SMBus config. Maybe find way to produce
	 *        SMBus config.
	 * @param c   The SMBus interface to use for communication. PEC must be
	 *            disabled.
	 * @throw DeviceMisidentified   The device did not respond as expected.
	 */
	MCP9808(
		std::unique_ptr<duds::hardware::interface::Smbus> &c // no PEC!
	);
	/**
	 * Stops sampling.
	 */
	~MCP9808();
	/**
	 * True when the device is sampling.
	 */
	bool running() const {
		return (config & 0x100) == 0;
	}
	/**
	 * Begin sampling.
	 */
	void start();
	/**
	 * Begin sampling.
	 */
	void resume() {
		start();
	}
	/**
	 * Stop sampling.
	 * @post  Reading a sample from the device will provide the last sampled
	 *        temperature.
	 */
	void suspend();
	/**
	 * Changes the sampling resolution.
	 */
	void resolution(Resolution r);
	/**
	 * Returns the current sampling resolution.
	 */
	Resolution resolution() const {
		return (Resolution)res;
	}
	double resolutionDegrees() const;
	/**
	 * Returns the last sampled temperature.
	 */
	duds::data::Quantity temperature() const;
	/**
	 * Reads sample data from the device.
	 */
	void sample();
};

} } } }

#endif        //  #ifndef MCP9808_HPP
