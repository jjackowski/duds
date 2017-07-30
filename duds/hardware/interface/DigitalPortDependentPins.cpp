/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPortDependentPins.hpp>

namespace duds { namespace hardware { namespace interface {

bool DigitalPortDependentPins::independentConfig() const {
	return false;
}

// may throw from DigitalPinCap::compatible()
bool DigitalPortDependentPins::proposeConfigImpl(
	const std::vector<unsigned int> &pvec,
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// inputs must match size, except initConf may be empty
	if ((propConf.size() != pvec.size()) || (
		!initConf.empty() && initConf.size() != pvec.size()
	)) {
		BOOST_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
			DigitalPortAffected(this)
		);
	}
	// put in inital values for the starting config if empty
	if (initConf.empty()) {
		initConf.insert(
			initConf.begin(),
			pvec.size(),
			DigitalPinConfig::OperationNoChange
		);
	}
	std::vector<DigitalPinConfig> pc(pins.size()), ic(pins.size());
	//std::vector<unsigned int> &vecp(pins.szie(), -1);
	std::vector<bool> visited(pins.size(), false);
	// translate from pin order in pvec to internal ordering
	std::vector<unsigned int>::const_iterator piter = pvec.cbegin();
	std::vector<DigitalPinConfig>::iterator pConfIter = propConf.begin();
	std::vector<DigitalPinConfig>::iterator iConfIter = initConf.begin();
	unsigned int pos = 0, neg = 0;
	for (; piter != pvec.cend(); ++piter, ++pos, ++pConfIter, ++iConfIter) {
		// multiple config check
		if (visited[*piter]) {
			BOOST_THROW_EXCEPTION(DigitalPinMultipleConfigError() <<
				PinErrorId(globalId(*piter)) << DigitalPortAffected(this)
			);
		}
		// skip -1's
		if (*piter == -1) {
			++neg;
		}
		// range & existence check
		else if ((*piter < pins.size()) && (pins[*piter] || (
			(*pConfIter == DigitalPinConfig::OperationNoChange) &&
			(*iConfIter == DigitalPinConfig::OperationNoChange)
		))) {
			// move configs into internal ordered vectors
			if (*iConfIter == DigitalPinConfig::OperationNoChange && pins[*piter]) {
				// fill with current config
				ic[*piter] = *iConfIter = pins[*piter].conf;
			} else {
				ic[*piter] = *iConfIter;
			}
			pc[*piter] = *pConfIter;
			// piter has next port-local pin ID; record reverse translation
			//vecp[*piter] = pos;
		} //else {
			// badness??
		//}
	}
	// not all port pins specified?
	if ((pvec.size() - neg) < pins.size()) {
		// find and fill in missing configs with current config
		std::vector<bool>::iterator viter = visited.begin();
		pConfIter = pc.begin();
		iConfIter = ic.begin();
		PinVector::const_iterator piniter = pins.cbegin();
		for (; viter != visited.end(); ++piter, ++pConfIter, ++iConfIter, ++piniter) {
			// not filled?
			if (!*viter) {
				// use current config for the initial & proposed configs
				*pConfIter = *iConfIter = piniter->conf;
			}
		}
	}
	bool good = true;
	// iterate over the config in specified pin order
	for (piter = pvec.cbegin(); piter != pvec.cend(); ++piter) {
		DigitalPinRejectedConfiguration::Reason err =
			DigitalPinRejectedConfiguration::NotRejected;
		// skip -1's
		if (*piter != -1) {
			// range & existence checks
			if ((*piter >= pins.size()) || (!pins[*piter] && (
				// non-existent & unsuable pins cannot be changed
				(pc[*piter] != DigitalPinConfig::OperationNoChange) ||
				(ic[*piter] != DigitalPinConfig::OperationNoChange)
			))) {
				// unusable
				err = DigitalPinRejectedConfiguration::Unsupported;
			} else if (pins[*piter]) {
				// referenced pin
				const PinEntry &pin = pins[*piter];
				DigitalPinConfig &pconf = pc[*piter];
				DigitalPinConfig &iconf = ic[*piter];
				// combine options
				pconf.reverseCombine(iconf);
				// test compatability - may throw
				DigitalPinRejectedConfiguration::Reason err = pin.cap.compatible(
					pconf
				);
				// check for changes to other pins if no error yet
				if (!err && !independentConfig(globalId(*piter), pconf, iconf)) {
					// check how the config may alter other pins
					err = inspectProposal(*piter, pc, ic);
				}
			}
			// error?
			if (err) {
				// flag it for return value
				good = false;
			}
		}
		// store reason if requested
		if (insertReason) {
			insertReason(err);
		}
	}
	// re-translate back to order in pvec
	pConfIter = propConf.begin();
	//iConfIter = initConf.begin();
	piter = pvec.cbegin();
	for (; piter != pvec.cend(); ++piter, ++pConfIter /*, ++iConfIter*/) {
		*pConfIter = pc[*piter];
		//*iConfIter = ic[*piter];
	}
	return good;
}

bool DigitalPortDependentPins::proposeFullConfigImpl(
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// inputs must match size of pins, except initConf may be empty
	if ((propConf.size() != pins.size()) || (
		!initConf.empty() && initConf.size() != pins.size()
	)) {
		BOOST_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
			DigitalPortAffected(this)
		);
	}
	// put in inital values for the starting config if empty
	if (initConf.empty()) {
		initConf = configurationImpl();
	}
	std::vector<DigitalPinConfig>::iterator pConfIter = propConf.begin();
	std::vector<DigitalPinConfig>::iterator iConfIter = initConf.begin();
	PinVector::const_iterator pvIter = pins.cbegin();
	unsigned int pos = 0;
	bool good = true;
	// iterate over the config
	for (; pvIter != pins.cend(); ++pos, ++iConfIter, ++ pConfIter, ++pvIter) {
		DigitalPinRejectedConfiguration::Reason err =
			DigitalPinRejectedConfiguration::NotRejected;
		// non-existence check
		if (!*pvIter && (
			// non-existent & unsuable pins cannot be changed
			(*pConfIter != DigitalPinConfig::OperationNoChange) ||
			(*iConfIter != DigitalPinConfig::OperationNoChange)
		)) {
			// unusable
			err = DigitalPinRejectedConfiguration::Unsupported;
		} else if (*pvIter) {
			// initial config unset?
			if (*iConfIter == DigitalPinConfig::OperationNoChange) {
				// set to current config
				*iConfIter = pvIter->conf;
			}
			// combine options
			pConfIter->reverseCombine(*iConfIter);
			// test compatability - may throw
			DigitalPinRejectedConfiguration::Reason err = pvIter->cap.compatible(
				*pConfIter
			);
			// check for changes to other pins if no error yet
			if (!err && !independentConfig(
				globalId(pos),
				*pConfIter,
				*iConfIter
			)) {
				// check how the config may alter other pins
				err = inspectProposal(pos, propConf, initConf);
			}
		}
		// error?
		if (err) {
			// flag it for return value
			good = false;
		}
		// store reason if requested
		if (insertReason) {
			insertReason(err);
		}
	}
	return good;
}

DigitalPinRejectedConfiguration::Reason
DigitalPortDependentPins::proposeConfigImpl(
	unsigned int gid,
	DigitalPinConfig &pconf,
	DigitalPinConfig &iconf
) const {
	// check range
	unsigned int lid = localId(gid);
	if ((lid >= pins.size()) || !pins[lid]) {
		// no pin
		return DigitalPinRejectedConfiguration::Unsupported;
	}
	if (iconf == DigitalPinConfig::OperationNoChange) {
		// use current config
		iconf = pins[lid].conf;
	}
	pconf.reverseCombine(iconf);
	// check for an independent configuration
	if (independentConfig(gid, pconf, iconf)) {
		// test compatability - may throw
		return pins[lid].cap.compatible(pconf);
	}
	// pass to the big proposeConfig
	std::vector<DigitalPinConfig> propConf { pconf };
	std::vector<DigitalPinConfig> initConf { iconf };
	std::vector<unsigned int> pins { lid };
	DigitalPinRejectedConfiguration::Reason err;
	proposeConfigImpl(
		pins,
		propConf,
		initConf,
		[&err](DigitalPinRejectedConfiguration::Reason e) { err = e; }
	);
	pconf = propConf[0];
	return err;
}

} } }
