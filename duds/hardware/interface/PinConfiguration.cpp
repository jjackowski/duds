/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <duds/hardware/interface/ChipBinarySelectManager.hpp>
#include <duds/hardware/interface/ChipMultiplexerSelectManager.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/interface/ChipPinSetSelectManager.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdlib>

namespace duds { namespace hardware { namespace interface {

PinConfiguration::Pin::Pin() : parent(nullptr) { }

PinConfiguration::Pin::Pin(
	const std::pair<const std::string, boost::property_tree::ptree> &item,
	Port *owner
) {
	parse(item, owner);
}

static unsigned int ParsePinId(const std::string &str) {
	// check for "none"
	if (str == "none") {
		// explicitly no pin
		return PinConfiguration::Pin::NoPin;
	}
	// parse as a number; may be global or port ID
	std::istringstream iss(str);
	unsigned int ui;
	iss >> ui;
	// got something?
	if (!iss.fail()) {
		return ui;
	}
	// no real clue
	DUDS_THROW_EXCEPTION(PinBadIdError() << PinBadId(str));
}

void PinConfiguration::Pin::parse(
	const boost::property_tree::ptree::value_type &item,
	Port *owner
) {
	// read in key for port ID
	pid = ParsePinId(item.first);
	// read optional modified ID
	std::string val = item.second.get_value<std::string>();
	if (!val.empty()) {
		gid = ParsePinId(val);
	} else {
		gid = pid + owner->idOffset;
	}
	// a name is in a child node
	if (!item.second.empty()) {
		name = item.second.front().first;
	}
	parent = owner;
}

PinConfiguration::Port::Port() : idOffset(0) { }

void PinConfiguration::Port::parse(
	const boost::property_tree::ptree::value_type &item
) {
	typeval = item.second.get_value<std::string>();
	try {
		for (auto const &subitem : item.second) {
			if (subitem.first == "idoffset") {
				idOffset = subitem.second.get_value<unsigned int>();
			} else {
				// parse the pin data; may throw; then store it
				std::pair<
					PinConfiguration::Pins::index<index_gid>::type::iterator,
					bool
				> npin =
				pins.insert(PinConfiguration::Pin(subitem, this));
				// global ID must not be less than the port's ID offset
				if (npin.first->gid < idOffset) {
					DUDS_THROW_EXCEPTION(PortBadPinIdError() <<
						PortName(item.first) <<
						PortPinId(npin.first->gid)
					);
				}
				if (npin.first->pid < idOffset) {
					DUDS_THROW_EXCEPTION(PortBadPinIdError() <<
						PortName(item.first) <<
						PortPinId(npin.first->pid)
					);
				}
			}
		}
	} catch (boost::exception &be) {
		be << PortName(item.first);
		throw;
	}

	// check for duplicate IDs
	unsigned int last = -1;
	for (auto &gpin : gidIndex()) {
		if (last == gpin.gid) {
			DUDS_THROW_EXCEPTION(PortDuplicatePinIdError() <<
				PortName(item.first) << PortPinId(last)
			);
		}
		last = gpin.gid;
	}
}

const PinConfiguration::Pin &PinConfiguration::pin(
	const std::string &str
) const {
	std::istringstream iss(str);
	unsigned int ui;
	iss >> ui;
	// got nothing?
	if (iss.fail()) {
		// lookup name
		const PinConfiguration::Pins::index<index_name>::type &nameidx =
			allpins.get<index_name>();
		auto pi = nameidx.find(str);
		// check for non-existing pin
		if (pi == nameidx.end()) {
			DUDS_THROW_EXCEPTION(PinBadIdError() << PinBadId(str));
		}
		// found it
		return *pi;
	}
	// lookup GID to ensure it exists
	const PinConfiguration::Pins::index<index_gid>::type &gididx =
		allpins.get<index_gid>();
	auto gi = gididx.find(ui);
	if (gi == gididx.end()) {
		DUDS_THROW_EXCEPTION(PinBadIdError() << PinBadId(str));
	}
	return *gi;
}

PinConfiguration::SelMgr::SelMgr() :
usePort(nullptr), type(Unknown), selStates(0) { }

static bool ParseState(const std::string &val) {
	if ((val == "0") || (val == "low")) {
		return false;
	} else if ((val == "1") || (val == "high")) {
		return true;
	}
	// bad input
	DUDS_THROW_EXCEPTION(SelectBadStateError() << SelectBadState(val));
}

PinConfiguration::SelMgr::SelMgr(
	const boost::property_tree::ptree::value_type &item,
	const PinConfiguration *pinconf
) : usePort(nullptr), selStates(0) {
	{ // parse type string
		std::string typestr = item.second.get_value<std::string>();
		if (typestr == "Binary") {
			type = Binary;
		} else if (typestr == "Multiplexer") {
			type = Multiplexer;
		} else if (typestr == "Pin") {
			type = Pin;
		} else if (typestr == "PinSet") {
			type = PinSet;
		} else {
			// type affects parsing, so this must be an error
			DUDS_THROW_EXCEPTION(SelectManagerUnknownTypeError() <<
				SelectBadType(typestr)
			);
		}
	}
	// parse based on chip select type
	switch (type) {
		case Binary: {
			// find pin to use
			std::string pid = item.second.get<std::string>("pin");
			pins.reserve(1);
			const PinConfiguration::Pin &p = pinconf->pin(pid);
			usePort = p.parent;
			pins.push_back(p.gid);
			// parse the two select states and check for duplicates
			std::string low(item.second.get<std::string>("low", ""));
			if (!low.empty() && pinconf->haveChipSelect(low)) {
				DUDS_THROW_EXCEPTION(SelectDuplicateError() <<
					SelectName(low)
				);
			}
			std::string high(item.second.get<std::string>("high", ""));
			if (!high.empty() && (pinconf->haveChipSelect(high) || (high == low))) {
				DUDS_THROW_EXCEPTION(SelectDuplicateError() <<
					SelectName(high)
				);
			}
			// parse initial state
			initSelHigh = ParseState(item.second.get<std::string>("init", "0"));
			// store data
			if (!low.empty()) {
				selNames[low] = 0;
			}
			if (!high.empty()) {
				selNames[high] = 1;
			}
		}
		break;
		case Multiplexer: {
			boost::property_tree::ptree::const_assoc_iterator piter =
				item.second.find("pins");
			// check for missing pins subtree
			if (piter == item.second.not_found()) {
				// the subtree is required
				DUDS_THROW_EXCEPTION(SelectNoPinsError());
			}
			// inspect the pins subtree
			for (auto const &pinitem : piter->second) {
				// get the item's value
				std::string pn = pinitem.second.get_value<std::string>();
				// may have been ommitted
				if (pn.empty()) {
					// use the item's key instead
					pn = pinitem.first;
				}
				// find the pin
				const PinConfiguration::Pin &p = pinconf->pin(pn);
				// port check
				if (!usePort) {
					usePort = p.parent;
				} else if (usePort != p.parent) {
					DUDS_THROW_EXCEPTION(SelectMultiplePortsError() <<
						PortPinId(p.gid) << PinBadId(pn) // << port name(s) ??
					);
				}
				// store pin
				pins.push_back(p.gid);
			}
			// must have pin(s)
			if (pins.empty()) {
				DUDS_THROW_EXCEPTION(SelectNoPinsError());
			}
			// inspect the select
			piter = item.second.find("selects");
			// if "selects" is found . . .
			if (piter != item.second.not_found()) {
				// . . . iterate over its children
				for (auto const &selitem : piter->second) {
					// check for duplicates
					if (selNames.count(selitem.first) ||
						pinconf->haveChipSelect(selitem.first)
					) {
						DUDS_THROW_EXCEPTION(SelectDuplicateError() <<
							SelectName(selitem.first)
						);
					}
					// store it
					selNames[selitem.first] = selitem.second.get_value<unsigned int>();
				}
			}
		}
		break;
		case Pin: {
			// find optional name
			std::string name = item.second.get<std::string>("name", "");
			if (!name.empty()) {
				// check for duplicates
				if (pinconf->haveChipSelect(name)) {
					DUDS_THROW_EXCEPTION(SelectDuplicateError() <<
						SelectName(name)
					);
				}
				// store name
				selNames[name] = 1;
			}
			// parse the pin data
			pins.reserve(1);
			const PinConfiguration::Pin &p = pinconf->pin(
				item.second.get<std::string>("pin")
			);
			usePort = p.parent;
			pins.push_back(p.gid);
			initSelHigh = ParseState(item.second.get<std::string>("select", "0"));
		}
		break;
		case PinSet: {
			// inspect the selections
			int spot = 0;
			for (auto const &selitem : item.second) {
				// check for duplicates
				if (selNames.count(selitem.first) ||
					pinconf->haveChipSelect(selitem.first)
				) {
					DUDS_THROW_EXCEPTION(SelectDuplicateError() <<
						SelectName(selitem.first)
					);
				}
				// get the pin name to use
				std::string pn = selitem.second.get_value<std::string>();
				// may be in a subtree
				if (pn.empty()) {
					pn = selitem.second.get<std::string>("pin", selitem.first);
					// can also specify a non-default select state
					bool sstate = ParseState(
						selitem.second.get<std::string>("select", "0")
					);
					if (sstate) {
						selStates |= 1 << spot;
					}
				}
				// find the pin
				const PinConfiguration::Pin &p = pinconf->pin(pn);
				// port check
				if (!usePort) {
					usePort = p.parent;
				} else if (usePort != p.parent) {
					DUDS_THROW_EXCEPTION(SelectMultiplePortsError() <<
						PortPinId(p.gid) << PinBadId(pn) // << port name(s) ??
					);
				}
				// store pin
				pins.push_back(p.gid);
				// store select
				selNames[selitem.first] = spot;
				++spot;
			}
			// must have pin(s)
			if (pins.empty()) {
				DUDS_THROW_EXCEPTION(SelectNoPinsError());
			}
		}
	}
}

PinConfiguration::ChipSel::ChipSel(SelMgr *m, int id) : mgr(m), chipId(id) { }

PinConfiguration::PinSet::PinSet(
	const boost::property_tree::ptree::value_type &item,
	const PinConfiguration *pinconf
) : usePort(nullptr) {
	// check for pins select line
	boost::property_tree::ptree::const_assoc_iterator piter =
		item.second.find("pins");
	bool toplevelpins = piter == item.second.not_found();
	const boost::property_tree::ptree &pinlevel =
		toplevelpins ? item.second : piter->second;
	// iterate over pins
	for (auto const &pinitem : pinlevel) {
		Pin pin;
		// get the item's value
		std::string pn = pinitem.second.get_value<std::string>();
		// may have been ommitted
		if (pn.empty()) {
			// use the item's key instead
			pn = pinitem.first;
		} else {
			// the key is the name
			pin.name = pinitem.first;
		}
		// find the pin
		const PinConfiguration::Pin &p = pinconf->pin(pn);
		// port check
		if (!usePort) {
			usePort = p.parent;
		} else if (usePort != p.parent) {
			DUDS_THROW_EXCEPTION(SetMultiplePortsError() <<
				PortPinId(p.gid) << PinBadId(pn) // << port name(s) ??
			);
		}
		// finalize data using found pin
		pin.parent = usePort;
		pin.gid = p.gid;
		pin.pid = p.pid; // probably not useful, but copy anyway
		// store pin
		pins.insert(std::move(pin));
	}
	assert(pins.size() == pinlevel.size());
	assert(usePort);
	if (!toplevelpins) {
		// check for optional select line
		selName = item.second.get<std::string>("select", "");
		// see if it is specified, but does not exist
		if (!selName.empty() && !pinconf->haveChipSelect(selName)) {
			DUDS_THROW_EXCEPTION(SelectDoesNotExistError() <<
				SelectName(selName)
			);
		}
	}
}

PinConfiguration::PinConfiguration(const boost::property_tree::ptree &pt) {
	parse(pt);
}

void PinConfiguration::parse(const boost::property_tree::ptree &pt) {
	// parse the ports section
	boost::property_tree::ptree::const_assoc_iterator citer = pt.find("ports");
	if (citer != pt.not_found()) {
		Pins::index<index_seq>::type &seqidx = allpins.get<index_seq>();
		for (auto const &subtree : citer->second) {
			// check for a repeated name
			if (ports.count(subtree.first)) {
				DUDS_THROW_EXCEPTION(PortDuplicateError() <<
					PortName(subtree.first)
				);
			}
			// parse and store port data
			Port &port = ports[subtree.first];
			port.parse(subtree);
			// populate all pins
			Pins::index<index_seq>::type &seqpin =
				port.pins.get<index_seq>();
			seqidx.insert(seqidx.end(), seqpin.begin(), seqpin.end());
		}
	}
	// parse the selects section
	citer = pt.find("selects");
	if (citer != pt.not_found()) {
		for (auto const &subtree : citer->second) {
			// check for a repeated name
			if (selMgrs.count(subtree.first)) {
				DUDS_THROW_EXCEPTION(SelectManagerDuplicateError() <<
					SelectManagerName(subtree.first)
				);
			}
			// ensure all errors include select manager name
			try {
				// parse and store port data
				std::pair<SelMgrMap::iterator, bool> itbo = selMgrs.emplace(
					SelMgrMap::value_type(subtree.first, SelMgr(subtree, this))
				);
				// maintain all chip selects in this object
				for (auto &sni : itbo.first->second.selNames) {
					chipSels[sni.first] = ChipSel(
						&(itbo.first->second), sni.second
					);
				}
			} catch (boost::exception &be) {
				be << SelectManagerName(subtree.first);
				throw;
			}
		}
	}
	// parse the sets section
	citer = pt.find("sets");
	if (citer != pt.not_found()) {
		for (auto const &subtree : citer->second) {
			// check for a repeated name
			if (pinSets.count(subtree.first)) {
				DUDS_THROW_EXCEPTION(SetDuplicateError() <<
					SetName(subtree.first)
				);
			}
			// ensure all errors include pin set name
			try {
				// parse and store port data
				std::pair<PinSetMap::iterator, bool> itbo = pinSets.emplace(
					PinSetMap::value_type(subtree.first, PinSet(subtree, this))
				);
			} catch (boost::exception &be) {
				be << SetName(subtree.first);
				throw;
			}
		}
	}
}

void PinConfiguration::attachPort(
	const std::shared_ptr<DigitalPort> &dp,
	const std::string &name
) {
	// no port?
	if (!dp) {
		DUDS_THROW_EXCEPTION(DigitalPortDoesNotExistError());
	}
	// check for named port not in the parsed config data
	if (!ports.count(name)) {
		DUDS_THROW_EXCEPTION(PortDoesNotExistError() <<
			PortName(name)
		);
	}
	Port &port = ports[name];
	Port *dbgPort = &(ports[name]);
	assert(dbgPort == &port);
	// check for a compatible set of pins (really just exist or not)
	Pins::index<index_gid>::type &gididx = port.pins.get<index_gid>();
	for (auto const &pin : gididx) {
		// must not be present, but is?
		if ((pin.pid == Pin::NoPin) && dp->exists(pin.gid)) {
			DUDS_THROW_EXCEPTION(DigitalPortHasPinError() <<
				PortName(name) << PinErrorId(pin.gid)
			);
		}
		// must be present, but isn't?
		else if ((pin.pid != Pin::NoPin) && !dp->exists(pin.gid)) {
			DUDS_THROW_EXCEPTION(DigitalPortLacksPinError() <<
				PortName(name) << PinErrorId(pin.gid)
			);
		}
		// if running here, must be ok so far
	}
	// attempt to create select managers and their select objects
	for (auto &mgr : selMgrs) {
		// skip if wrong port
		if (mgr.second.usePort != &port) {
			continue;
		}
		assert(!mgr.second.csm); // shouldn't already exist
		assert(!mgr.second.pins.empty());
		assert(
			(mgr.second.type == SelMgr::Multiplexer) ||
			(mgr.second.type == SelMgr::PinSet) ||
			(mgr.second.pins.size() == 1)
		);
		// make the manager object and give it an access object from the port
		switch (mgr.second.type) {
			case SelMgr::Binary:
				mgr.second.csm = std::make_shared<ChipBinarySelectManager>(
					dp->access(mgr.second.pins.front()), mgr.second.initSelHigh
				);
			break;
			case SelMgr::Multiplexer:
				mgr.second.csm = std::make_shared<ChipMultiplexerSelectManager>(
					dp->access(mgr.second.pins)
				);
			break;
			case SelMgr::Pin:
				mgr.second.csm = std::make_shared<ChipPinSelectManager>(
					dp->access(mgr.second.pins.front()),
					mgr.second.initSelHigh ?
						ChipPinSelectManager::SelectHigh :
						ChipPinSelectManager::SelectLow
				);
			break;
			case SelMgr::PinSet:
				mgr.second.csm = std::make_shared<ChipPinSetSelectManager>(
					dp->access(mgr.second.pins),
					mgr.second.selStates
				);
			break;
			default:
				// !!!! corrupt data
				abort();
		}
		// create any chip selects
		for (auto const &sel : mgr.second.selNames) {
			ChipSel &cs = chipSels[sel.first];
			assert(cs.chipId == sel.second);
			cs.sel.modify(mgr.second.csm, sel.second);
		}
	}
	// attempt to create pin set objects
	for (auto &pset : pinSets) {
		// skip if wrong port
		if (pset.second.usePort != &port) {
			continue;
		}
		// put pin IDs in a vector
		std::vector<unsigned int> pvec;
		pvec.reserve(pset.second.pins.size());
		for (auto const &pin : pset.second.seqIndex()) {
			assert(pin.parent == &port);
			pvec.push_back(pin.gid);
		}
		// make the set
		pset.second.dpSet = DigitalPinSet(dp, std::move(pvec));
	}
	// store port for later reference
	port.dport = dp;
}

const PinConfiguration::Port &PinConfiguration::port(
	const std::string &name
) const {
	PortMap::const_iterator iter = ports.find(name);
	if (iter == ports.end()) {
		DUDS_THROW_EXCEPTION(PortDoesNotExistError() <<
			PortName(name)
		);
	}
	return iter->second;
}

const PinConfiguration::PinSet &PinConfiguration::pinSet(
	const std::string &name
) const {
	PinSetMap::const_iterator iter = pinSets.find(name);
	if (iter == pinSets.end()) {
		DUDS_THROW_EXCEPTION(SetDoesNotExistError() <<
			SetName(name)
		);
	}
	return iter->second;
}

const PinConfiguration::ChipSel &PinConfiguration::chipSelect(
	const std::string &name
) const {
	ChipSelMap::const_iterator iter = chipSels.find(name);
	if (iter == chipSels.end()) {
		DUDS_THROW_EXCEPTION(SelectDoesNotExistError() <<
			SelectName(name)
		);
	}
	return iter->second;
}

const PinConfiguration::SelMgr &PinConfiguration::selectManager(
	const std::string &name
) const {
	SelMgrMap::const_iterator iter = selMgrs.find(name);
	if (iter == selMgrs.end()) {
		DUDS_THROW_EXCEPTION(SelectManagerDoesNotExistError() <<
			SelectManagerName(name)
		);
	}
	return iter->second;
}

void PinConfiguration::getPinSetAndSelect(
	DigitalPinSet &dpset,
	ChipSelect &sel,
	const std::string &setName
) const {
	PinSetMap::const_iterator piter = pinSets.find(setName);
	if (piter == pinSets.end()) {
		DUDS_THROW_EXCEPTION(SetDoesNotExistError() << SetName(setName));
	}
	if (!piter->second.dpSet.havePins()) {
		DUDS_THROW_EXCEPTION(SetNotCreatedError() << SetName(setName));
	}
	dpset = piter->second.dpSet;
	if (piter->second.selName.empty()) {
		sel.reset();
	} else {
		ChipSelMap::const_iterator siter = chipSels.find(piter->second.selName);
		if (siter == chipSels.end()) {
			DUDS_THROW_EXCEPTION(SelectDoesNotExistError() <<
				SelectName(piter->second.selName)
			);
		}
		if (!siter->second.sel.haveManager()) {
			DUDS_THROW_EXCEPTION(SelectManagerNotCreated() <<
				SelectName(piter->second.selName)
			);
		}
		sel = siter->second.sel;
	}
}

const DigitalPinSet &PinConfiguration::getPinSet(
	const std::string &setName
) const {
	PinSetMap::const_iterator piter = pinSets.find(setName);
	if (piter == pinSets.end()) {
		DUDS_THROW_EXCEPTION(SetDoesNotExistError() << SetName(setName));
	}
	if (!piter->second.dpSet.havePins()) {
		DUDS_THROW_EXCEPTION(SetNotCreatedError() << SetName(setName));
	}
	return piter->second.dpSet;
}

const ChipSelect &PinConfiguration::getSelect(
	const std::string &selName
) const {
	ChipSelMap::const_iterator siter = chipSels.find(selName);
	if (siter == chipSels.end()) {
		DUDS_THROW_EXCEPTION(SelectDoesNotExistError() << SelectName(selName));
	}
	if (!siter->second.sel.haveManager()) {
		DUDS_THROW_EXCEPTION(SelectManagerNotCreated() << SelectName(selName));
	}
	return siter->second.sel;
}

DigitalPin PinConfiguration::getPin(
	const std::string &pinName
) const {
	const PinConfiguration::Pins::index<index_name>::type &nidx =
		allpins.get<index_name>();
	PinConfiguration::Pins::index<index_name>::type::iterator niter =
		nidx.find(pinName);
	if (niter == nidx.end()) {
		DUDS_THROW_EXCEPTION(PinBadIdError() << PinBadId(pinName));
	}
	if (!niter->parent || !niter->parent->dport) {
		DUDS_THROW_EXCEPTION(PortDoesNotExistError() << PinBadId(pinName));
	}
	return DigitalPin(niter->parent->dport, niter->gid);
}

bool PinConfiguration::getPin(
	DigitalPin &dest,
	const std::string &pinName
) const {
	const PinConfiguration::Pins::index<index_name>::type &nidx =
		allpins.get<index_name>();
	PinConfiguration::Pins::index<index_name>::type::iterator niter =
		nidx.find(pinName);
	if ((niter == nidx.end()) || !niter->parent || !niter->parent->dport) {
		dest.reset();
		return false;
	}
	dest = DigitalPin(niter->parent->dport, niter->gid);
	return true;
}

bool PinConfiguration::havePin(
	const std::string &pinName
) const {
	const PinConfiguration::Pins::index<index_name>::type &nidx =
		allpins.get<index_name>();
	PinConfiguration::Pins::index<index_name>::type::iterator niter =
		nidx.find(pinName);
	return (niter != nidx.end()) && niter->parent && !niter->parent->dport;
}

} } }
