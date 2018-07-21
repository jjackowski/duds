/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CHIPACCESS_HPP
#define CHIPACCESS_HPP

#include <duds/hardware/interface/ChipSelectManager.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * An object used to provide chip select control to a single user at a time.
 * When the object is destroyed, chip select is made available for another user.
 * @author  Jeff Jackowski
 */
class ChipAccess : boost::noncopyable {
	/**
	 * ChipSelectManager::access(int) calls the constructor.
	 */
	friend std::unique_ptr<ChipAccess> ChipSelectManager::access(int);
	/**
	 * ChipSelectManager::access(ChipAccess &, int) changes @a manager.
	 */
	friend void ChipSelectManager::access(ChipAccess &, int);
	/**
	 * The manager to which this object is attached.
	 */
	std::shared_ptr<ChipSelectManager> manager;
	/**
	 * Constructs a ChipAccess object for use with the given manager.
	 * @param m  The manager to access.
	 */
	ChipAccess(const std::shared_ptr<ChipSelectManager> &m) : manager(m) { }
	public:
	/**
	 * Makes a ChipAccess object that has no access.
	 */
	ChipAccess() { }
	/**
	 * Relinquishes access.
	 */
	~ChipAccess() {
		retire();
	}
	/**
	 * Relinquish access.
	 */
	void retire() {
		if (manager) {
			manager->retire(this);
			manager.reset();
		}
	}
	/**
	 * Selects the chip.
	 */
	void select() {
		manager->select();
	}
	/**
	 * Deselects the chip.
	 */
	void deselect() {
		manager->deselect();
	}
	/**
	 * Changes the chip in use while not giving up access to the chip selector.
	 * If the chip is the same as the one already in use, nothing happens.
	 * If it is different, the validity of the new ID is checked,
	 * and if good, deselect() is called to deselect the current chip, then
	 * the new ID is recorded.
	 * @post   The previous chip and new chip will be deselected.
	 * @param  chipId  The ID of the chip to use.
	 * @throw  ChipSelectInvalidChip   The given @a chipId is invalid. The
	 *                                 exception will include the ChipSelectId
	 *                                 attribute with the requested chip ID.
	 *                                 No changes will be made to the current
	 *                                 chip selection state; ownership of
	 *                                 access is not lost.
	 * @throw ChipSelectInvalidAccess  This is an invalid access object.
	 */
	void changeChip(int chipId) {
		manager->changeChip(chipId);
	}
};

} } }


#endif        //  #ifndef CHIPACCESS_HPP
