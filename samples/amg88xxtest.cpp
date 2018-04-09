/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/AMG88xx.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

constexpr int valw = 8;
bool quit = false;

template<typename InputIterator, typename OutputIterator, typename RatioType>
void ExponentialMovingAverage(
	OutputIterator result,
	InputIterator prev,
	InputIterator samp,
	RatioType fracNew,
	std::size_t items
) {
	for (; items; --items, ++result, ++prev, ++samp) {
		*result = *samp * fracNew + *prev * ((RatioType)1 - fracNew);
	}
}

const char tind[7] = { ' ', '.', ',', 'x', '!', 'X', '#' };

void runtest(duds::hardware::devices::instruments::AMG88xx &meter, bool tenFps)
try {
	double img[8][8], emavg[8][8];
	unsigned int frame = 0;
	std::cout.precision(2);
	std::cout << std::fixed;
	meter.start();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	meter.sample();
	std::copy_n(&(meter.image()[0][0]), 64, &(emavg[0][0]));
	do {
		if (tenFps) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		meter.sample();
		std::copy_n(&(meter.image()[0][0]), 64, &(img[0][0]));
		ExponentialMovingAverage(
			&(emavg[0][0]),
			&(emavg[0][0]),
			&(img[0][0]),
			0.35,
			64
		);
		double *pixel = &(emavg[0][0]);
		double mt = 640.0, Mt = 0;
		for (int i = 64; i > 0; --i, ++pixel) {
			mt = std::min(mt, *pixel);
			Mt = std::max(Mt, *pixel);
		}
		double rng = Mt - mt;
		if ((mt > 273.14) && (mt < 273.16)) {
			meter.start();
		}
		std::cout << "Frame " << frame++ << ", thermistor at " <<
		meter.temperature().value - 273.15 << "C, floor = " << mt - 273.15 <<
		"C, max +" << rng;
		rng = std::max(rng, 5.0);
		for (int x = 0; x < 8; ++x) {
			std::cout << "\n\t";
			for (int y = 0; y < 8; ++y) {
				std::cout << std::setw(6) << emavg[x][y] - 273.15 << ' ';
			}
			std::cout << "   ";
			for (int y = 0; y < 8; ++y) {
				int spot = ((emavg[x][y] - mt) / rng) * 7.0;
				spot = std::min(spot, 6);
				// early attempt at possible human body heat detection
				if ((emavg[x][y] > 306.0) && (emavg[x][y] <= 308.5)) {
					std::cout << 'h' << tind[spot];
				} else if ((emavg[x][y] > 309.0) && (emavg[x][y] <= 311.0)) {
					std::cout << 'H' << tind[spot];
				} else {
					std::cout << tind[spot] << tind[spot];
				}
			}
		}
		std::cout << std::endl;
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

int main(int argc, char *argv[])
try {
	std::string i2cpath;
	int i2caddr;
	bool tenFps = false;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for AMG88xx test"
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
			)
			( // the I2C device address to use
				"i2caddr,a",
				boost::program_options::value<int>(&i2caddr)->
					default_value(0x69),
				"Specify Smbus device address"
			)
			( // 10fps
				"ten,t",
				"Run at 10 FPS"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test program for AMG88xx thermal camera\n" <<
			argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("ten")) {
			tenFps = true;
		}
	}
	std::unique_ptr<duds::hardware::interface::I2c> i2c(
		new duds::hardware::interface::linux::DevI2c(i2cpath, i2caddr)
	);
	duds::hardware::devices::instruments::AMG88xx meter(i2c);
	if (tenFps) {
		meter.tenFps();
	} else {
		meter.oneFps();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	std::thread doit(runtest, std::ref(meter), tenFps);
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
