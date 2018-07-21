/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinAccess.hpp>
#include <duds/hardware/interface/DigitalPinSetAccess.hpp>

namespace duds { namespace hardware { namespace interface {

DigitalPort::DigitalPort(unsigned int numpins, int unsigned firstid) :
pins(numpins), idOffset(firstid), waiting(0) { }

DigitalPort::~DigitalPort() { }

void DigitalPort::shutdown() {
	std::unique_lock<std::mutex> lock(block);
	// find existing pins' global IDs
	std::vector<unsigned int> gids;
	gids.reserve(pins.size());
	PinVector::iterator iter = pins.begin();
	for (unsigned int gid = idOffset; iter != pins.end(); ++iter, ++gid) {
		// pin exists?
		if (*iter) {
			// add its ID
			gids.push_back(gid);
		}
	}
	// wait on all pins to become available
	waitForAvailability(lock, &(gids[0]), gids.size());
	// remove all pins
	pins.clear();
	// awaken any threads waiting on access
	if (waiting) {
		pinwait.notify_all();
		// wait on those threads
		do {
			// this could throw an exception, but there seems to be no good
			// response
			pinwait.wait(lock);
			assert(waiting >= 0);
		} while (waiting);
	}
}

bool DigitalPort::exists(unsigned int gid) const {
	std::lock_guard<std::mutex> lock(block);
	unsigned int lid = localId(gid);
	// check for non-existence
	if ((lid >= pins.size()) || !pins[lid]) {
		return false;
	}
	return true;
}

std::vector<unsigned int> DigitalPort::localIds(
	const std::vector<unsigned int> &globalIds
) const {
	std::vector<unsigned int> localIds;
	localIds.reserve(globalIds.size());
	std::vector<unsigned int>::const_iterator iter = globalIds.cbegin();
	for (; iter != globalIds.cend(); ++iter) {
		// gap?
		if (*iter == -1) {
			// preserve the gap (-1 ID)
			localIds.push_back(-1);
		} else {
			localIds.push_back(localId(*iter));
		}
	}
	return localIds;
}

std::vector<unsigned int> DigitalPort::globalIds(
	const std::vector<unsigned int> &localIds
) const {
	std::vector<unsigned int> globalIds;
	globalIds.reserve(localIds.size());
	std::vector<unsigned int>::const_iterator iter = localIds.cbegin();
	for (; iter != localIds.cend(); ++iter) {
		// gap?
		if (*iter == -1) {
			// preserve the gap (-1 ID)
			globalIds.push_back(-1);
		} else {
			globalIds.push_back(localId(*iter));
		}
	}
	return globalIds;
}
/*
void DigitalPort::addPins(unsigned int maxId) {

}

void DigitalPort::removePin(unsigned int localPinId) {

}
*/
bool DigitalPort::areAvailable(const unsigned int *reqpins, std::size_t len) {
	// check for no pins -- exit condition
	if (pins.empty()) {
		// other threads waiting on pins?
		if (waiting) {
			// awaken them so that they can exit (cascades)
			pinwait.notify_one();
		}
		// no pins!
		DUDS_THROW_EXCEPTION(duds::general::ObjectDestructedError());
	}
	// check for availability of requested pins
	for (; len; len--, reqpins++) {
		// skip -1's
		if (*reqpins != -1) {
			unsigned int lid = localId(*reqpins);
			// check for non-existent pin
			if ((lid >= pins.size()) || !pins[lid]) {
				DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
					DigitalPortAffected(this) << PinErrorId(*reqpins)
				);
			}
			// check for pin in use
			if (pins[lid].access) {
				// will have to wait on it
				return false;
			}
		}
	}
	return true;
}

void DigitalPort::waitForAvailability(
	std::unique_lock<std::mutex> &lock,
	const unsigned int * const reqpins,
	const std::size_t len
) {
	// check on availability
	while (!areAvailable(reqpins, len)) {
		// count this as a waiting thread
		waiting++;
		// wait for notification
		pinwait.wait(lock);
		// no longer waiting
		waiting--;
	}
}

void DigitalPort::madeAccess(DigitalPinAccess &acc) { }

void DigitalPort::madeAccess(DigitalPinSetAccess &acc) { }

void DigitalPort::retiredAccess(const DigitalPinAccess &acc) noexcept { }

void DigitalPort::retiredAccess(const DigitalPinSetAccess &acc) noexcept { }

void DigitalPort::access(
	const unsigned int *reqpins,
	const unsigned int len,
	std::unique_ptr<DigitalPinAccess> *acc
) {
	if (!len) {
		DUDS_THROW_EXCEPTION(PinEmptyAccessRequest() << DigitalPortAffected(this));
	}
	// avoid throwing ObjectDestructedError when called on an empty port
	if (pins.empty()) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this));
	}
	// need exclusive access
	std::unique_lock<std::mutex> lock(block);
	// wait for pins
	waitForAvailability(lock, reqpins, len);
	// get the access objects for each pin
	for (int i = len; i; --i, ++reqpins, ++acc) {
		// skip -1's
		if (*reqpins != -1) {
			unsigned int lid = localId(*reqpins);
			// check once more for availability in case a pin is specified
			// twice in pins
			if (pins[lid].access) {
				DUDS_THROW_EXCEPTION(PinInUse() << DigitalPortAffected(this) <<
					PinErrorId(*reqpins)
				);
			}
			// create the access object
			*acc = std::unique_ptr<DigitalPinAccess>(
				new DigitalPinAccess(this, *reqpins)
			);
			// record the access
			pins[lid].access = acc->get();
			try {
				// notify implementation of the new access object
				madeAccess(**acc);
			} catch (...) {
				// implementation rejected access, so revoke access
				for (; i <= len; ++i, --acc) {
					updateAccess(**acc, nullptr);
				}
				throw;
			}
		}
	}
}
/*
void DigitalPort::access(
	const unsigned int *reqpins,
	const unsigned int len,
	std::unique_ptr<DigitalPinAccess[]> &acc
) {
	// avoid throwing ObjectDestructedError when called on an empty port
	if (pins.empty()) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this));
	}
	// need exclusive access
	std::unique_lock<std::mutex> lock(block);
	// wait for pins
	waitForAvailability(lock, reqpins, len);
	// need to allocate objects?
	if (!acc) {
		// allocate without constructing; leave uninitialized
		acc.reset((DigitalPinAccess*)
			new char(sizeof(DigitalPinAccess[len])));
	}
	// get the access objects for each pin
	for (int i = 0; i < len; i++, reqpins++) {
		// skip -1's
		if (*reqpins != -1) {
			unsigned int lid = localId(*reqpins);
			// check once more for availability in case a pin is specified
			// twice in pins
			if (pins[lid].access) {
				DUDS_THROW_EXCEPTION(PinInUse() << DigitalPortAffected(this) <<
					PinErrorId(*reqpins)
				);
			}
			// create the access object
			new (&(acc[i])) DigitalPinAccess(pins[lid]);
			// provide it to the DigitalPin (privilaged access)
			pins[lid].access = &(acc[i]);
		} else {
			// make access object without a pin
			new (&(acc[i])) DigitalPinAccess();
		}
	}
}
*/
void DigitalPort::access(
	const unsigned int *reqpins,
	const unsigned int len,
	DigitalPinAccess *acc
) {
	if (!len) {
		DUDS_THROW_EXCEPTION(PinEmptyAccessRequest() << DigitalPortAffected(this));
	}
	// avoid throwing ObjectDestructedError when called on an empty port
	if (pins.empty()) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this));
	}
	// need exclusive access
	std::unique_lock<std::mutex> lock(block);
	// wait for pins
	waitForAvailability(lock, reqpins, len);
	// get the access objects for each pin
	for (int i = 0; i < len; ++i, ++reqpins) {
		// skip -1's
		if (*reqpins != -1) {
			unsigned int lid = localId(*reqpins);
			// check once more for availability in case a pin is specified
			// twice in pins
			if (pins[lid].access) {
				DUDS_THROW_EXCEPTION(PinInUse() << DigitalPortAffected(this) <<
					PinErrorId(*reqpins)
				);
			}
			// create the access object
			new (acc + i) DigitalPinAccess(this, *reqpins);
			// record the access
			pins[lid].access = &(acc[i]);
			try {
				// notify implementation of the new access
				madeAccess(*(acc + i));
			} catch (...) {
				// implementation rejected access, so revoke access
				for (; i >= 0; --i) {
					updateAccess(*(acc + i), nullptr);
				}
				throw;
			}
		} else {
			// make access object without a pin
			new (acc + i) DigitalPinAccess();
		}
	}
}

