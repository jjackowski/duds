/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/test/VirtualPort.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>

namespace duds { namespace hardware { namespace interface { namespace test {

VirtualPort::VirtualPort(
	unsigned int numpins,
	unsigned int firstid
) : DigitalPortIndependentPins(numpins, firstid) {
	for (std::uint32_t pidx = 0; pidx < numpins; ++pidx) {
		initPin(pidx, pidx);
	}
}

VirtualPort::VirtualPort(
	const std::vector<unsigned int> &ids,
	unsigned int firstid
) : DigitalPortIndependentPins(ids.size(), firstid) {
	std::vector<unsigned int>::const_iterator iter = ids.begin();
	for (unsigned int pid = 0; iter != ids.end(); ++iter, ++pid) {
		initPin(*iter, pid);
	}
}

std::shared_ptr<VirtualPort> VirtualPort::makeConfiguredPort(
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
		if (pin.gid > next) {
			// add unavailable pins
			gpios.insert(gpios.end(), pin.gid - next, -1);
		}
		// add available pin
		gpios.push_back(pin.pid);
		next = pin.gid + 1;
	}
	std::shared_ptr<VirtualPort> sp = std::make_shared<VirtualPort>(
		gpios,
		port.idOffset
	);
	pc.attachPort(sp, name);
	return sp;
}

VirtualPort::~VirtualPort() {
	shutdown();
}

void VirtualPort::initPin(std::uint32_t offset, unsigned int pid) {

	//std::cout << "VirtualPort::initPin(), offset(gid) = " << offset <<
	//"  pid = " << pid << std::endl;

	if (offset == -1) {
		// line cannot be used
		pins[pid].markNonexistent();
		return;
	}
	pins[pid].conf.options = DigitalPinConfig::DirInput;
	pins[pid].cap.capabilities =
		DigitalPinCap::Input |
		DigitalPinCap::OutputPushPull  /*|
		DigitalPinCap::EventEdgeFalling |  // theses are not yet supported
		DigitalPinCap::EventEdgeRising |
		DigitalPinCap::EventEdgeChange |
		DigitalPinCap::InterruptOnEvent */;
	// no data on output currents
	pins[pid].cap.maxOutputCurrent = 0;
}

bool VirtualPort::simultaneousOperations() const {
	return true;
}

void VirtualPort::configurePort(
	unsigned int lid,
	const DigitalPinConfig &cfg,
	DigitalPinAccessBase::PortData *
) {
	DigitalPinConfig &dpc = pins[lid].conf;
	// change in config?
	if (
		(dpc.options & DigitalPinConfig::DirMask) !=
		(cfg & DigitalPinConfig::DirMask)
	) {
		if (cfg & DigitalPinConfig::DirInput) {
			//gr->inputOffset(chipFd, lid);
		} else if (cfg & DigitalPinConfig::DirOutput) {
			/*gr->outputOffset(
				chipFd,
				lid,
				dpc.options & DigitalPinConfig::OutputState
			);*/
		}
	}
}

bool VirtualPort::inputImpl(
	unsigned int gid,
	DigitalPinAccessBase::PortData *
) {
	/** @todo  Provide a way for test code to set the input result. */
	int lid = localId(gid);
	pins[lid].conf.options.setTo(DigitalPinConfig::InputState, true);
	return true;
}

std::vector<bool> VirtualPort::inputImpl(
	const std::vector<unsigned int> &pvec,
	DigitalPinAccessBase::PortData *
) {
	/** @todo  Provide a way for test code to set the input result. */
	// return input states
	std::vector<bool> outv;
	outv.reserve(pvec.size());
	int idx = 0;
	for (const unsigned int &gid : pvec) {
		outv.push_back(
			(pins[localId(gid)].conf.options & DigitalPinConfig::InputState) > 0
		);
	}
	return outv;
}

void VirtualPort::outputImpl(
	unsigned int lid,
	bool state,
	DigitalPinAccessBase::PortData *
) {
	// store new state
	pins[lid].conf.options.setTo(DigitalPinConfig::OutputState, state);
}

void VirtualPort::outputImpl(
	const std::vector<unsigned int> &pvec,
	const std::vector<bool> &state,
	DigitalPinAccessBase::PortData *
) {
	// loop through all pins to alter
	std::vector<unsigned int>::const_iterator piter = pvec.begin();
	std::vector<bool>::const_iterator siter = state.begin();
	for (; piter != pvec.end(); ++piter, ++siter) {
		// store new state
		pins[*piter].conf.options.setTo(DigitalPinConfig::OutputState, *siter);
	}
}

} } } } // namespaces
