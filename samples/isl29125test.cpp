/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/devices/instruments/ISL29125.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>

constexpr int valw = 8;
bool quit = false;

namespace displays = duds::hardware::devices::displays;

void runtest(
	const std::shared_ptr<displays::HD44780> &tmd,
	duds::hardware::devices::instruments::ISL29125 &rgb
) try {
	displays::TextDisplayStream tds(tmd);
	do {
		rgb.sample();
		tds << "R:" << std::right << std::setw(5) << rgb.red() <<
		" G:" << std::setw(5) << rgb.green() << "\nB:" << std::setw(5) <<
		rgb.blue() << displays::move(0,0);
		std::cout << "Red: " << std::right << std::setw(5) << rgb.red() <<
		"  Green: " << std::setw(5) << rgb.green() << " Blue: " << std::setw(5)
		<< rgb.blue() << '\r';
		std::cout.flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	} while (!quit);
} catch (...) {
	std::cerr << "Test failed in thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	//                       LCD pins:  4  5   6   7  RS   E
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21 };
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		std::make_shared<duds::hardware::interface::linux::SysFsPort>(
			gpios, 0
		);
	assert(!port->simultaneousOperations());  //  :-(
	// select pin
	std::unique_ptr<duds::hardware::interface::DigitalPinAccess> selacc =
		port->access(5); // gpio 21
	std::shared_ptr<duds::hardware::interface::ChipPinSelectManager> selmgr =
		std::make_shared<duds::hardware::interface::ChipPinSelectManager>(
			selacc
		);
	assert(!selacc);
	duds::hardware::interface::ChipSelect lcdsel(selmgr, 1);
	// set for LCD data
	gpios.clear();
	gpios.insert(gpios.begin(), uintIterator(0), uintIterator(5));
	duds::hardware::interface::DigitalPinSet lcdset(port, gpios);
	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			lcdset, lcdsel, 16, 2
		);
	tmd->initialize();
	std::unique_ptr<duds::hardware::interface::I2c> i2c(
		new duds::hardware::interface::linux::DevI2c(
			"/dev/i2c-1",
			0x44
		)
	);
	duds::hardware::devices::instruments::ISL29125 rgb(i2c);
	bool wide = true;
	if ((argc > 1) && ((argv[1][0] == '0') || (argv[1][0] == 'n'))) {
		wide = false;
	}
	rgb.init(wide);
	std::thread doit(runtest, std::ref(tmd), std::ref(rgb));
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
