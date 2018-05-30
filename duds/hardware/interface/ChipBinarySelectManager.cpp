/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipBinarySelectManager.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>

namespace duds { namespace hardware { namespace interface {

bool ChipBinarySelectManager::validChip(int chipId) const noexcept {
	return selpin && (chipId >= 0) && (chipId <= 1);
}

void ChipBinarySelectManager::select() {
	selpin->output(cid > 0);
}

void ChipBinarySelectManager::deselect() {
	selpin->output(cid == 0);
}

void ChipBinarySelectManager::setSelectPin(
	std::unique_ptr<DigitalPinAccess> &&dpa,
	int initSel
) {
	if (!dpa) {
		DUDS_THROW_EXCEPTION(PinDoesNotExist());
	}
	// require exclusive access
	std::unique_lock<std::mutex> lock(block);
	// is a chip in use?
	if (inUse()) {
		assert(selpin);
		// fail; provide the pin and chip IDs for what is currently in use
		DUDS_THROW_EXCEPTION(ChipSelectInUse() <<
			PinErrorId(selpin->globalId()) << ChipSelectIdError(1)
		);
	}
	// get the capabilities for inspection
	DigitalPinCap cap = dpa->capabilities();
	// check for no output ability
	if (!cap.canOutput()) {
		DUDS_THROW_EXCEPTION(DigitalPinCannotInputError() <<
			PinErrorId(dpa->globalId())
		);
	}
	// assure the requested state prior to begining output
	dpa->output(initSel > 0);
	// work out the actual output config
	if (cap & DigitalPinCap::OutputPushPull) {
		dpa->modifyConfig(DigitalPinConfig(
			DigitalPinConfig::DirOutput | DigitalPinConfig::OutputPushPull
		));
	} else if (cap & DigitalPinCap::OutputDriveLow) {
		dpa->modifyConfig(DigitalPinConfig(
			DigitalPinConfig::DirOutput | DigitalPinConfig::OutputDriveLow
		));
	} else {
		dpa->modifyConfig(DigitalPinConfig(
			DigitalPinConfig::DirOutput | DigitalPinConfig::OutputDriveHigh
		));
	}
	// store this access object; seems good if it got this far without
	// an exception
	selpin = std::move(dpa);
}

ChipBinarySelectManager::ChipBinarySelectManager() { }

ChipBinarySelectManager::ChipBinarySelectManager(
	std::unique_ptr<DigitalPinAccess> &&dpa, int initSel
) {
	setSelectPin(std::move(dpa), initSel);
}

ChipBinarySelectManager::~ChipBinarySelectManager() {
	shutdown();
}

} } }