std::unique_ptr<DigitalPinAccess> DigitalPort::access(const unsigned int pin) {
	std::unique_ptr<DigitalPinAccess> result(new DigitalPinAccess());
	access(&pin, 1, &result);
	return result;
}

void DigitalPort::access(
	const unsigned int *reqpins,
	const unsigned int len,
	DigitalPinSetAccess &acc
) {
	if (!len) {
		DUDS_THROW_EXCEPTION(PinEmptyAccessRequest() << DigitalPortAffected(this));
	}
	// avoid throwing ObjectDestructedError when called on an empty port
	if (pins.empty()) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this));
	}
	// supplied access object must either not be used by a port, or used by
	// this port
	if (acc.port() && acc.port() != this) {
		DUDS_THROW_EXCEPTION(PinSetWrongPort() << DigitalPortAffected(this));
	}
	// the port pointer might be null
	acc.dp = this;
	// need exclusive access
	std::unique_lock<std::mutex> lock(block);
	// wait for pins
	waitForAvailability(lock, reqpins, len);
	// pre-allocate space for pin IDs
	acc.reserveAdditional(len);
	// get the access to each pin
	for (int i = 0; i < len; i++, reqpins++) {
		// skip -1's
		if (*reqpins != -1) {
			unsigned int lid = localId(*reqpins);
			// check once more for availability in case a pin is specified
			// twice in pins
			if (pins[lid].access) {
				DUDS_THROW_EXCEPTION(PinInUse() << DigitalPortAffected(this) <<
					PinErrorId(*reqpins)
				);
			}
			// provide access
			acc.pinvec.push_back(lid);
			// record the access
			pins[lid].access = &acc;
		} else {
			// spot must still be used
			acc.pinvec.push_back(-1);
		}
	}
	try {
		// notify implementation of the new access
		madeAccess(acc);
	} catch (...) {
		// implementation rejected access, so revoke access
		updateAccess(acc, nullptr);
		throw;
	}
}

