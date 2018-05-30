/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CHIPSELECTMANAGER_HPP
#define CHIPSELECTMANAGER_HPP

#include <mutex>
#include <condition_variable>
#include <memory>
#include <boost/noncopyable.hpp>

namespace duds { namespace hardware { namespace interface {

class ChipAccess;

/**
 * The base class for all chip selection managers, the classes that handle the
 * output state to select a chip. The managers can only select one chip at a
 * time. They must be thread-safe. They must either be able to select no chip,
 * or designate a default chip. When destructed, they must select the default
 * chip or deselect all chips. A chip identifier must be either invalid or
 * have a 1-to-1 relation with the valid chips. Valid chip identifiers must
 * have a value of zero or greater; negative values are reserved to signal
 * manager termination and other conditions. Any changes to the set of
 * valid chips or to which chip an identifier references must not happen while
 * an access object is active; see ChipPinSelectManager for an implementation
 * example.
 *
 * @warning  Any required DigitalPinAccess objects for an operation should be
 *           acquired before any ChipAccess objects are needed for a single
 *           operation. This ordering is used by this library. Using a
 *           different ordering risks a deadlock.
 *
 * @author  Jeff Jackowski
 */
class ChipSelectManager : boost::noncopyable,
public std::enable_shared_from_this<ChipSelectManager> {
	/**
	 * The access object calls select(), deselect(), and retire(ChipAccess *).
	 */
	friend class ChipAccess;
protected:
	/**
	 * Used to synchonize access. Any changes that affect the set of valid chip
	 * IDs should occur when this semaphore is locked, or inside a derived
	 * class's constructor.
	 */
	std::mutex block;
private:
	/**
	 * Used to awaken threads waiting on a chip select.
	 */
	std::condition_variable selwait;
	/**
	 * The currently in use access object, or nullptr.
	 * I AM CURACC!
	 */
	ChipAccess *curacc;
	/**
	 * A count of the threads waiting to access chips.
	 * @note  This should only be modifid when @a block is locked.
	 */
	int waiting;
	/**
	 * Called by ChipAccess::~ChipAccess() to indicate that the access object is
	 * no longer in use, freeing the manager to offer access to other users.
	 * @throw ChipSelectInvalidAccess  The given access object, @a ca, is not
	 *                                 the active one for this manager,
	 *                                 @a curacc.
	 */
	void retire(ChipAccess *ca);
protected:
	/**
	 * Selected chip ID, or -1 to terminate.
	 * @note  This should only be modifid when @a block is locked.
	 */
	int cid;
	/**
	 * Selects the chip identified by @a cid. If the chip is already selected,
	 * it must remain selected.
	 * @note  There is no need for thread synchronization in this function.
	 */
	virtual void select() = 0;
	/**
	 * Deselects the chip identified by @a cid. If the chip is already
	 * deselected, it must remain deselected.
	 * @note  There is no need for thread synchronization in this function.
	 */
	virtual void deselect() = 0;
	/**
	 * Changes the chip in use while continuing to use an existing access
	 * object. If the chip is the same as the one already in use, nothing
	 * happens. If it is different, the validity of the new ID is checked,
	 * and if good, deselect() is called to deselect the current chip, then
	 * the new ID is recorded.
	 * @param chipId  The ID of the chip to use.
	 * @throw  ChipSelectInvalidChip   The given @a chipId is invalid. The
	 *                                 exception will include the ChipSelectId
	 *                                 attribute with the requested chip ID.
	 *                                 No changes will be made to the current
	 *                                 chip selection state.
	 * @throw ChipSelectInvalidAccess  There is currently no access object for
	 *                                 this manager.
	 */
	void changeChip(int chipId);
	/**
	 * Obtains the resources for providing an access object, but does not make
	 * an access object.
	 * @pre  The caller has a lock on @a block.
	 */
	void baseAccess(std::unique_lock<std::mutex> &lock, int chipId);
	/**
	 * Waits on a ChipAccess object if one is in use, then begins forcing
	 * any threads waiting on access to wake up and throw exceptions. This
	 * @b must be called in the destructor of non-abstract implementations of
	 * ChipSelectManager.
	 */
	void shutdown();
public:
	ChipSelectManager();
	/**
	 * Derived non-abstract classes @b must call shutdown() in thier destructor.
	 * This destructor only exists to avoid great badness and does nothing
	 * itself.
	 */
	virtual ~ChipSelectManager() = 0;
	/**
	 * Returns true if an access object provided by this manager exists.
	 */
	bool inUse() const {
		return curacc;
	}
	/**
	 * Returns true if @a chipId references a valid chip for this manager.
	 * All negative values must be considered invalid by all managers. Any
	 * non-negative value may be considered valid at the manager's discretion.
	 * @warning       This function must not lock @a block. Doing so will cause
	 *                access() to hang in a deadlock.
	 * @param chipId  The chip identifier that will be checked for validity.
	 *                The id must either be invalid or reference exactly one
	 *                chip.
	 * @return  True if @a chipId references a chip.
	 */
	virtual bool validChip(int chipId) const noexcept = 0;
	/**
	 * Acquires access to the requested chip and issues a ChipAccess object.
	 * The chip is @b not selected; use ChipAccess::select() to select the
	 * chip. If another chip is currently in use, this function will block
	 * until the associated ChipAccess object is destroyed.
	 * @warning       Attempting to select two chips from the same
	 *                ChipSelectManager on the same thread will cause a
	 *                deadlock.
	 * @param chipId  The number identifying the chip to select.
	 * @return        A ChipAccess object intended for use like a scoped lock
	 *                object; when it is destroyed, the chip will be deselected.
	 * @throw  ChipSelectInvalidChip  The given @a chipId is invalid. The
	 *                                exception will include the ChipSelectId
	 *                                attribute with the requested chip ID.
	 * @throw  ObjectDestructedError  The manager object was destructed before
	 *                                the access object could be obtained.
	 */
	std::unique_ptr<ChipAccess> access(int chipId);
	/**
	 * Acquires access to the requested chip and modifies an existing
	 * ChipAccess object to provide that access.
	 * The chip is @b not selected; use ChipAccess::select() to select the
	 * chip. If another chip is currently in use, this function will block
	 * until the associated ChipAccess object is destroyed.
	 * @pre           @a acc is not providing access to any ChipSelectManager.
	 * @warning       Attempting to select two chips from the same
	 *                ChipSelectManager on the same thread will cause a
	 *                deadlock.
	 * @param acc     The ChipAccess object that will be modified to provide
	 *                access to chip selection.
	 * @param chipId  The number identifying the chip to select.
	 * @throw  ChipSelectAccessInUse  The given ChipAccess object, @a acc, is
	 *                                already providing access to a
	 *                                ChipSelectManager.
	 * @throw  ChipSelectInvalidChip  The given @a chipId is invalid. The
	 *                                exception will include the ChipSelectId
	 *                                attribute with the requested chip ID.
	 * @throw  ObjectDestructedError  The manager object was destructed before
	 *                                the access object could be obtained.
	 */
	void access(ChipAccess &acc, int chipId);
	/**
	 * Selects the requested chip and issues a ChipAccess object. If another
	 * chip is currently in use, this function will block
	 * until the associated ChipAccess object is destroyed.
	 * @warning       Attempting to select two chips from the same
	 *                ChipSelectManager on the same thread will cause a
	 *                deadlock.
	 * @param chipId  The number identifying the chip to select.
	 * @return        A ChipAccess object intended for use like a scoped lock
	 *                object; when it is destroyed, the chip will be deselected.
	 * @throw  ChipSelectInvalidChip  The given @a chipId is invalid. The
	 *                                exception will include the ChipSelectId
	 *                                attribute with the requested chip ID.
	 * @throw  ObjectDestructedError  The manager object was destructed before
	 *                                the access object could be obtained.
	 */
	std::unique_ptr<ChipAccess> select(int chipId);
	/**
	 * Selects the requested chip and modifies a ChipAccess object to further
	 * control chip selection. If another
	 * chip is currently in use, this function will block
	 * until the associated ChipAccess object is destroyed.
	 * @pre           @a acc is not providing access to any ChipSelectManager.
	 * @warning       Attempting to select two chips from the same
	 *                ChipSelectManager on the same thread will cause a
	 *                deadlock.
	 * @param acc     The ChipAccess object that will be modified to provide
	 *                access to chip selection.
	 * @param chipId  The number identifying the chip to select.
	 * @throw  ChipSelectAccessInUse  The given ChipAccess object, @a acc, is
	 *                                already providing access to a
	 *                                ChipSelectManager.
	 * @throw  ChipSelectInvalidChip  The given @a chipId is invalid. The
	 *                                exception will include the ChipSelectId
	 *                                attribute with the requested chip ID.
	 * @throw  ObjectDestructedError  The manager object was destructed before
	 *                                the access object could be obtained.
	 */
	void select(ChipAccess &acc, int chipId);
};

} } }
#endif        //  #ifndef CHIPSELECTMANAGER_HPP

