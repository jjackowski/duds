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

#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/devices/displays/BppImageArchive.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include <duds/time/planetary/Planetary.hpp>

#include <iostream>
#include <sstream>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>

namespace Font {
	#include <numberparts.h>
}

char DigitPart(int x, int y, char d);

/**
 * A character in string for large output is not in the large font.
 */
struct TextLargeCharUnsupported :
duds::hardware::devices::displays::TextDisplayError { };

void WriteLarge(
	const std::shared_ptr<duds::hardware::devices::displays::TextDisplay> &disp,
	const std::string &str,
	unsigned int c,
	unsigned int r
);

enum {
	Clear,
	UpLeft,
	BarLeft,
	BarUp,
	BarCorn,
	Dot
};

#define DIGSEG(x, y, c)  (((c) << ((x) * 3)) << ((y) * 9))

const std::uint32_t DigitFont[] = {
	// 0
	DIGSEG(0, 0, UpLeft)  | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, BarLeft) | DIGSEG(1, 1, Clear)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 1
	DIGSEG(0, 0, Clear)   | DIGSEG(1, 0, BarLeft) | DIGSEG(2, 0, Clear) |
	DIGSEG(0, 1, Clear)   | DIGSEG(1, 1, BarLeft) | DIGSEG(2, 1, Clear) |
	DIGSEG(0, 2, Clear)   | DIGSEG(1, 2, BarCorn) | DIGSEG(2, 2, Clear),
	// 2
	DIGSEG(0, 0, BarUp)   | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, UpLeft)  | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarCorn) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 3
	DIGSEG(0, 0, BarUp)   | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, BarUp)   | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 4
	DIGSEG(0, 0, BarLeft) | DIGSEG(1, 0, Clear)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, BarUp)   | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, Clear)   | DIGSEG(1, 2, Clear)   | DIGSEG(2, 2, BarCorn),
	// 5
	DIGSEG(0, 0, UpLeft)  | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarCorn) |
	DIGSEG(0, 1, BarUp)   | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 6
	DIGSEG(0, 0, UpLeft)  | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarCorn) |
	DIGSEG(0, 1, UpLeft)  | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 7
	DIGSEG(0, 0, BarUp)   | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, Clear)   | DIGSEG(1, 1, Clear)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, Clear)   | DIGSEG(1, 2, Clear)   | DIGSEG(2, 2, BarCorn),
	// 8
	DIGSEG(0, 0, UpLeft)  | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, UpLeft)  | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn),
	// 9
	DIGSEG(0, 0, UpLeft)  | DIGSEG(1, 0, BarUp)   | DIGSEG(2, 0, BarLeft) |
	DIGSEG(0, 1, BarUp)   | DIGSEG(1, 1, BarUp)   | DIGSEG(2, 1, BarLeft) |
	DIGSEG(0, 2, BarUp)   | DIGSEG(1, 2, BarUp)   | DIGSEG(2, 2, BarCorn)
	// could extend for hex, decimal point
};

char DigitPart(int x, int y, char d) {
	return (DigitFont[d - '0'] >> (y * 9 + x * 3)) & 7;
};

void WriteLarge(
	const std::shared_ptr<duds::hardware::devices::displays::TextDisplay> &disp,
	const std::string &str,
	unsigned int c,
	unsigned int r
) {
	//const std::shared_ptr<TextDisplay> &disp = tds.display();
	// must start on line 0 or 1
	if (c > 1) {
		DUDS_THROW_EXCEPTION(
			duds::hardware::devices::displays::TextDisplayRangeError() <<
			duds::hardware::devices::displays::TextDisplayPositionInfo(
				duds::hardware::devices::displays::Info_DisplayColRow(c, r)
			)
		);
	}
	// work out width
	int width = 0;
	for (char c : str) {
		if ((c >= '0') && (c <= '9')) {
			width += 3;
		} else if (c == ':') {
			++width;
		} else {
			DUDS_THROW_EXCEPTION(TextLargeCharUnsupported());
		}
	}
	if ((c + width) > disp->columns()) {
		DUDS_THROW_EXCEPTION(
			duds::hardware::devices::displays::TextDisplayRangeError() <<
			duds::hardware::devices::displays::TextDisplayPositionInfo(
				duds::hardware::devices::displays::Info_DisplayColRow(c, r)
			)
		);
	}
	// write out 3 lines
	for (int y = 0; y < 3; ++y) {
		std::string line;
		for (char c : str) {
			if (c == ':') {
				if (y < 2) {
					line.push_back(Dot + 8);
				} else {
					line.push_back(' ');
				}
			} else {
				for (int x = 0; x < 3; ++x) {
					char dp = DigitPart(x, y, c);
					if (!dp) {
						dp = ' ';
					} else {
						dp += 8;
					}
					line.push_back(dp);
				}
			}
		}
		// write whole line
		disp->write(line, c, r + y);
	}
}


// -----------------------------------------------------------------------


namespace displays = duds::hardware::devices::displays;

bool quit = false;

void runtest(const std::shared_ptr<displays::HD44780> &tmd)
try {
	duds::hardware::devices::clocks::LinuxClockDriver lcd;
	duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample ts;
	do {
		lcd.sampleTime(ts);
		boost::posix_time::time_duration time =
			duds::time::planetary::earth->posix(ts.value).time_of_day();
		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(2) << time.hours() << ':' <<
		std::setw(2) << time.minutes() << ':' << std::setw(2) << time.seconds();
		//std::cout << "Writing time " << oss.str() << std::endl;
		WriteLarge(tmd, oss.str(), 0, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(980));
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	/*
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for addressLCD"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Show network addresses on attached text LCD\n" <<
			argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
	} */
	
	// load some icons before messing with hardware
	/*
	displays::BppImageArchive imgArc;
	imgArc.load("neticons.bppia");
	std::shared_ptr<displays::BppImage> wiredIcon = imgArc.get("WiredLAN");
	std::shared_ptr<displays::BppImage> wirelessIcon = imgArc.get("WirelessLAN_S2");
	*/
	duds::time::planetary::Earth::make();
	
	std::shared_ptr<displays::BppImage> NumPartUpLeft =
		displays::BppImage::make(Font::NumPartUpLeft);
	std::shared_ptr<displays::BppImage> NumPartBarLeft =
		displays::BppImage::make(Font::NumPartBarLeft);
	std::shared_ptr<displays::BppImage> NumPartBarUp =
		displays::BppImage::make(Font::NumPartBarUp);
	std::shared_ptr<displays::BppImage> NumPartBarCorn =
		displays::BppImage::make(Font::NumPartBarCorn);
	std::shared_ptr<displays::BppImage> NumPartDot =
		displays::BppImage::make(Font::NumPartDot);
	
	// configure display
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
			lcdset, lcdsel, 20, 4
		);
	tmd->initialize();
	tmd->setGlyph(NumPartUpLeft, 1);
	tmd->setGlyph(NumPartBarLeft, 2);
	tmd->setGlyph(NumPartBarUp, 3);
	tmd->setGlyph(NumPartBarCorn, 4);
	tmd->setGlyph(NumPartDot, 5);

	std::thread doit(&runtest, std::ref(tmd));

	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
