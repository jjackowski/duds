/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/INA219.hpp>
#include <duds/hardware/interface/linux/DevSmbus.hpp>
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/display/TextDisplayStream.hpp>
#include <duds/ui/graphics/BppImageArchive.hpp>
#ifdef USE_SYSFS_PORT
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#else
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#endif
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
//#include <duds/general/IntegerBiDirIterator.hpp>
#include <boost/program_options.hpp>

duds::ui::graphics::BppImageArchive imgArc;

/**
 * A character in string for large output is not in the large font.
 */
struct LargeCharUnsupported : duds::hardware::display::DisplayError { };

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
	const std::shared_ptr<duds::hardware::display::TextDisplay> &disp,
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
const std::uint32_t DigitFont[2][12] = {
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
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn),
		// could extend for hex
		// V
		DigSeg(0, 0, BarLeft) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, UpLeft)  | DigSeg(2, 1, BarCorn) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarCorn) | DigSeg(2, 2, Clear),
		// W
		DigSeg(0, 0, BarLeft) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, BarLeft) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, BarUp)   | DigSeg(1, 2, BarUp)   | DigSeg(2, 2, BarCorn)
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
		DigSeg(0, 2, BarDown) | DigSeg(1, 2, BarDown) | DigSeg(2, 2, BarLeft),
		// could extend for hex
		// V
		DigSeg(0, 0, BarLeft) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarLeft) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, BarDown) | DigSeg(2, 1, BarCorn) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, BarLeft) | DigSeg(2, 2, Clear),
		// W
		DigSeg(0, 0, BarCorn) | DigSeg(1, 0, Clear)   | DigSeg(2, 0, BarCorn) |
		DigSeg(0, 1, BarLeft) | DigSeg(1, 1, BarCorn) | DigSeg(2, 1, BarLeft) |
		DigSeg(0, 2, DownLeft)| DigSeg(1, 2, DownLeft)| DigSeg(2, 2, BarLeft)
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

static int glyphSet;

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
			duds::hardware::display::DisplayBoundsError() <<
			duds::hardware::display::TextDisplayPositionInfo(
				duds::hardware::display::Info_DisplayColRow(c, r)
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
		if (((c >= '0') && (c <= '9')) || (c == ' ') || (c == ';')) {
			width += 3;
		} else if ((c == ':') || (c == '~') || (c == '.')) {
			++width;
		} else {
			DUDS_THROW_EXCEPTION(LargeCharUnsupported());
		}
	}
	if ((c + width) > disp->columns()) {
		DUDS_THROW_EXCEPTION(
			duds::hardware::display::DisplayBoundsError() <<
			duds::hardware::display::TextDisplayPositionInfo(
				duds::hardware::display::Info_DisplayColRow(c, r)
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
				if (y > 1) {
					line.push_back(BarCorn);
				} else {
					line.push_back(' ');
				}
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
namespace display = duds::hardware::display;

constexpr int valw = 8;
bool quit = false;

void runtest(
	duds::hardware::devices::instruments::INA219 &ina,
	const std::shared_ptr<displays::HD44780> &tmd,
	const int delay,
	const int step
)
try {
	std::cout.precision(5);
	display::TextDisplayStream tds(tmd);
	tds << "Power     max:" << std::setfill(' ') << std::right << std::fixed;
	tds.precision(2);
	double maxpow = 0;
	double maxstep = 0;
	int s = step;
	// short delay to limit the effect of starting the program on the results
	std::this_thread::sleep_for(std::chrono::milliseconds(128));
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
		if (busP.value > maxpow) {
			maxpow = busP.value;
		}
		if (busP.value > maxstep) {
			maxstep = busP.value;
		}
		/*
		std::cout << "Shunt: " << std::setw(valw) << shnV.value <<
			"v   Bus: " << std::setw(valw) << busV.value << "v  " <<
			std::setw(valw) << busI.value << "A  " << std::setw(valw) <<
			busP.value <<
			'W' << std::endl;
			*/
		if (!--s) {
			s = step;
			std::ostringstream oss;
			oss.precision(2);
			oss << std::setfill(' ') << std::setw(4) << std::right << std::fixed <<
			maxstep << ';';  // 'W' is in array 2 past '9';
			//std::cout << "\tDisp out: \"" << oss.str() << "\"" << std::endl;
			WriteLarge(tmd, oss.str(), 7, 1);
			tds << display::move(tmd->columns() - 5, 0) << std::setw(4) <<
			maxpow << 'W';
			maxstep = 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

int main(int argc, char *argv[])
try {
	std::string i2cpath, confpath;
	int delay, step;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for INA219 test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the I2C device file to use
				"dev",
				boost::program_options::value<std::string>(&i2cpath)->
					default_value("/dev/i2c-1"),
				"Specify SMBus device file"
			) // the time between samples
			(
				"delay,d",
				boost::program_options::value<int>(&delay)->
					default_value(10),
				"Time in milliseconds bewteen samples"
			)
			(
				"step,s",
				boost::program_options::value<int>(&step)->
					default_value(100),
				"The number of samples between LCD updates"
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
			std::cout << "Test program for using an INA219 and displaying the "
			"results to a HD44780\n" << argv[0] << " [options]\n" << optdesc
			<< std::endl;
			return 0;
		}
	}
	std::unique_ptr<duds::hardware::interface::Smbus> smbus(
		new duds::hardware::interface::linux::DevSmbus(
			i2cpath,
			0x40,
			duds::hardware::interface::Smbus::NoPec()
		)
	);
	duds::hardware::devices::instruments::INA219 meter(smbus, 0.1);
	{
		std::string imgpath(argv[0]);
		int found = 0;
		while (!imgpath.empty() && (found < 3)) {
			imgpath.pop_back();
			if (imgpath.back() == '/') {
				++found;
			}
		}
		// load some icons before messing with hardware
		imgArc.load(imgpath + "images/numberparts.bppia");
	}

	// read in digital pin config
	boost::property_tree::ptree tree;
	boost::property_tree::read_info(confpath, tree);
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	duds::hardware::interface::PinConfiguration pc(pinconf);

	// configure display
	#ifdef USE_SYSFS_PORT
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		duds::hardware::interface::linux::SysFsPort::makeConfiguredPort(pc);
	#else
	std::shared_ptr<duds::hardware::interface::linux::GpioDevPort> port =
		duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(pc);
	#endif
	duds::hardware::interface::DigitalPinSet lcdset;
	duds::hardware::interface::ChipSelect lcdsel;
	pc.getPinSetAndSelect(lcdset, lcdsel, "lcdText");

	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			std::move(lcdset), std::move(lcdsel), 20, 4
		);
	tmd->initialize();
	std::thread doit(runtest, std::ref(meter), std::ref(tmd), delay, step);
	std::cin.get();
	quit = true;
	doit.join();
} catch (...) {
	std::cerr << "ERROR: " << boost::current_exception_diagnostic_information()
	<< std::endl;
	return 1;
}
