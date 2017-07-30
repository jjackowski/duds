/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CHIPSELECT_HPP
#define CHIPSELECT_HPP

#include <duds/hardware/interface/ChipAccess.hpp>

namespace duds { namespace hardware { namespace interface {

class ChipSelectManager;

/**
 * An object to wrap together a ChipSelectManager and chip ID to simplify
 * code that needs to repeatedly select the same chip.
 * @author  Jeff Jackowski
 */
class ChipSelect {
	/**
	 * The manager that will handle the selection.
	 */
	std::shared_ptr<ChipSelectManager> mgr;
	/**
	 * The chip to select.
	 */
	int cid;
public:
	/**
	 * Initializes the object to a non-configured state.
	 * @post  isConfigured() will return false.
	 */
	ChipSelect() noexcept;
	/**
	 * Makes a ChipSelect to select @a chipId from @a csm.
	 * @param csm     A pointer to the manager, or NULL to be non-configured.
	 * @param chipId  A non-negative chip ID, or negative to be non-configured.
	 * @throw ChipSelectInvalidChip  The manager reports that the given chip
	 *                               ID is invalid.
	 */
	ChipSelect(ChipSelectManager *csm, int chipId);
	/**
	 * Makes a ChipSelect to select @a chipId from @a csm.
	 * @param csm     A shared_ptr to the manager. It should be empty to be
	 *                non-configured.
	 * @param chipId  A non-negative chip ID, or negative to be non-configured.
	 * @throw ChipSelectInvalidChip  The manager reports that the given chip
	 *                               ID is invalid.
	 */
	ChipSelect(const std::shared_ptr<ChipSelectManager> &csm, int chipId);
	/**
	 * Obtains a ChipAccess object.
	 * @post    The chip is not selected, but the resource is acquired.
	 * @return  The access object for selecting the chip.
	 * @throw   ChipSelectBadManager  The ChipSelectManager, @a manager, is
	 *                                not set. This is normal when the default
	 *                                constructor is used.
	 */
	std::unique_ptr<ChipAccess> access();
	/**
	 * Modifies a ChipAccess object to use the manager and chip specified
	 * within this object.
	 * @pre     @a acc is not providing access for chip selection.
	 * @post    The chip is not selected, but the resource is acquired.
	 * @param acc   The access object that will allow chip selection.
	 * @throw   ChipSelectBadManager  The ChipSelectManager, @a manager, is
	 *                                not set. This is normal when the default
	 *                                constructor is used.
	 */
	void access(ChipAccess &acc);
	/**
	 * Obtains an access object and selects the chip.
	 * @post    The chip is selected.
	 * @return  The access object for selecting the chip.
	 * @throw   ChipSelectBadManager  The ChipSelectManager, @a manager, is
	 *                                not set. This is normal when the default
	 *                                constructor is used.
	 */
	std::unique_ptr<ChipAccess> select();
	/**
	 * Modifies a ChipAccess object to use the manager and chip specified
	 * within this object, and selects the chip.
	 * @pre     @a acc is not providing access for chip selection.
	 * @post    The chip is selected.
	 * @param acc   The access object that will allow chip selection.
	 * @throw   ChipSelectBadManager  The ChipSelectManager, @a manager, is
	 *                                not set. This is normal when the default
	 *                                constructor is used.
	 */
	void select(ChipAccess &acc);
	/**
	 * Returns true if this object has an associated manager.
	 */
	bool haveManager() const {
		return (bool)mgr;
	}
	/**
	 * Returns the associated manager object.
	 */
	const std::shared_ptr<ChipSelectManager> &manager() const {
		return mgr;
	}
	/**
	 * Returns true if this object was configured with a chip to select.
	 */
	bool configured() const {
		return cid >= 0;
	}
	/**
	 * Returns true if this object appears to be in a usable state.
	 */
	bool usable() const {
		return mgr && (cid >= 0);
	}
	/**
	 * This object evaluates to true if this object appears to be in a
	 * usable state.
	 */
	operator bool () const {
		return usable();
	}
	/**
	 * Returns the chip ID of the chip this object will select.
	 */
	int chipId() const {
		return cid;
	}
};

} } }


#endif        //  #ifndef CHIPSELECT_HPP
