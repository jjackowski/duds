/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/TSL2591.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <duds/hardware/interface/linux/SysPwm.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>

constexpr int valw = 8;
bool quit = false;

void runtest(
	duds::hardware::devices::instruments::TSL2591 &meter,
	duds::hardware::interface::linux::SysPwm &pwm
)
try {
	duds::data::Unit chku = duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		);
	std::cout.precision(6);
	double expmavg = 0;
	double emaBr = 0;
	int cnt = 0;
	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(128));
		meter.sample();
		duds::data::Quantity visB = meter.brightness();
		duds::data::Quantity irB = meter.brightnessIr();
		assert(visB.unit == chku);
		assert(irB.unit == chku);
		if ((++cnt % 16) == 0) {
			std::cout << "Brightness: " << std::setw(5) << meter.brightnessCount()
			<< "  Brightness avg: " << std::setprecision(5) << std::setw(12) <<
			"  Duty: " << std::setprecision(5) << std::setw(8) << expmavg <<
			std::endl;
		}
		
		emaBr = (double)meter.brightnessCount() * 0.4 + emaBr * 0.6;
		// really bright?
		if (emaBr > 28672.0) {
			pwm.disable();
			expmavg = 0;
		} else {
			// not quite bright enough?
			if (emaBr > 24576.0) {
				pwm.dutyFull();
				expmavg = 1.0;
			} else {
				// dimmer; backlight doesn't need to be so bright
				double r = (double)(emaBr - 2048.0) * (1.0/22528.0);
				if (r > 1.0) {
					r = 1.0;
				}
				// impose a minimum brightness
				else if (r < 0.1) {
					r = 0.1;
				}
				expmavg = 0.66 * expmavg + 0.34 * r;
				pwm.dutyCycle(expmavg);
			}
			pwm.enable();
		}
		
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

int main(int argc, char *argv[])
try {
	std::unique_ptr<duds::hardware::interface::I2c> i2c(
		new duds::hardware::interface::linux::DevI2c(
			"/dev/i2c-1",
			0x29
		)
	);
	duds::hardware::devices::instruments::TSL2591 meter(i2c);
	int gain = 1;
	if (argc > 1) {
		gain = argv[1][0] - '0';
	}
	int integration = 1;
	if (argc > 2) {
		integration = argv[2][0] - '0';
	}
	meter.init(gain, integration);
	duds::hardware::interface::linux::SysPwm pwm(0, 0);
	pwm.frequency(10000);
	pwm.dutyZero();
	pwm.disable();
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	std::thread doit(runtest, std::ref(meter), std::ref(pwm));
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
