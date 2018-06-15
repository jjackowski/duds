/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * A sample of using HD44780 and TextDisplayStream. It is rather
 * simple and kind of stupid, but shows how to use an output stream to put
 * text on an LCD.
 */

#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
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
	const std::shared_ptr<displays::HD44780> &tmd
) try {
	std::cout << "Start test" << std::endl;
	displays::TextDisplayStream tds(tmd);
	tds << displays::move(13, 1) << "Run";
	int loop = -1;
	do {
		if ((loop & 31) == 16) {
			tds << displays::clear << "Still testing...";
		}
		tds << "Test " << std::hex << (++loop) << "  \n";
		std::cout << "Wrote some." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (!quit);
} catch (...) {
	std::cerr << "Test failed in thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(void)
try {
	//                       LCD pins:  4  5   6   7  RS   E
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21 };
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		std::make_shared<duds::hardware::interface::linux::SysFsPort>(
			gpios, 0
		);
	assert(!port->simultaneousOperations());  //  :-(
	// select pin
	std::shared_ptr<duds::hardware::interface::ChipPinSelectManager> selmgr =
		std::make_shared<duds::hardware::interface::ChipPinSelectManager>(
			port->access(5) // gpio 21
		);
	duds::hardware::interface::ChipSelect lcdsel(selmgr, 1);
	// set for LCD data
	gpios.clear();
	gpios.insert(gpios.begin(), uintIterator(0), uintIterator(5));
	duds::hardware::interface::DigitalPinSet lcdset(port, gpios);
	
	std::cout << "Construct" << std::endl;
	
	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			std::move(lcdset), std::move(lcdsel), 16, 2
		);
	
	std::cout << "Init" << std::endl;
	
	tmd->initialize();

	std::thread doit(runtest, std::ref(tmd));
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
