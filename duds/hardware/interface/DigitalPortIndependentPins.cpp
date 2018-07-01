/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPortIndependentPins.hpp>

namespace duds { namespace hardware { namespace interface {

bool DigitalPortIndependentPins::independentConfig() const {
	return true;
}

bool DigitalPortIndependentPins::independentConfig(
	unsigned int,
	const DigitalPinConfig &,
	const DigitalPinConfig &
) const {
	return true;
}

// may throw from DigitalPinCap::compatible()
bool DigitalPortIndependentPins::proposeConfigImpl(
	const std::vector<unsigned int> &pvec,
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// inputs must match size, except initConf may be empty
	if ((propConf.size() != pvec.size()) || (
		!initConf.empty() && initConf.size() != pvec.size()
	)) {
		DUDS_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
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
	// iterate over the pins & config
	std::vector<unsigned int>::const_iterator piter = pvec.cbegin();
	std::vector<DigitalPinConfig>::iterator pConfIter = propConf.begin();
	std::vector<DigitalPinConfig>::iterator iConfIter = initConf.begin();
	bool good = true;
	for (; piter != pvec.cend(); ++piter, ++pConfIter, ++iConfIter) {
		DigitalPinRejectedConfiguration::Reason err =
			DigitalPinRejectedConfiguration::NotRejected;
		// skip -1's
		if (*piter != -1) {
			// range & existence check
			if ((*piter >= pins.size()) || (!pins[*piter] && (
				// non-existent & unsuable pins cannot be changed
				(*pConfIter != DigitalPinConfig::OperationNoChange) ||
				(*iConfIter != DigitalPinConfig::OperationNoChange)
			))) {
				// unusable
				err = DigitalPinRejectedConfiguration::Unsupported;
			} else if (pins[*piter]) {
				// initial config unset?
				if (*iConfIter == DigitalPinConfig::OperationNoChange) {
					// set to current config
					*iConfIter = pins[*piter].conf;
				}
				// combine options
				pConfIter->reverseCombine(*iConfIter);
				// test compatability - may throw
				err = pins[*piter].cap.compatible(*pConfIter);
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
	return good;
}

// not much of an improvement, performance wise, over the above function
bool DigitalPortIndependentPins::proposeFullConfigImpl(
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// inputs must match size of pins, except initConf may be empty
	if ((propConf.size() != pins.size()) || (
		!initConf.empty() && initConf.size() != pins.size()
	)) {
		DUDS_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
			DigitalPortAffected(this)
		);
	}
	// put in inital values for the starting config if empty
	if (initConf.empty()) {
		initConf = configurationImpl();
	}
	// iterate over the pins & config
	std::vector<DigitalPinConfig>::iterator pConfIter = propConf.begin();
	std::vector<DigitalPinConfig>::iterator iConfIter = initConf.begin();
	PinVector::const_iterator pvIter = pins.begin();
	unsigned int pos = 0;
	bool good = true;
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
			// combine options
			pConfIter->reverseCombine(*iConfIter);
			// test compatability - may throw
			err = pins[pos].cap.compatible(*pConfIter);
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
DigitalPortIndependentPins::proposeConfigImpl(
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
	// test compatability - may throw
	return pins[lid].cap.compatible(pconf);
}

void DigitalPortIndependentPins::configurePort(
	const std::vector<DigitalPinConfig> &cfgs,
	DigitalPinAccessBase::PortData *pdata
) {
	std::vector<DigitalPinConfig>::const_iterator iter = cfgs.cbegin();
	for (unsigned int lid = 0; iter != cfgs.end(); ++lid, ++iter)  {
		configurePort(lid, *iter, pdata);
	}
}

} } }
