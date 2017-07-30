/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipMultiplexerSelectManager.hpp>

namespace duds { namespace hardware { namespace interface {

bool ChipMultiplexerSelectManager::validChip(int chipId) const noexcept {
	if (outacc) {
		return (chipId > 0) && (chipId < (1 << outacc->size()));
	}
	return false;
}

void ChipMultiplexerSelectManager::setAccess(
	std::unique_ptr<DigitalPinSetAccess> &acc
) {
	if (!acc || !acc->havePins()) {
		BOOST_THROW_EXCEPTION(PinDoesNotExist());
	}
	// require exclusive access
	std::unique_lock<std::mutex> lock(block);
	// is a chip in use?
	if (inUse()) {
		assert(outacc);
		// fail; provide the pin and chip IDs for what is currently in use
		BOOST_THROW_EXCEPTION(ChipSelectInUse() <<
			ChipSelectIdError(cid)
		);
	}
	// get the capabilities for inspection
	std::vector<DigitalPinCap> caps = acc->capabilities();
	// prepare to configure each pin
	std::vector<DigitalPinConfig> conf;
	conf.reserve(caps.size());
	// iteratate over all the pins
	std::vector<DigitalPinCap>::const_iterator iter = caps.cbegin();
	unsigned int pos = 0;
	for (; iter != caps.cend(); ++pos, ++iter) {
		// check for no output ability
		if (!iter->canOutput()) {
			BOOST_THROW_EXCEPTION(DigitalPinCannotInputError() <<
				PinErrorId(acc->globalId(pos))
			);
		}
		// work out the actual output config
		conf.push_back(DigitalPinConfig(iter->firstOutputDriveConfigFlags()));
	}
	// assure a deselected state prior to requesting output
	acc->output(false);
	// make all pins outputs
	acc->modifyConfig(conf);
	// store this access object; seems good if it got this far without
	// an exception
	outacc = std::move(acc);
}

std::unique_ptr<DigitalPinSetAccess>
ChipMultiplexerSelectManager::releaseAccess() {
	// get exclusive access to manager data
	std::unique_lock<std::mutex> lock(block);
	// is a chip in use?
	if (inUse()) {
		// fail; provide the chip ID for what is currently in use
		BOOST_THROW_EXCEPTION(ChipSelectInUse() << ChipSelectIdError(cid));
	}
	return std::move(outacc);
}

void ChipMultiplexerSelectManager::select() {
	outacc->write(cid);
}

void ChipMultiplexerSelectManager::deselect() {
	outacc->write((std::int32_t)0);
}

} } }

