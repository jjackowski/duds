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
 * A sample of using HD44780 and BppImage to show large digits with the time.
 */

#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/devices/displays/BppImageArchive.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

duds::hardware::devices::displays::BppImageArchive imgArc;

/**
 * A character in string for large output is not in the large font.
 */
struct TextLargeCharUnsupported :
duds::hardware::devices::displays::TextDisplayError { };

/**
 * Writes out a string with large 3x3 digits, spaces, and colons to a text
 * display.
 * @param disp  The destination display.
 * @param str   The string to write. The values supported are:
 *              - '0' through '9': writes 3x3 representation of the digit.
 *              - ':': writes 1x3 colon
 *              - '.': writes a dot, suitable for decimal point
 *              - space: leaves 3x3 blank spot
 *              - '~': leaves 1x3 blank spot
 * @param c     The leftmost column where the string will start.
 * @param r     The uppermost row where the string will start.
 * @throw       TextLargeCharUnsupported  @a str has an unsupported character.
 * @throw       TextDisplayRangeError    The string does not fit on the display.
 */
void WriteLarge(
	const std::shared_ptr<duds::hardware::devices::displays::TextDisplay> &disp,
	const std::string &str,
	unsigned int c,
	unsigned int r
);

/**
 * The character values associated to large 3x3 digit parts.
 */
enum DigitPartCodes {
	/**
	 * Denotes a clear spot, a space, inside a large digit. This causes a
	 * regular space to be sent to the display.
	 */
	Clear,
	UpLeft,
	BarLeft,
	BarUp,
	BarCorn,
	/**
	 * Used only for colon.
	 */
	Dot,
	DownLeft = UpLeft,
	BarDown = BarUp,
};

/**
 * Returns the character value to use for the given part of a large 3x3 digit.
 * @param ud  0 for characters shifted upward, or 1 for downward shift.
 * @param x   The X coordinate into the digit.
 * @param y   The Y coordinate into the digit.
 * @param d   The digit; must be bewteen 0 and 9, not '0' and '9'.
 * @return    The value to send to the display.
 */
char DigitPart(int ud, int x, int y, char d);

/**
 * Produces the value to store for DigitPart to retrieve. The character values
 * for a single digit are packed into a single std::uint32_t. The result is
 * a value for a part of a 3x3 digit that is ready to be OR'd together with
 * the other digit parts.
 * @param x  The X coordinate into the digit. It must be between 0 and 2.
 * @param y  The Y coordinate into the digit. It must be between 0 and 2.
 * @param c  The character value to store. It must be between 0 and 7.
 */
constexpr std::uint32_t DigSeg(
	std::uint32_t x,
	std::uint32_t y,
	std::uint32_t c
) {
	return (c << (x * 3)) << (y * 9);
}

/**
 * An array of font data used to write large 3x3 digits to a text display
 * that supports at least 4 definable characters.
 */
