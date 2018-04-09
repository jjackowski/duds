/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/APDS9301.hpp>
#include <duds/hardware/interface/linux/DevSmbus.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

constexpr int valw = 8;
bool quit = false;

void runtest(duds::hardware::devices::instruments::APDS9301 &meter)
try {
	duds::data::Unit chku = duds::data::units::Watt / (
			duds::data::units::Meter * duds::data::units::Meter
		);
	std::cout.precision(6);
	meter.resume();
	do {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		meter.sample();
		duds::data::Quantity visB = meter.irradiance();
		duds::data::Quantity irB = meter.irradianceIr();
		duds::data::Quantity ill = meter.illuminance();
		assert(visB.unit == chku);
		assert(irB.unit == chku);
		assert(ill.unit == duds::data::units::Lux);
		std::cout << "Visible: " << std::setw(10) << visB.value << "W/m2  IR: " <<
		std::setw(10) << irB.value << "W/m2  Illuminance: " << std::setw(10) <<
		ill.value << " lux" << std::endl;
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

int main(int argc, char *argv[])
try {
	std::string i2cpath;
	float inttime;
	int i2caddr;
	bool hgain = false;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for APDS9301 test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the I2C device file to use
				"smbdev,i",
				boost::program_options::value<std::string>(&i2cpath)->
					default_value("/dev/i2c-1"),
				"Specify Smbus device file"
			)
			( // the I2C device address to use
				"smbaddr,a",
				boost::program_options::value<int>(&i2caddr)->
					default_value(0x39),
				"Specify Smbus device address"
			)
			( // integration time
				"integrate,e",
				boost::program_options::value<float>(&inttime)->
					default_value(0.014f),
				"Maximum integration time"
			)
			( // high gain
				"highgain,g",
				"Use high gain (16x)"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test program for light meter APDS9301\n" << argv[0]
			<< " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("highgain")) {
			hgain = true;
		}
	}
	std::unique_ptr<duds::hardware::interface::Smbus> smb(
		new duds::hardware::interface::linux::DevSmbus(
			i2cpath, i2caddr, duds::hardware::interface::Smbus::NoPec()
		)
	);
	duds::hardware::devices::instruments::APDS9301 meter(smb);
	meter.init(inttime, hgain);
	std::cout << "Integration period is " << meter.period() << " seconds.\n"
	"Maximum reportable irradiance is " << meter.maxIrradiance().value <<
	std::endl;
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
