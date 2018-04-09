/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef INA219_HPP
#define INA219_HPP

#include <duds/hardware/interface/Smbus.hpp>
#include <duds/data/Quantity.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Preliminary support for TI's INA219.
 * This device measures two voltages in a way that allows computation of
 * current and power. The current must pass through a shunt resistor. The
 * voltage across that resistor, and the voltage between there and ground,
 * is measured. This class can compute the power and current based on these
 * measurements, but it needs to know the shunt resistance for these
 * computations
 *
 * Normally uses address 0x40. Does not use PEC; not really a SMBus device.
 *
 * @author  Jeff Jackowski
 */
class INA219 : boost::noncopyable {
	/**
	 * Communication bus.
	 */
	std::unique_ptr<duds::hardware::interface::Smbus> com;
	/**
	 * Shunt resistance; used to compute current.
	 */
	double shunt;
	/**
	 * The raw data for the sampled shunt voltage.
	 */
	std::int16_t shuntV;
	/**
	 * The raw data for the sampled bus voltage.
	 */
	std::int16_t busV;
public:
	/**
	 * @todo  Find way to reject bad SMBus config. Maybe find way to produce
	 *        SMBus config.
	 * @param c   The SMBus interface to use for communication. PEC must be
	 *            disabled.
	 * @param sr  The shunt resistance in ohms.
	 */
	INA219(
		std::unique_ptr<duds::hardware::interface::Smbus> &c, // no PEC!
		double sr
	);
	~INA219();
	/**
	 * Returns the maximum current that can be measured by the device. This
	 * value changes with the shunt resistance.
	 */
	duds::data::Quantity maxCurrent() const;
	/**
	 * Returns the shunt resistance.
	 */
	duds::data::Quantity shuntResistance() const;
	/**
	 * Returns the sampled shunt voltage.
	 */
	duds::data::Quantity shuntVoltage() const;
	/**
	 * Returns the sampled bus voltage.
	 */
	duds::data::Quantity busVoltage() const;
	/**
	 * Returns the computed bus current using the sampled shunt voltage and the
	 * shunt resistance.
	 */
	duds::data::Quantity busCurrent() const;
	/**
	 * Returns the computed bus power using the sampled bus voltage and the
	 * computed bus current.
	 */
	duds::data::Quantity busPower() const;
	/**
	 * Reads sample data from the device.
	 */
	void sample();

	void vals(std::int16_t &sV, std::int16_t &bV) {
		sV = shuntV;
		bV = busV;
	}
};

} } } }

#endif        //  #ifndef INA219_HPP