std::unique_ptr<DigitalPinSetAccess> DigitalPort::access(
	const std::vector<unsigned int> &pins
) {
	std::unique_ptr<DigitalPinSetAccess> result(new DigitalPinSetAccess());
	access(&(pins[0]), pins.size(), *(result.get()));
	return result;
}

void DigitalPort::updateAccess(
	const DigitalPinAccess &oldAcc,
	DigitalPinAccess *newAcc
) {
	// need exclusive access
	std::lock_guard<std::mutex> lock(block);
	// sanity check; failures should be bug in library or memory corruption
	assert(
		// the access object has recorded use of this port object
		(oldAcc.port() == this) &&
		// within range of the vector
		(oldAcc.localId() < pins.size()) &&
		// pin marked as existing
		pins[oldAcc.localId()] &&
		// recorded access object is identical to one passed in
		(pins[oldAcc.localId()].access == &oldAcc)
	);
	// notifty implementation of a retirement
	if (!newAcc) {
		retiredAccess(oldAcc);
	}
	// transfer access
	pins[oldAcc.localId()].access = newAcc;
	// notify any threads waiting on access
	if (!newAcc && waiting) {
		pinwait.notify_all();
	}
}

void DigitalPort::updateAccess(
	const DigitalPinSetAccess &oldAcc,
	DigitalPinSetAccess *newAcc
) {
	// sanity check: the access object has recorded use of this port object
	assert(oldAcc.port() == this);
	// need exclusive access
	std::lock_guard<std::mutex> lock(block);
	// notifty implementation of a retirement
	if (!newAcc) {
		retiredAccess(oldAcc);
	}
	// iterate through the pins
	std::vector<unsigned int>::const_iterator iter = oldAcc.pinvec.begin();
	for (; iter != oldAcc.pinvec.end(); ++iter) {
		// skip -1's
		if (*iter != -1) {
			// sanity check; failures should be bug in library or memory corruption
			assert(
				// within range of the vector
				(*iter < pins.size()) &&
				// pin marked as existing
				pins[*iter] &&
				// recorded access object is identical to one passed in
				(pins[*iter].access == &oldAcc)
			);
			// transfer access
			pins[*iter].access = newAcc;
		}
	}
	// notify any threads waiting on access
	if (!newAcc && waiting) {
		pinwait.notify_all();
	}
}

