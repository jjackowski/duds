/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
//#include <duds/hardware/interface/Pwm.hpp>
#include <fstream>
#include <chrono>
#include <boost/exception/info.hpp>

// !@?!#?!#?
#undef linux

namespace duds { namespace hardware { namespace interface { namespace linux {

struct PwmError : virtual std::exception, virtual boost::exception { };

typedef boost::error_info<struct Info_SysPwmChip, int>  SysPwmChip;
typedef boost::error_info<struct Info_SysPwmChannel, int>  SysPwmChannel;
typedef boost::error_info<struct Info_SysPwmPeriodNs, long>
	SysPwmPeriodNs;
typedef boost::error_info<struct Info_SysPwmDutyNs, long>
	SysPwmDutyNs;

/**
 * This is a Linux-only PWM driver that I need for my eclipse project. I intend
 * to make a generalized interface, but I need to investigate PWM devices a bit
 * more before I can make a decent one.
 * @author  Jeff Jackowski
 */
class SysPwm /* : public Pwm */ {
	std::fstream en;
	std::fstream dc;
	std::fstream per;
	std::chrono::nanoseconds dutyNs;
	std::chrono::nanoseconds periodNs;
	bool running;
public:
	SysPwm(int chip, int channel);
	~SysPwm();
	void enable(bool state = true);
	void disable() {
		enable(false);
	}
	bool enabled() const {
		return running;
	}
	std::chrono::nanoseconds dutyPeriod() const {
		return dutyNs;
	}
	void dutyPeriod(const std::chrono::nanoseconds &ns);
	void dutyZero() {
		dutyPeriod(std::chrono::nanoseconds(0));
	}
	void dutyFull() {
		dutyPeriod(periodNs);
	}
	double dutyCycle() const;
	void dutyCycle(double ratio);
	std::chrono::nanoseconds period() const {
		return periodNs;
	}
	void period(const std::chrono::nanoseconds &ns);
	void frequency(unsigned int hz);
	unsigned int frequency() const;
};

} } } }
