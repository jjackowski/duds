/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
/**
 * @file
 * A sample of using HD44780 and TextDisplayStream along with BppImage to
 * define graphic icons for use with the display. Shows IPv4 addresses on the
 * display with icons for wired and wireless networks.
 */

#include <duds/hardware/devices/displays/ST7920.hpp>
#include <duds/hardware/devices/displays/BppImageArchive.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#ifndef USE_SYSFS_PORT
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#endif
#include <duds/hardware/interface/test/VirtualPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <thread>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>


bool quit = false;

namespace displays = duds::hardware::devices::displays;

template <std::size_t B>
class PatternFiller {
public:
	template <typename R, typename P>
	static R fill(R partial, P pattern) {
		return PatternFiller<B - 1>::fill(partial | ((R)pattern << (B * sizeof(P) * 8)), pattern);
	}
};

template <>
class PatternFiller<0> {
public:
	template <typename R, typename P>
	static R fill(R partial, P pattern) {
		return partial | pattern;
	}
};

template <typename R, typename P>
R PatternFill(P pattern) {
	return PatternFiller<sizeof(R) / sizeof(P) - 1>::fill((R)0, pattern);
}

void runtest(
	const std::shared_ptr<displays::ST7920> &disp,
	bool once
) try {
	displays::BppImage img(disp->frame().dimensions());
	displays::BppImage::PixelBlock pval;
	displays::BppImage::PixelBlock *buf;
	std::size_t blkcnt;
	int pat = 0;
	do {
		// draw a pattern
		switch (pat) {
			case 0:
				// dark left
				pval = PatternFill<displays::BppImage::PixelBlock>(
					(std::uint16_t)0xFF
				);
				for (
					blkcnt = img.bufferSize(), buf = img.buffer();
					blkcnt > 0;
					++buf, --blkcnt
				) {
					*buf = pval;
				}
				break;
			case 1:
				// dark right
				pval = PatternFill<displays::BppImage::PixelBlock>(
					(std::uint16_t)0xFF00
				);
				for (
					blkcnt = img.bufferSize(), buf = img.buffer();
					blkcnt > 0;
					++buf, --blkcnt
				) {
					*buf = pval;
				}
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				// on even pat, 0x55 on odd lines, 0xAA on even lines
				// reverse for odd pat
				for (int h = 0; h < img.height(); ++h) {
					if (h & 1) {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0xAAAA >> (pat & 1))
						);
					} else {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0x5555 << (pat & 1))
						);
					}
					buf = img.bufferLine(h);
					for (blkcnt = img.blocksPerLine(); blkcnt > 0; ++buf, --blkcnt) {
						*buf = pval;
					}
				}
				break;
			case 7:
			case 8:
			case 9:
			case 10:
				// on even pat, 0x33 on odd lines, 0xCC on even lines
				// reverse for odd pat
				for (int h = 0; h < img.height(); ++h) {
					if (h & 1) {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0xCCCC >> (pat & 1))
						);
					} else {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0x3333 << (pat & 1))
						);
					}
					buf = img.bufferLine(h);
					for (blkcnt = img.blocksPerLine(); blkcnt > 0; ++buf, --blkcnt) {
						*buf = pval;
					}
				}
				break;
			case 11:
			case 12:
			case 13:
			case 14:
				// on even pat, 0x33 on odd lines, 0xCC on even lines
				// reverse for odd pat
				for (int h = 0; h < img.height(); ++h) {
					if (h & 2) {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0xCCCC >> (pat & 1))
						);
					} else {
						pval = PatternFill<displays::BppImage::PixelBlock>(
							(std::uint16_t)(0x3333 << (pat & 1))
						);
					}
					buf = img.bufferLine(h);
					for (blkcnt = img.blocksPerLine(); blkcnt > 0; ++buf, --blkcnt) {
						*buf = pval;
					}
				}
				break;
		}
		disp->write(&img);
		if (!once) {
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		if (++pat > 15) {
			if (once) return;
			pat = 0;
		}
	} while (!quit);
} catch (...) {
	std::cerr << "Test failed in runtest():\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

int main(int argc, char *argv[])
try {
	std::string confpath;
	int dispW, dispH;
	bool noinput = false, once = false, fakeport = false;
	bool usegpiodev =
		#ifdef USE_SYSFS_PORT
		false;
		#else
		true;
		#endif
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for ST7920 test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the LCD size
				"width,x",
				boost::program_options::value<int>(&dispW)->
					default_value(144),
				"Display width in pixels"
			)
			( // the LCD size
				"height,y",
				boost::program_options::value<int>(&dispH)->
					default_value(32),
				"Display height in pixels"
			)
			(
				"conf,c",
				boost::program_options::value<std::string>(&confpath)->
					default_value("samples/pins.conf"),
				"Pin configuration file; REQUIRED"
			)
			(
				"sysfs,s",
				"Use the GPIO interface at /sys/class/gpio/."
			)
			#ifndef USE_SYSFS_PORT
			(
				"gpiodev,g",
				"Use the GPIO device file."
			)
			#endif
			(
				"fake,f",
				"Use the VirtualPort interface for GPIO."
			)
			( // don't read from cin; run everything on one thread
				"noinput",
				"Do not accept input for termination request. OpenRC will claim "
				"this program has crashed without this option because it appears "
				"to send the termination request."
			)
			(
				"once,1",
				"Run once through all test patterns without a delay. Implies "
				"noinput. Use this for profiling."
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test of ST7920 graphic LCD driver\n" <<
			argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("noinput")) {
			noinput = true;
		}
		if (vm.count("once")) {
			once = true;
			noinput = true;
		}
		if (vm.count("fake")) {
			fakeport = true;
		} else {
			if (vm.count("sysfs")) {
				usegpiodev = false;
			}
			#ifndef USE_SYSFS_PORT
			if (vm.count("gpiodev")) {
				usegpiodev = true;
			}
			#endif
		}
	}

	/*
	std::string binpath(argv[0]);
	while (!binpath.empty() && (binpath.back() != '/')) {
		binpath.pop_back();
	}
	// load some icons before messing with hardware
	displays::BppImageArchive imgArc;
	imgArc.load(binpath + "neticons.bppia");
	std::shared_ptr<displays::BppImage> wiredIcon = imgArc.get("WiredLAN");
	std::shared_ptr<displays::BppImage> wirelessIcon[4] = {
		imgArc.get("WirelessLAN_S0"),
		imgArc.get("WirelessLAN_S1"),
		imgArc.get("WirelessLAN_S2"),
		imgArc.get("WirelessLAN_S3"),
	};
	*/
	// read in digital pin config
	boost::property_tree::ptree tree;
	boost::property_tree::read_info(confpath, tree);
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	duds::hardware::interface::PinConfiguration pc(pinconf);

	// configure display
	std::shared_ptr<duds::hardware::interface::DigitalPort> port;
	if (fakeport) {
		port = duds::hardware::interface::test::VirtualPort::makeConfiguredPort(pc);
	} else {
		#ifdef USE_SYSFS_PORT
		port = duds::hardware::interface::linux::SysFsPort::makeConfiguredPort(pc);
		#else
		if (usegpiodev) {
			port =
				duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(pc);
		} else {
			port =
				duds::hardware::interface::linux::SysFsPort::makeConfiguredPort(pc);
		}
		#endif
	}

	duds::hardware::interface::DigitalPinSet lcdset;
	duds::hardware::interface::ChipSelect lcdsel;
	pc.getPinSetAndSelect(lcdset, lcdsel, "lcd");
	// LCD driver
	std::shared_ptr<displays::ST7920> disp =
		std::make_shared<displays::ST7920>(
			std::move(lcdset), std::move(lcdsel), dispW, dispH
		);
	disp->initialize();

	if (noinput) {
		// will not return, unless once is true
		runtest(disp, once);
	} else {
		std::thread doit(runtest, std::ref(disp), once);
		std::cin.get();
		quit = true;
		doit.join();
	}
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
