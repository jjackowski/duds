/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/INA219.hpp>
#include <duds/hardware/interface/linux/DevSmbus.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>

constexpr int valw = 8;
bool quit = false;

void runtest(duds::hardware::devices::instruments::INA219 &ina) {
	std::cout.precision(5);
	std::int16_t sv, bv;
	do {
		ina.sample();
		duds::data::Quantity shnV = ina.shuntVoltage();
		duds::data::Quantity busV = ina.busVoltage();
		duds::data::Quantity busI = ina.busCurrent();
		assert(busV.unit == duds::data::units::Volt);
		assert(busI.unit == duds::data::units::Ampere);
		assert(shnV.unit == duds::data::units::Volt);
		duds::data::Quantity busP = busV * busI;
		assert(busP.unit == duds::data::units::Watt);
		ina.vals(sv, bv);
		std::cout << "Shunt: " << std::setw(valw) << shnV.value <<
			"v   Bus: " << std::setw(valw) << busV.value << "v  " <<
			std::setw(valw) << busI.value << "A  " << std::setw(valw) << 
			busP.value <<
			//'W' << std::endl;
			"W   s = " << std::setw(valw-2) << sv << " b = " << 
			std::setw(valw-2) << bv << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (!quit);
}

int main(void)
try {
	std::unique_ptr<duds::hardware::interface::Smbus> smbus(
		new duds::hardware::interface::linux::DevSmbus(
			"/dev/i2c-1",
			0x40,
			duds::hardware::interface::Smbus::NoPEC()
		)
	);
	duds::hardware::devices::instruments::INA219 meter(smbus, 0.1);
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	std::thread doit(runtest, std::ref(meter));
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "ERROR: " << boost::current_exception_diagnostic_information()
	<< std::endl;
	return 1;
}