DigitalPinConfig DigitalPort::configuration(unsigned int gid) const {
	unsigned int lid = localId(gid);
	// assure no changes to the vector of pins
	std::lock_guard<std::mutex> lock(block);
	// check for non-existant pin
	if ((lid >= pins.size()) || !pins[lid]) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << PinErrorId(gid) <<
			DigitalPortAffected(this)
		);
	}
	// return copy of the config
	return pins[lid].conf;
}
/*
DigitalPinConfig &DigitalPort::proposedPinConfig(
	std::vector<DigitalPinConfig> &proposed,
	unsigned int localPinId
) const {
	// find the proposed config object
	DigitalPinConfig &cfg = proposed[localPinId];
	// does it claim no change? this is construction default in configure()
	if (cfg.options == DigitalPinConfig::OperationNoChange) {
		// copy the current config to avoid querying it again
		cfg = pins[localPinId].conf;
	}
	// return the proposed config
	return cfg;
}
*/

DigitalPinCap DigitalPort::capabilities(unsigned int gid) const {
	unsigned int lid = localId(gid);
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	if ((lid >= pins.size()) || !pins[lid]) {
		// no pin
		DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
			DigitalPortAffected(this) << PinErrorId(gid)
		);
	}
	return pins[lid].cap;
}

std::vector<DigitalPinCap> DigitalPort::capabilities() const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	std::vector<DigitalPinCap> caps;
	caps.reserve(pins.size());
	PinVector::const_iterator pin = pins.cbegin();
	for (; pin != pins.cend(); ++pin) {
		caps.push_back(pin->cap);
	}
	return caps;
}

std::vector<DigitalPinCap> DigitalPort::capabilities(
	const std::vector<unsigned int> &pvec, bool global
) const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// allocate
	std::vector<DigitalPinCap> res;
	res.reserve(pins.size());
	std::vector<unsigned int>::const_iterator iter = pvec.begin();
	for (; iter != pvec.cend(); ++iter) {
		// handle -1's
		if (*iter == -1) {
			res.push_back(NonexistentDigitalPin);
		} else {
			unsigned int lid;
			if (global) {
				lid = localId(*iter);
			} else {
				lid = *iter;
			}
			// non-existence check
			if (lid >= pins.size()) {
				// no pin
				DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
					DigitalPortAffected(this) << PinErrorId(
						global ? *iter : globalId(*iter)
					)
				);
			}
			// push the capabilities
			res.push_back(pins[lid].cap);
		}
	}
	return res;
}

