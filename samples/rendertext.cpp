/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * A sample demonstrating rendering text using bit-per-pixel graphics and
 * outputting the result to the console. Used to assist debugging some
 * unexpected text rendering results.
 */

#include <duds/hardware/devices/displays/SimulatedBppDisplay.hpp>
#include <duds/hardware/devices/displays/ST7920.hpp>
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <duds/ui/graphics/BppImageArchive.hpp>
#include <duds/ui/graphics/BppStringCache.hpp>
#include <iostream>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>


bool quit = false;

//namespace display = duds::hardware::display;

int main(int argc, char *argv[])
try {
	std::string fontpath, confpath, lcdname;
	std::string imgpath(argv[0]);
	int dispW, dispH;
	bool uselcd = false;
	{
		int found = 0;
		while (!imgpath.empty() && (found < 3)) {
			imgpath.pop_back();
			if (imgpath.back() == '/') {
				++found;
			}
		}
		imgpath += "images/";
	}
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for bit-per-pxiel text rendering test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			(
				"font",
				boost::program_options::value<std::string>(&fontpath)->
					default_value(imgpath + "font_8x16.bppia"),
				"Font file"
			)
			(
				"st7920",
				"Use a graphic ST7920 LCD instead of console output"
			)
			(
				"conf,c",
				boost::program_options::value<std::string>(&confpath)->
					default_value("samples/pins.conf"),
				"Pin configuration file; required if ST7920 LCD used"
			)
			(
				"lcdname",
				boost::program_options::value<std::string>(&lcdname)->
					default_value("lcdGraphic"),
				"Name of LCD inside pin configuration"
			)
			( // the display width
				"width,x",
				boost::program_options::value<int>(&dispW)->
					default_value(144),
				"ST7920 display width in pixels"
			)
			( // the display height
				"height,y",
				boost::program_options::value<int>(&dispH)->
					default_value(32),
				"ST7920 display height in pixels"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Bit-per-pxiel text rendering test\nOnce running, input "
			"text to render in stdin and press enter to render the text.\n"
			"Enter a blank line to quit.\n\t" << argv[0] << " [options]\n" <<
			optdesc << std::endl;
			return 0;
		}
		if (vm.count("st7920")) {
			uselcd = true;
		}
	}

	// load font
	duds::ui::graphics::BppFontSptr font =
		std::make_shared<duds::ui::graphics::BppFont>();
	font->load(fontpath);
	duds::ui::graphics::BppStringCache bsc(font);
	
	duds::ui::graphics::ImageDimensions cdim = font->estimatedMaxCharacterSize();
	std::cout << "Estimated character size is " << cdim << std::endl;
	
	std::shared_ptr<duds::hardware::devices::displays::ST7920> lcd;
	duds::hardware::devices::displays::SimulatedBppDisplay sd;
	// display configuration
	duds::hardware::interface::PinConfiguration pc;
	std::shared_ptr<duds::hardware::interface::DigitalPort> port;
	// ST7920
	if (uselcd) {
		boost::property_tree::ptree tree;
		boost::property_tree::read_info(confpath, tree);
		// if an exception is thrown here, the program will terminate without
		// getting to the catch block below; don't know why
		pc.parse(tree.get_child("pins"));
		port = duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(pc);
		duds::hardware::interface::DigitalPinSet lcdset;
		duds::hardware::interface::ChipSelect lcdsel;
		pc.getPinSetAndSelect(lcdset, lcdsel, lcdname);
		lcd = std::make_shared<duds::hardware::devices::displays::ST7920>(
			std::move(lcdset), std::move(lcdsel), dispW, dispH
			);
		lcd->initialize();
	}		

	char inbuf[256] = { 0 };
	do {
		std::cin.getline(inbuf, 255);
		if (std::cin.good() && inbuf[0]) {
			std::cout << "Rendering \"" << inbuf << "\"";
			// render the text
			duds::ui::graphics::ConstBppImageSptr label = bsc.text(inbuf);
			std::cout << ", size " << label->width() << 'x' << label->height() <<
			std::endl;
			assert(label->dimensions() == font->lineDimensions(inbuf));
			if (uselcd) {
				const duds::ui::graphics::ImageDimensions &dim = lcd->dimensions();
				duds::ui::graphics::BppImage frame(dim);
				frame.clearImage();
				duds::ui::graphics::ImageLocation loc(
					std::max((dim.w - label->width()) / 2, 0),
					std::max((dim.h - label->height()) / 2, 0)
				);
				frame.write(label, loc);
				lcd->write(&frame);
			} else {
				sd.configure(label->dimensions());
				sd.write(label);
			}
		}
	} while (std::cin.good() && inbuf[0]);

} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
