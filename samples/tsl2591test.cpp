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
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>

constexpr int valw = 8;
bool quit = false;

void runtest(duds::hardware::devices::instruments::TSL2591 &meter)
try {
	duds::data::Unit chku = duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		);
	std::cout.precision(6);
	std::uint16_t c0, c1;
	do {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		meter.sample();
		duds::data::Quantity visB = meter.brightness();
		duds::data::Quantity irB = meter.brightnessIr();
		assert(visB.unit == chku);
		assert(irB.unit == chku);
		std::cout << "Visible: " << std::setw(16) << visB.value << "W/m2  " <<
		std::setw(5) << meter.brightnessCount() << " count   IR: " <<
		std::setw(16) << irB.value << "W/m2  " << std::setw(5) <<
		meter.brightnessIrCount() << " count" << std::endl;
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
	int gain = 0;
	if (argc > 1) {
		gain = argv[1][0] - '0';
	}
	int integration = 0;
	if (argc > 2) {
		integration = argv[2][0] - '0';
	}
	meter.init(gain, integration);
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	std::thread doit(runtest, std::ref(meter));
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