std::vector<DigitalPinConfig> DigitalPort::configurationImpl() const {
	std::vector<DigitalPinConfig> conf;
	conf.reserve(pins.size());
	PinVector::const_iterator pin = pins.cbegin();
	for (; pin != pins.cend(); ++pin) {
		conf.push_back(pin->conf);
	}
	return conf;
}

std::vector<DigitalPinConfig> DigitalPort::configuration() const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	return configurationImpl();
}

std::vector<DigitalPinConfig> DigitalPort::configuration(
	const std::vector<unsigned int> &pvec, bool global
) const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// allocate
	std::vector<DigitalPinConfig> res;
	res.reserve(pins.size());
	std::vector<unsigned int>::const_iterator iter = pvec.begin();
	for (; iter != pvec.cend(); ++iter) {
		// handle -1's
		if (*iter == -1) {
			res.push_back(DigitalPinConfig::OperationNoChange);
		} else {
			unsigned int lid;
			if (global) {
				lid = localId(*iter);
			} else {
				lid = *iter;
			}
			// non-existence check
			if (lid >= pins.size()) {
				// no pin
				DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
					DigitalPortAffected(this) << PinErrorId(
						global ? *iter : globalId(*iter)
					)
				);
			}
			// push the config
			res.push_back(pins[lid].conf);
		}
	}
	return res;
}

DigitalPinRejectedConfiguration::Reason DigitalPort::proposeConfig(
	unsigned int gid,
	DigitalPinConfig &pconf,
	DigitalPinConfig &iconf
) const {
	// assure no changes to the pins from other threads
	std::unique_lock<std::mutex> lock(block);
	return proposeConfigImpl(gid, pconf, iconf);
}

bool DigitalPort::proposeConfig(
	const std::vector<unsigned int> &pins,
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// got global IDs, but need locals
	std::vector<unsigned int> local = localIds(pins);
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	return proposeConfigImpl(local, propConf, initConf, insertReason);
}

bool DigitalPort::proposeConfigLocalIds(
	const std::vector<unsigned int> &pins,
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	return proposeConfigImpl(pins, propConf, initConf, insertReason);
}

bool DigitalPort::proposeFullConfig(
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
) const {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	return proposeFullConfigImpl(propConf, initConf, insertReason);
}

DigitalPinConfig DigitalPort::modifyConfig(
	unsigned int gid,
	const DigitalPinConfig &cfg,
	DigitalPinAccessBase::PortData *pdata
) {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// check range
	unsigned int lid = localId(gid);
	if ((lid >= pins.size()) || !pins[lid]) {
		// no pin
		DUDS_THROW_EXCEPTION(PinDoesNotExist() <<
			PinErrorId(gid) << DigitalPortAffected(this) <<
			// reason included because it matches the result of proposeConfig()
			DigitalPinRejectedConfiguration::ReasonInfo(
				DigitalPinRejectedConfiguration::Unsupported
			)
		);
	}
	const DigitalPinConfig &iconf = pins[lid].conf;
	DigitalPinConfig actcfg = DigitalPinConfig::combine(iconf, cfg);
	// If this should be speedy and unsafe, call configurePort here and
	// skip the rest, except for pins[lid].conf = cfg;
	// test compatability - may throw
	DigitalPinRejectedConfiguration::Reason err =
		pins[lid].cap.compatible(actcfg);
	if (err) {
		DUDS_THROW_EXCEPTION(DigitalPinConfigError() <<
			PinErrorId(gid) << DigitalPortAffected(this) <<
			DigitalPinRejectedConfiguration::ReasonInfo(err) <<
			DigitalPinCapInfo(pins[lid].cap) <<
			DigitalPinConfigInfo(actcfg)
		);
	}
	// check for an independent configuration
	if (independentConfig(gid, actcfg, iconf)) {
		// make changes
		configurePort(lid, actcfg, pdata);
		// record the new config
		pins[lid].conf = actcfg;
	} else {
		// prepare to reconfigure whole port
		std::vector<DigitalPinConfig> initConf = configurationImpl();
		std::vector<DigitalPinConfig> propConf(initConf);
		propConf[lid] = actcfg;
		// do it -- implementation used by both modifyConfig functions
		modifyFullConfig(propConf, initConf, pdata);
	}
	return actcfg;
}

