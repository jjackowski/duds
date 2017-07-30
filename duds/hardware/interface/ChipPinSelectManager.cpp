/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>

namespace duds { namespace hardware { namespace interface {

bool ChipPinSelectManager::validChip(int chipId) const noexcept {
	return selpin && (chipId == 1);
}

void ChipPinSelectManager::select() {
	selpin->output(selstate);
}

void ChipPinSelectManager::deselect() {
	selpin->output(!selstate);
}

void ChipPinSelectManager::setSelectPin(
	std::unique_ptr<DigitalPinAccess> &dpa,
	SelectState selectState
) {
	if (!dpa || !dpa->havePin()) {
		BOOST_THROW_EXCEPTION(PinDoesNotExist());
	}
	// require exclusive access
	std::unique_lock<std::mutex> lock(block);
	// is a chip in use?
	if (inUse()) {
		assert(selpin);
		// fail; provide the pin and chip IDs for what is currently in use
		BOOST_THROW_EXCEPTION(ChipSelectInUse() <<
			PinErrorId(selpin->globalId()) << ChipSelectIdError(1)
		);
	}
	// get the capabilities for inspection
	DigitalPinCap cap = dpa->capabilities();
	// check for no output ability
	if (!cap.canOutput()) {
		BOOST_THROW_EXCEPTION(DigitalPinCannotInputError() <<
			PinErrorId(dpa->globalId())
		);
	}
	// assure a deselected state prior to requesting output
	dpa->output(!selstate);
	// work out the actual output config
	dpa->modifyConfig(DigitalPinConfig(cap.firstOutputDriveConfigFlags()));
	// store this access object; seems good if it got this far without
	// an exception
	selpin = std::move(dpa);
	selstate = selectState;
}

ChipPinSelectManager::ChipPinSelectManager(
	std::unique_ptr<DigitalPinAccess> &dpa,
	SelectState selectState
) {
	setSelectPin(dpa, selectState);
}

ChipPinSelectManager::~ChipPinSelectManager() {
	shutdown();
}

} } }