const std::uint32_t DigitFont[2][10] = {
	{ // shifted upward and to the left 
		// 0
		DigSeg(0, 0, UpLeft)  | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, Clear)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 1
		DigSeg(0, 0, Clear)   | DigSeg(1, 0, BarLeft) | DigSeg(2, 0, Clear) |
		DigSeg(0, 1, Clear)   | DigSeg(1, 1, BarLeft) | DigSeg(2, 1, Clear) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, BarCorn) | DigSeg(2, 2, Clear),
		// 2
		DigSeg(0, 0, BarUp)   | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, UpLeft)  | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarCorn) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 3
		DigSeg(0, 0, BarUp)   | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarUp)   | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 4
		DigSeg(0, 0, BarLeft) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarUp)   | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, Clear)   | DigSeg(2, 2, BarCorn),
		// 5
		DigSeg(0, 0, UpLeft)  | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, BarUp)   | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 6
		DigSeg(0, 0, UpLeft)  | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, UpLeft)  | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 7
		DigSeg(0, 0, BarUp)   | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, Clear)   | DigSeg(1, 1, Clear)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, Clear)   | DigSeg(2, 2, BarCorn),
		// 8
		DigSeg(0, 0, UpLeft)  | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, UpLeft)  | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// 9
		DigSeg(0, 0, UpLeft)  | DigSeg(1, 0, BarUp)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarUp)   | DigSeg(1, 1, BarUp)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn)
		// could extend for hex
	},
	{ // shifted downward and to the left 
		// 0
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, Clear)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// 1
		DigSeg(0, 0, Clear)   | DigSeg(1, 0, BarCorn) | DigSeg(2, 0, Clear) |
		DigSeg(0, 1, Clear)   | DigSeg(1, 1, BarLeft) | DigSeg(2, 1, Clear) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, BarLeft) | DigSeg(2, 2, Clear),
		// 2
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, BarDown) | DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarCorn),
		// 3
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, BarDown) | DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarDown) | DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// 4
		DigSeg(0, 0, BarCorn) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, DownLeft)| DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, Clear)   | DigSeg(2, 2, BarLeft),
		// 5
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, DownLeft)| DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarCorn) |
		DigSeg(0, 2, BarDown) | DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// 6
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, DownLeft)| DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarCorn) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// 7
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, Clear)   | DigSeg(1, 1, Clear)   | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, Clear)   | DigSeg(1, 2, Clear)   | DigSeg(2, 2, BarLeft),
		// 8
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, DownLeft)| DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// 9
		DigSeg(0, 0, BarDown) | DigSeg(1, 0, BarDown) | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, DownLeft)| DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarDown) | DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft)
		// could extend for hex
	},
};

char DigitPart(int ud, int x, int y, char d) {
	return (DigitFont[ud][d - '0'] >> (y * 9 + x * 3)) & 7;
};

enum GlyphSet {
	None,
	Upward,
	Downward
};

static int glyphSet = 2;

static const char *glyphNames[2][5] = {
	{ // up
		"UpNumPartUpLeft",
		"UpNumPartBarLeft",
		"UpNumPartBarUp",
		"UpNumPartBarCorn",
		"UpNumPartDot"
	},
	{ // down
		"DownNumPartDownLeft",
		"DownNumPartBarLeft",
		"DownNumPartBarDown",
		"DownNumPartBarCorn",
		"DownNumPartDot"
	}
};