void DigitalPort::modifyFullConfig(
	std::vector<DigitalPinConfig> &propConf,
	std::vector<DigitalPinConfig> &initConf,
	DigitalPinAccessBase::PortData *pdata
) {
	// prepare data for config check
	std::vector<DigitalPinRejectedConfiguration::Reason> errs;
	// generate complete config and validate the config; check for error
	if (!proposeFullConfigImpl(
		propConf,
		initConf,
		[&errs](DigitalPinRejectedConfiguration::Reason e) {
			errs.push_back(e);
		}
	)) {
		// badness!
		DUDS_THROW_EXCEPTION(DigitalPinConfigError() <<
			DigitalPinRejectedConfiguration::ReasonVectorInfo(errs) <<
			DigitalPortAffected(this)
		);
	}
	// apply config
	configurePort(propConf, pdata);
	// record new config
	PinVector::iterator pin = pins.begin();
	std::vector<DigitalPinConfig>::const_iterator conf = propConf.cbegin();
	for (; conf != propConf.cend(); ++conf, ++pin) {
		pin->conf = *conf;
	}
}

void DigitalPort::modifyConfig(
	std::vector<DigitalPinConfig> &cfgs,
	DigitalPinAccessBase::PortData *pdata
) {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// prepare data for config check
	std::vector<DigitalPinConfig> initConf = configurationImpl();
	// do it -- implementation used by both modifyConfig functions
	modifyFullConfig(cfgs, initConf, pdata);
}

void DigitalPort::modifyConfig(
	const std::vector<unsigned int> &pvec,
	std::vector<DigitalPinConfig> &cfgs,
	DigitalPinAccessBase::PortData *pdata
) {
	// inputs must match size and not be empty
	if (cfgs.empty() || (cfgs.size() != pvec.size())) {
		DUDS_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
			DigitalPortAffected(this)
		);
	}
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// produce a config for the whole port
	std::vector<DigitalPinConfig> initConf = configurationImpl();
	std::vector<DigitalPinConfig> propConf = initConf;
	std::vector<unsigned int>::const_iterator piter = pvec.begin();
	std::vector<DigitalPinConfig>::const_iterator citer = cfgs.begin();
	for (; piter != pvec.end(); ++citer, ++piter) {
		// range & existence check
		if ((*piter != -1) && (*piter < pins.size()) && pins[*piter]) {
			// produce finalized configuration
			propConf[*piter].combine(*citer);
		}
	}
	// do it -- implementation used by both modifyConfig functions
	modifyFullConfig(propConf, initConf, pdata);
}

bool DigitalPort::input(unsigned int gid, DigitalPinAccessBase::PortData *pdata) {
	unsigned int lid = localId(gid);
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// out-of-range & non-existence check
	/** @todo  Are these required? A -1 could get through an access object. */
	if ((lid >= pins.size()) || !pins[lid]) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this)
			<< PinErrorId(gid)
		);
	}
	// check for a non-input configuration
	if (!(pins[lid].conf & DigitalPinConfig::DirInput)) {
		DUDS_THROW_EXCEPTION(PinWrongDirection() <<
			DigitalPortAffected(this) << PinErrorId(gid)
		);
	}
	// passed error checks; do the output
	return inputImpl(gid, pdata);
}

