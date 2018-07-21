/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipPinSetSelectManager.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>

namespace duds { namespace hardware { namespace interface {

bool ChipPinSetSelectManager::validChip(int chipId) const noexcept {
	return selpins && selpins->exists((unsigned int)chipId);
}

void ChipPinSetSelectManager::select() {
	selpins->output(cid, (selstates & (1 << cid)) > 0);
}

void ChipPinSetSelectManager::deselect() {
	selpins->output(cid, (selstates & (1 << cid)) == 0);
}

void ChipPinSetSelectManager::setSelectPins(
	std::unique_ptr<DigitalPinSetAccess> &&dpsa,
	std::uint32_t selectStates
) {
	if (!dpsa || !dpsa->havePins()) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist());
	}
	if (dpsa->size() > 32) {
		DUDS_THROW_EXCEPTION(ChipSelectTooManyPins());
	}
	// require exclusive access
	std::unique_lock<std::mutex> lock(block);
	// is a chip in use?
	if (inUse()) {
		assert(selpins);
		// fail; provide the pin and chip IDs for what is currently in use
		DUDS_THROW_EXCEPTION(ChipSelectInUse() <<
			PinErrorId(selpins->globalId(cid)) << ChipSelectIdError(cid)
		);
	}
	// get the capabilities for inspection
	std::vector<DigitalPinCap> caps = dpsa->capabilities();
	// prepare to configure each pin
	std::vector<DigitalPinConfig> conf;
	conf.reserve(caps.size());
	// iteratate over all the pins
	std::vector<DigitalPinCap>::const_iterator iter = caps.cbegin();
	unsigned int pos = 0;
	for (; iter != caps.cend(); ++pos, ++iter) {
		// check for no output ability
		if (!iter->canOutput()) {
			DUDS_THROW_EXCEPTION(DigitalPinCannotInputError() <<
				PinErrorId(dpsa->globalId(pos))
			);
		}
		// work out the actual output config
		conf.push_back(DigitalPinConfig(iter->firstOutputDriveConfigFlags()));
	}

	// assure a deselected state prior to requesting output
	dpsa->write(~selectStates & ((1 << dpsa->size()) - 1));
	// work out the actual output config
	dpsa->modifyConfig(conf);
	// store this access object; seems good if it got this far without
	// an exception
	selpins = std::move(dpsa);
	selstates = selectStates;
}

ChipPinSetSelectManager::ChipPinSetSelectManager(
	std::unique_ptr<DigitalPinSetAccess> &&dpsa,
	std::uint32_t selectStates
) {
	setSelectPins(std::move(dpsa), selectStates);
}

ChipPinSetSelectManager::~ChipPinSetSelectManager() {
	shutdown();
}

} } }

