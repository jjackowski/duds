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
 * Tests the EvdevInput class. It requires access to an input device file
 * (/dev/input/event[0-9]+) specified as the first argument. It only looks
 * for a small number of events. If found, it will show their values when they
 * change. Use ctrl-c to exit.
 */

#include <duds/os/linux/EvdevErrors.hpp>
#include <duds/os/linux/EvdevInput.hpp>
#include <iostream>
#include <thread>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/program_options.hpp>
#include <csignal>

std::sig_atomic_t quit = 0;

void signalHandler(int) {
	quit = 1;
}

namespace duds { namespace os { namespace linux {

	std::ostream &operator << (std::ostream &os, EventTypeCode etc) {
		static const std::string unknown("Unknown");
		os << etc.typeName(unknown) << ':' << etc.codeName(unknown);
		return os;
	}

} } }

// these are the events that will be shown by this sample program
const duds::os::linux::EventTypeCode events[] = {
	duds::os::linux::EventTypeCode(EV_KEY, KEY_LEFT),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_RIGHT),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_UP),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_DOWN),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_HOME),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_END),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_LEFT),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_PAGEUP),
	duds::os::linux::EventTypeCode(EV_KEY, KEY_PAGEDOWN),
	duds::os::linux::EventTypeCode(EV_KEY, BTN_LEFT),
	duds::os::linux::EventTypeCode(EV_KEY, BTN_RIGHT),
	duds::os::linux::EventTypeCode(EV_KEY, BTN_MIDDLE),
	duds::os::linux::EventTypeCode(EV_ABS, ABS_X),
	duds::os::linux::EventTypeCode(EV_ABS, ABS_Y),
	duds::os::linux::EventTypeCode(EV_REL, REL_X),
	duds::os::linux::EventTypeCode(EV_REL, REL_Y)
};

void show(duds::os::linux::EventTypeCode etc, std::int32_t val) {
	std::cout << "Event " << etc << ", value " << val << std::endl;
}

int main(int argc, char *argv[])
try {
	std::string devpath;
	int delay;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for EvdevInput test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the device file to use
				"dev,i",
				boost::program_options::value<std::string>(&devpath), //->required(),
				"Specify input device file"
			)
		;
		boost::program_options::positional_options_description posoptdesc;
		posoptdesc.add("dev", 1);
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::command_line_parser(argc, argv).
			options(optdesc).positional(posoptdesc).run(),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test program for EvdevInput class\n" << argv[0] <<
			" [options]\n" << optdesc << std::endl;
			return 0;
		}
	}
	duds::os::linux::EvdevInput evin;
	duds::os::linux::InputHandlersSptr inhan = evin.makeConnectedHandlers();
	try {
		evin.open(devpath);
	} catch (duds::os::linux::EvdevFileOpenError &) {
		std::cerr << "Failed to open device file " << devpath << std::endl;
		return 2;
	}  catch (duds::os::linux::EvdevInitError &eie) {
		std::cerr << "Failed to initalize libevdev, error code " <<
		*boost::get_error_info<boost::errinfo_errno>(eie) << std::endl;
		return 3;
	}
	int cnt = 0;
	for (auto event : events) {
		if (evin.hasEvent(event)) {
			inhan->connect(event, &show);
			std::cout << "Found event " << event << std::endl;
			++cnt;
		}
	}
	std::cout << "Found " << cnt << " events" << std::endl;
	if (!cnt) {
		return 0;
	}
	std::signal(SIGINT, &signalHandler);
	std::signal(SIGTERM, &signalHandler);
	while (!quit) {
		if (evin.eventsAvailable()) {
			evin.respondToNextEvent();
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
	std::cout << "Bye!" << std::endl;
	return 0;
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