void WriteLarge(
	const std::shared_ptr<duds::hardware::devices::displays::HD44780> &disp,
	const std::string &str,
	unsigned int c,
	unsigned int r
) {
	// must start on line 0 or 1
	if (r > 1) {
		DUDS_THROW_EXCEPTION(
			duds::hardware::devices::displays::TextDisplayRangeError() <<
			duds::hardware::devices::displays::TextDisplayPositionInfo(
				duds::hardware::devices::displays::Info_DisplayColRow(c, r)
			)
		);
	}
	// do glyphs need to be loaded?
	if ((glyphSet + 1) != r) {
		glyphSet = r + 1;
		for (int i = 0; i < 5; ++i) {
			disp->setGlyph(imgArc.get(glyphNames[r][i]), i + 1);
		}
	}
	// work out width
	int width = 0;
	for (char c : str) {
		if ((c >= '0') && (c <= '9')) {
			width += 3;
		} else if ((c == ':') || (c == '~') || (c == '.')) {
			++width;
		} else if (c == ' ') {
			width += 3;
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
				// character only on two rows; other is a space
				if ((!r && (y < 2)) || (r && ( y > 0))) {
					line.push_back(Dot);
				} else {
					line.push_back(' ');
				}
			} else if (c == '~') {
				line.push_back(' ');
			} else if (c == '.') {
				line.push_back(BarCorn);
			} else if (c == ' ') {
				line += "   ";
			}else {
				for (int x = 0; x < 3; ++x) {
					char dp = DigitPart(r, x, y, c);
					if (!dp) {
						dp = ' ';
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
	displays::TextDisplayStream tds(tmd);
	std::chrono::high_resolution_clock::time_point start;
	float dispTime = 32768.0f;  // in microseconds
	// examples never delete these
	boost::gregorian::date_facet *dateform =
		new boost::gregorian::date_facet("%a %b %e, %Y");
	// change how dates are output to the stream
	tds.imbue(std::locale(tds.getloc(), dateform));
	do {
		// used to tell how long it took to write the time & date to the display
		start = std::chrono::high_resolution_clock::now();
		//tds << displays::move(0,0);
		lcd.sampleTime(ts);
		// put time in time_t; integer value is in seconds UTC
		std::time_t tt = duds::time::planetary::earth->timeUtc(ts.value);
		// compute microseconds after time in tt
		int uS = (ts.value.time_since_epoch().count() / 1000) % 1000000;
		// adavnce time ahead by 64ms plus an estimate of how long it takes to
		// write out the time and date
		int advT = uS + 64000 + (int)dispTime;
		// advancing past the sampled second in tt?
		if (advT > 1000000) {
			// advance tt to match
			++tt;
		}
		// request local time; GNU version includes current timezone
		std::tm ltime;
		localtime_r(&tt, &ltime);
		// test for showing both sets of large digits
		if (ltime.tm_min & 1) {
			tds << displays::move(0,3);
		} else {
			tds << displays::move(0,0);
		}
		// Get the date of the local time; Boost is better with the dates than
		// C++11, but lacks local timezone. Looks like much of the Boost
		// date_time library will be in C++20.
		boost::gregorian::date date = boost::gregorian::date_from_tm(ltime);
		// write out the date and timezone
		tds << date << ' ' << std::setw(3) << ltime.tm_zone << displays::startLine;
		// write out the time to a string stream
		std::ostringstream oss;
		char sep;
		// blinking colon
		if (ltime.tm_sec & 1) {
			sep = '~';
		} else {
			sep = ':';
		}
		oss << std::setfill(' ') << std::setw(2) << ltime.tm_hour << sep <<
		std::setfill('0') << std::setw(2) << ltime.tm_min << sep <<
		std::setw(2) << ltime.tm_sec;
		// write out the time as 3x3 digits to the display
		if (ltime.tm_min & 1) {  // test for showing both sets of large digits
			WriteLarge(tmd, oss.str(), 0, 0);
		} else {
			WriteLarge(tmd, oss.str(), 0, 1);
		}
		// update exponential moving average of how long it takes to display the
		// thime
		auto timeTaken = std::chrono::high_resolution_clock::now() - start;
		dispTime = dispTime * 0.8f + 0.2f *
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(
				timeTaken
			).count());
		// time to wait for display update
		std::chrono::microseconds delay(1000000 - uS - 64000 - (int)dispTime);
		// sleep for at least 16ms
		if (delay.count() > 16384) {
			std::this_thread::sleep_for(delay);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	} while (!quit);
	// I am curious how long this is. Answer is about 70ms.
	std::cout << "Last moving average of time taken to display: " <<
	dispTime << "uS" << std::endl;
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

//typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	std::string confpath;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for clockLCD"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			(
				"conf,c",
				boost::program_options::value<std::string>(&confpath)->
					default_value("samples/pins.conf"),
				"Pin configuration file; REQUIRED"
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
	}
	duds::time::planetary::Earth::make();
	{
		std::string binpath(argv[0]);
		while (!binpath.empty() && (binpath.back() != '/')) {
			binpath.pop_back();
		}
		// load some icons before messing with hardware
		imgArc.load(binpath + "numberparts.bppia");
	}

	// read in digital pin config
	boost::property_tree::ptree tree;
	boost::property_tree::read_info(confpath, tree);
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	duds::hardware::interface::PinConfiguration pc(pinconf);

	// configure display
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		duds::hardware::interface::linux::SysFsPort::makeConfiguredPort(pc);
	duds::hardware::interface::DigitalPinSet lcdset;
	duds::hardware::interface::ChipSelect lcdsel;
	pc.getPinSetAndSelect(lcdset, lcdsel, "lcd");
	
	/* old
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
	*/
	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			lcdset, lcdsel, 20, 4
		);
	tmd->initialize();

	std::thread doit(&runtest, std::ref(tmd));

	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
