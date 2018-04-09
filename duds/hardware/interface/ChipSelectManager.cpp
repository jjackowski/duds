/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipAccess.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace interface {

ChipSelectManager::ChipSelectManager() : curacc(nullptr), cid(0), waiting(0) { }

ChipSelectManager::~ChipSelectManager() { }

void ChipSelectManager::shutdown() {
	// require exclusive access
	std::unique_lock<std::mutex> lock(block);
	// wait on current selection
	while (curacc) {
		selwait.wait(lock);
	}
	// set termination condition
	cid = -1;
	// clear waiting threads
	selwait.notify_all();
	// wait on threads
	while (waiting) {
		// this could throw an exception, but there seems to be no good
		// response
		selwait.wait(lock);
		assert(waiting >= 0);
	}
}

void ChipSelectManager::retire(ChipAccess *ca) {
	if (ca == curacc) {
		// deselect the chip
		deselect();
		// lose the access object; it should be destructing
		curacc = nullptr;
		// unblock one thread waiting on the chip, if any
		selwait.notify_one();
	} else {
		// panic!
		DUDS_THROW_EXCEPTION(ChipSelectInvalidAccess());
	}
}

void ChipSelectManager::changeChip(int chipId) {
	std::lock_guard<std::mutex> lock(block);
	if (!curacc) {
		// bad request
		DUDS_THROW_EXCEPTION(ChipSelectInvalidAccess());
	}
	// check for differnt ID
	if (chipId != cid) {
		// assure the new chip is valid
		if (!validChip(chipId)) {
			DUDS_THROW_EXCEPTION(ChipSelectInvalidChip() <<
				ChipSelectIdError(chipId));
		}
		// deselect the current chip
		deselect();
		// use the new chip
		cid = chipId;
	}
}

void ChipSelectManager::baseAccess(std::unique_lock<std::mutex> &lock,
int chipId) {
	if (!validChip(chipId)) {
		DUDS_THROW_EXCEPTION(ChipSelectInvalidChip() <<
			ChipSelectIdError(chipId));
	}
	waiting++;
	while (curacc) {
		selwait.wait(lock);
	}
	waiting--;
	// check termination condition
	if (cid < 0) {
		// no other threads waiting on access?
		if (!waiting) {
			// notify the destructing thread
			selwait.notify_all();
		}
		DUDS_THROW_EXCEPTION(duds::general::ObjectDestructedError());
	}
	// set the chip ID to access
	cid = chipId;
}

std::unique_ptr<ChipAccess> ChipSelectManager::access(int chipId) {
	std::unique_lock<std::mutex> lock(block);
	// obtain resources
	baseAccess(lock, chipId);
	// produce & return access object
	return std::unique_ptr<ChipAccess>(
		curacc = new ChipAccess(shared_from_this())
	);
}

void ChipSelectManager::access(ChipAccess &acc, int chipId) {
	if (acc.manager) {
		DUDS_THROW_EXCEPTION(ChipSelectAccessInUse());
	}
	std::unique_lock<std::mutex> lock(block);
	// obtain resources
	baseAccess(lock, chipId);
	// configure access object
	acc.manager = shared_from_this();
	curacc = &acc;
}

std::unique_ptr<ChipAccess> ChipSelectManager::select(int chipId) {
	std::unique_ptr<ChipAccess> ca = access(chipId);
	select();
	return ca;
}

void ChipSelectManager::select(ChipAccess &acc, int chipId) {
	access(acc, chipId);
	select();
}

} } }
