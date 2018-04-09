/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/AM2320.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

constexpr int valw = 8;
bool quit = false;

void runtest(duds::hardware::devices::instruments::AM2320 &meter, int delay)
try {
	int succ = 0, fail = 0;
	std::cout << std::fixed << std::setprecision(1);
	do {
		// sample data will be (delay-2) seconds old when output to std::cout
		std::this_thread::sleep_for(std::chrono::seconds(delay));
		bool skip = true;
		try {
			// failures are not uncommon
			meter.sample();
			++succ;
			skip = false;
		} catch (duds::hardware::interface::I2cErrorNoDevice &) {
			throw;
		} catch (duds::hardware::interface::I2cErrorUnsupported &) {
			throw;
		} catch (...) {
			std::cerr << "Failed sample attempt " << ++fail << ":\n" <<
			boost::current_exception_diagnostic_information() << std::endl;
		}
		if (!skip) {
			duds::data::Quantity humid = meter.relHumidity();
			duds::data::Quantity temp = meter.temperature();
			assert(humid.unit == duds::data::Unit(0));
			assert(temp.unit == duds::data::units::Kelvin);
			std::cout << "Temp: " << std::setw(6) <<
			temp.value << "K  " << std::setw(5) <<
			temp.value - 273.15 << "C  " << std::setw(5) <<
			temp.value * 1.8 - 459.67 << "F   Rel humid: " <<
			std::setw(5) << humid.value << '%' << std::endl;
		}
	} while (!quit);
	std::cout << "Read " << succ << " samples successfully, and failed to read "
	<< fail << " samples.\n" << std::setprecision(2) <<
	100.0f * (float)fail / (float)(succ + fail) << "% failed." << std::endl;
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

int main(int argc, char *argv[])
try {
	std::string i2cpath;
	int delay;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for AM2320 test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the I2C device file to use
				"i2cdev,i",
				boost::program_options::value<std::string>(&i2cpath)->
					default_value("/dev/i2c-1"),
				"Specify I2C device file"
			) // the time between samples
			(
				"delay,d",
				boost::program_options::value<int>(&delay)->
					default_value(8),
				"Time in seconds bewteen samples"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test program for temperature and relative humidity "
			"sensor AM2320\n" << argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
	}
	std::unique_ptr<duds::hardware::interface::I2c> i2c(
		new duds::hardware::interface::linux::DevI2c(i2cpath, 0x5C)
	);
	duds::hardware::devices::instruments::AM2320 meter(i2c);
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	std::thread doit(runtest, std::ref(meter), delay);
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