std::vector<bool> DigitalPort::input(
	const std::vector<unsigned int> &pvec,
	DigitalPinAccessBase::PortData *pdata
) {
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// check all pins existence and input config
	std::vector<unsigned int>::const_iterator piter = pvec.cbegin();
	for (; piter != pvec.cend(); ++piter) {
		// out-of-range & non-existence check
		/** @todo  Are these required? A -1 could get through an access object. */
		if ((*piter >= pins.size()) || !pins[*piter]) {
			DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this)
				<< PinErrorId(globalId(*piter))
			);
		}
		// check for a non-input configuration
		if (!(pins[*piter].conf & DigitalPinConfig::DirInput)) {
			DUDS_THROW_EXCEPTION(PinWrongDirection() <<
				DigitalPortAffected(this) << PinErrorId(globalId(*piter)) <<
				DigitalPinConfigInfo(pins[*piter].conf)
			);
		}
	}
	// passed error checks; do the input
	return inputImpl(pvec, pdata);
}

std::vector<bool> DigitalPort::inputImpl(
	const std::vector<unsigned int> &pvec,
	DigitalPinAccessBase::PortData *pdata
) {
	// using this implementation only makes sense if simultaneous operations
	// are not supported
	assert(!simultaneousOperations());
	std::vector<bool> res;
	res.reserve(pvec.size());
	std::vector<unsigned int>::const_iterator iter = pvec.cbegin();
	for (auto pid : pvec) {
		res.push_back(inputImpl(globalId(pid), pdata));
	}
	return res;
}

void DigitalPort::output(
	unsigned int gid,
	bool state,
	DigitalPinAccessBase::PortData *pdata
) {
	unsigned int lid = localId(gid);
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// out-of-range & non-existence check
	/** @todo  Are these required? A -1 could get through an access object. */
	if ((lid >= pins.size()) || !pins[lid]) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this)
			<< PinErrorId(gid)
		);
	}
	// no output capability check
	if (!pins[lid].cap.canOutput()) {
		DUDS_THROW_EXCEPTION(DigitalPinCannotOutputError() <<
			DigitalPortAffected(this) << PinErrorId(gid) <<
			DigitalPinCapInfo(pins[lid].cap)
		);
	}
	// passed error checks; do the output
	outputImpl(lid, state, pdata);
}

void DigitalPort::output(
	const std::vector<unsigned int> &pvec,
	const std::vector<bool> &state,
	DigitalPinAccessBase::PortData *pdata
) {
	// the inputs must be the same size
	if (pvec.size() != state.size()) {
		DUDS_THROW_EXCEPTION(DigitalPinConfigRangeError() <<
			DigitalPortAffected(this)
		);
	}
	// assure no changes to the pins from other threads
	std::lock_guard<std::mutex> lock(block);
	// check all pins existence and output capability
	std::vector<unsigned int>::const_iterator piter = pvec.cbegin();
	for (; piter != pvec.cend(); ++piter) {
		// out-of-range & non-existence check
		/** @todo  Are these required? A -1 could get through an access object. */
		if ((*piter >= pins.size()) || !pins[*piter]) {
			DUDS_THROW_EXCEPTION(PinDoesNotExist() << DigitalPortAffected(this)
				<< PinErrorId(globalId(*piter))
			);
		}
		// no output capability check
		if (!pins[*piter].cap.canOutput()) {
			DUDS_THROW_EXCEPTION(DigitalPinCannotOutputError() <<
				DigitalPortAffected(this) << PinErrorId(globalId(*piter))
			);
		}
	}
	// passed error checks; do the output
	outputImpl(pvec, state, pdata);
}

void DigitalPort::outputImpl(
	const std::vector<unsigned int> &pvec,
	const std::vector<bool> &state,
	DigitalPinAccessBase::PortData *pdata
) {
	// using this implementation only makes sense if simultaneous operations
	// are not supported
	assert(!simultaneousOperations());
	// loop through the pins to handle each in turn
	std::vector<unsigned int>::const_iterator piter = pvec.cbegin();
	std::vector<bool>::const_iterator biter = state.cbegin();
	for (; piter != pvec.cend(); ++biter, ++piter) {
		outputImpl(*piter, *biter, pdata);
	}
}

} } }
