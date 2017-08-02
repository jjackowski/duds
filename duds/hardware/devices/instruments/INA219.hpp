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
 * Preliminary support for TI's INA219 that appears broken.
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
	std::int16_t shuntV;
	std::int16_t busV;
public:
	/**
	 * @todo  Find way to reject bad SMBus config. Maybe find way to produce
	 *        SMBus config.
	 */
	INA219(
		std::unique_ptr<duds::hardware::interface::Smbus> &c, // no PEC!
		double s
	);
	~INA219();
	duds::data::Quantity maxCurrent() const;
	duds::data::Quantity shuntResistance() const;
	duds::data::Quantity shuntVoltage() const;
	duds::data::Quantity busVoltage() const;
	duds::data::Quantity busCurrent() const;
	duds::data::Quantity busPower() const {
		return busVoltage() * busCurrent();
	}
	void sample();
	
	void vals(std::int16_t &sV, std::int16_t &bV) {
		sV = shuntV;
		bV = busV;
	}
};

} } } }

#endif        //  #ifndef INA219_HPP
