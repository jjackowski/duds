/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/linux/SysPwm.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <sstream>

// !@?!#?!#?
#undef linux

namespace duds { namespace hardware { namespace interface { namespace linux {

static const char *prefix = "/sys/class/pwm/pwmchip"; // 0/pwm0
// files:
// duty_cycle  enable  period  polarity
SysPwm::SysPwm(int chip, int channel) {
	std::ostringstream fname;
	fname << prefix << chip << "/pwm" << channel << "/enable";
	en.open(fname.str());
	if (!en.is_open()) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	unsigned int val;
	en >> val;
	if (en.fail() || (val > 1)) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	running = val == 1;
	// unwrite "enable"
	fname.seekp(-6, std::ios_base::cur);
	fname << "period";
	per.open(fname.str());
	if (!per.is_open()) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	per >> val;
	if (per.fail()) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	periodNs = std::chrono::nanoseconds(val);
	// unwrite "period"
	fname.seekp(-6, std::ios_base::cur);
	fname << "duty_cycle";
	dc.open(fname.str());
	if (!dc.is_open()) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	dc >> val;
	if (dc.fail()) {
		BOOST_THROW_EXCEPTION(PwmError() << SysPwmChip(chip) <<
			SysPwmChannel(channel) << boost::errinfo_file_name(fname.str())
		);
	}
	dutyNs = std::chrono::nanoseconds(val);
};

SysPwm::~SysPwm() {
	disable();
}

void SysPwm::enable(bool state) {
	if (state != running) {
		en.seekg(0);
		char v = '0';
		if (state) {
			++v;
		}
		en << v << std::endl;
		if (en.fail()) {
			BOOST_THROW_EXCEPTION(PwmError());
		}
		running = state;
	}
}

void SysPwm::dutyPeriod(const std::chrono::nanoseconds &ns) {
	if (dutyNs != ns) {
		dc.seekg(0);
		dc << ns.count() << std::endl;
		if (dc.fail()) {
			BOOST_THROW_EXCEPTION(PwmError() << SysPwmDutyNs(ns.count()));
		}
		dutyNs = ns;
	}
}

double SysPwm::dutyCycle() const {
	return (double)dutyNs.count() / (double)periodNs.count();
}

void SysPwm::period(const std::chrono::nanoseconds &ns) {
	if (periodNs != ns) {
		per.seekg(0);
		per << ns.count() << std::endl;
		if (per.fail()) {
			BOOST_THROW_EXCEPTION(PwmError() << SysPwmPeriodNs(ns.count()));
		}
		periodNs = ns;
	}
}

void SysPwm::dutyCycle(double ratio) {
	std::chrono::nanoseconds t((std::chrono::nanoseconds::rep)(
		(double)(periodNs.count()) * ratio)
	);
	dutyPeriod(t);
}

void SysPwm::frequency(unsigned int hz) {
	period(
		std::chrono::nanoseconds((std::chrono::nanoseconds::rep)(
			(1.0 / (double)hz) * (double)(std::nano::den))
		)
	);
}

unsigned int SysPwm::frequency() const {
	return 1.0 / (double)periodNs.count();
}

} } } }
