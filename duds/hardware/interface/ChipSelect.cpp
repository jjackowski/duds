/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipSelect.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>

namespace duds { namespace hardware { namespace interface {

ChipSelect::ChipSelect() noexcept : cid(-1) { }

ChipSelect::ChipSelect(ChipSelectManager *csm, int chipId) {
	if (csm && (chipId >= 0)) {
		std::shared_ptr<ChipSelectManager> m = csm->shared_from_this();
		if (!m->validChip(cid = chipId)) {
			BOOST_THROW_EXCEPTION(ChipSelectInvalidChip() <<
				ChipSelectIdError(chipId));
		}
		mgr = std::move(m);
	}
	else {
		cid = -1;
	}
}

ChipSelect::ChipSelect(
	const std::shared_ptr<ChipSelectManager> &csm,
	int chipId) :
cid(chipId) {
	if (csm && (chipId >= 0)) {
		if (!csm->validChip(cid = chipId)) {
			BOOST_THROW_EXCEPTION(ChipSelectInvalidChip() <<
				ChipSelectIdError(chipId));
		}
		mgr = csm;
	}
	else {
		cid = -1;
	}
}

std::unique_ptr<ChipAccess> ChipSelect::access() {
	if (!mgr) {
		BOOST_THROW_EXCEPTION(ChipSelectBadManager() <<
			ChipSelectIdError(cid));
	}
	return mgr->access(cid);
}

void ChipSelect::access(ChipAccess &acc) {
	if (!mgr) {
		BOOST_THROW_EXCEPTION(ChipSelectBadManager() <<
			ChipSelectIdError(cid));
	}
	mgr->access(acc, cid);
}

std::unique_ptr<ChipAccess> ChipSelect::select() {
	if (!mgr) {
		BOOST_THROW_EXCEPTION(ChipSelectBadManager() <<
			ChipSelectIdError(cid));
	}
	return mgr->select(cid);
}

void ChipSelect::select(ChipAccess &acc) {
	if (!mgr) {
		BOOST_THROW_EXCEPTION(ChipSelectBadManager() <<
			ChipSelectIdError(cid));
	}
	mgr->select(acc, cid);
}

} } }

