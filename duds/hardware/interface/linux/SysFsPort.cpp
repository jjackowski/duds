/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/exception/errinfo_file_name.hpp>
#include <sstream>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>

namespace duds { namespace hardware { namespace interface { namespace linux {

static const char *prefix = "/sys/class/gpio/gpio";

SysFsPort::SysFsPort(const std::vector<unsigned int> &ids, unsigned int firstid) :
DigitalPortIndependentPins(ids.size(), firstid), fspins(ids.size()) {
	PinVector::iterator piter = pins.begin();
	FsPinVector::iterator fiter = fspins.begin();
	std::vector<unsigned int>::const_iterator iiter = ids.cbegin();
	for (; piter != pins.end(); ++firstid, ++iiter, ++fiter, ++piter) {
		try {
			fiter->open(piter->conf, piter->cap, *iiter);
		} catch (PinError &pe) {
			pe << PinErrorId(firstid);
			throw;
		}
	}
}

std::shared_ptr<SysFsPort> SysFsPort::makeConfiguredPort(
	PinConfiguration &pc,
	const std::string &name
) {
	// find the port's config object
	const PinConfiguration::Port &port = pc.port(name);
	// enumerate the pins
	std::vector<unsigned int> gpios;
	unsigned int next = port.idOffset;
	gpios.reserve(port.pins.size());
	for (auto const &pin : port.gidIndex()) {
		// need empty spots?
		if (pin.gid < next) {
			// add unavailable pins
			gpios.insert(gpios.end(), pin.gid - next, -1);
		}
		// add available pin
		gpios.push_back(pin.pid);
		next = pin.gid + 1;
	}
	std::shared_ptr<SysFsPort> sp = std::make_shared<SysFsPort>(
		gpios,       // <--- CHANGE this to a move
		port.idOffset
	);
	pc.attachPort(sp, name);
	return sp;
}

SysFsPort::~SysFsPort() {
	shutdown();
	// more stuff here?
}

bool SysFsPort::simultaneousOperations() const {
	return false;
}

void SysFsPort::configurePort(
	unsigned int localPinId,
	const DigitalPinConfig &cfg
) {
	assert(cfg & (DigitalPinConfig::DirInput | DigitalPinConfig::DirOutput));
	try {
		// change direction
		fspins[localPinId].setDirection(cfg & DigitalPinConfig::DirOutput);
	} catch (PinError &pe) {
		pe << PinErrorId(globalId(localPinId));
		throw;
	}
}

bool SysFsPort::inputImpl(unsigned int gid)
try {
	return fspins[localId(gid)].read();
} catch (PinError &pe) {
	pe << PinErrorId(gid);
	throw;
}

void SysFsPort::outputImpl(unsigned int lid, bool state)
try {
	fspins[lid].write(state);
} catch (PinError &pe) {
	pe << PinErrorId(globalId(lid));
	throw;
}

void SysFsPort::FsPin::open(
	DigitalPinConfig &conf,  // uninitialized
	DigitalPinCap &cap,      // uninitialized
	unsigned int pin
) {
	fsid = pin;
	/** @todo  Flag events and interrupts when support is added. */
	// initialize the configuration and capability values to clear / nonexistent
	conf = DigitalPinConfig(DigitalPinConfig::ClearAll());
	cap = NonexistentDigitalPin;
	std::ostringstream fname;
	fname << prefix << pin << "/value";
	value.open(fname.str().c_str());
	bool noOutput = false;
	if (!value.is_open()) {
		// allow for input only
		value.open(fname.str().c_str(), std::ios_base::in);
		if (!value.is_open()) {
			DUDS_THROW_EXCEPTION(PinIoError() << SysFsPinErrorId(pin) <<
				boost::errinfo_file_name(fname.str())
			);
		}
		// cannot change the value; still good for input
		noOutput = true;
	} else {
		// obtain current pin value
		curoutval = reqoutval = read();
	}
	// unwrite "value"
	fname.seekp(-5, std::ios_base::cur);
	fname << "direction";
	std::string dir;
	direction.open(fname.str().c_str());
	if (!direction.is_open()) {
		// allow for no direction writes
		direction.open(fname.str().c_str(), std::ios_base::in);
		if (!direction.is_open()) {
			value.close();
			DUDS_THROW_EXCEPTION(PinIoError() << SysFsPinErrorId(pin) <<
				boost::errinfo_file_name(fname.str())
			);
		}
		// read current direction
		direction >> dir;
		// no longer needed
		direction.close();
	} else {
		// read current direction
		direction >> dir;
	}
	// parse direction for initial setting & caps
	if (dir == "in") {  // currently input
		isoutput = false;
		// stuck on input?
		if (!direction.is_open()) {
			noOutput = true;
		}
		cap.capabilities |= DigitalPinCap::Input;
		conf.options |= DigitalPinConfig::DirInput;
	} else if (dir == "out") {  // currently output
		isoutput = true;
		// stuck on output?
		if (!direction.is_open()) {
			// check for non-writable value file
			if (noOutput) {
				// useless pin; an output that cannot be changed
				value.close();
				DUDS_THROW_EXCEPTION(PinUnsupportedOperation() <<
					SysFsPinErrorId(pin)
				);
			}
		} else {
			// can input
			cap.capabilities |= DigitalPinCap::Input;
		}
		conf.options |= DigitalPinConfig::DirOutput |
			DigitalPinConfig::OutputPushPull;
	} else {
		// unexpected value
		DUDS_THROW_EXCEPTION(PinIoError() << SysFsPinErrorId(pin));
	}
	// I don't like double negatives, but I'm not changing the logic to fix it.
	if (!noOutput) {
		// add output flag
		cap.capabilities |= DigitalPinCap::OutputPushPull;
	}
}

void SysFsPort::FsPin::setDirection(bool output) {
	// only change direction; do not set direction to what it already is
	if (output != isoutput) {
		if (output) {
			direction << "out" << std::endl;
		} else {
			direction << "in" << std::endl;
		}
		if (direction.fail()) {
			DUDS_THROW_EXCEPTION(PinIoError() <<
				SysFsPinErrorId(fsid)
			);
		} else if (output) {
			// assure the logic to avoid unneeded changes will see the next
			// write as a change
			curoutval = !reqoutval;
			write(reqoutval);
		}
		isoutput = output;
	}
}

bool SysFsPort::FsPin::read() {
	value.seekg(0);
	char v;
	value >> v;
	if (value.fail()) {
		DUDS_THROW_EXCEPTION(PinIoError() <<
			SysFsPinErrorId(fsid)
		);
	}
	return v == '1';
}

void SysFsPort::FsPin::write(bool w) {
	// record this as the requested output value; used when switching from
	// input to output
	reqoutval = w;
	// Changing the output results in an error if the pin is configured
	// as an input; the filesystem interface does not allow for specifying
	// a value to output ahead of switching to output.
	// Don't continue if the request is the same as the current output.
	if (isoutput && (w != curoutval)) {
		char v = '0';
		if (w) {
			++v;
		}
		value << v << std::endl;
		if (value.fail()) {
			DUDS_THROW_EXCEPTION(PinIoError() <<
				SysFsPinErrorId(fsid)
			);
		}
		// record this as the current output
		curoutval = w;
	}
}

} } } } // namespaces
