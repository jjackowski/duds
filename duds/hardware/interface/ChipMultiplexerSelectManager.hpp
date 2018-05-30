/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipSelectManager.hpp>
#include <duds/hardware/interface/ChipSelectErrors.hpp>
#include <duds/hardware/interface/DigitalPinSetAccess.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Selects one of several chips using several pins for parallel output of a
 * number. This implementation requires a multiplexer, but uses the fewest
 * number of pins for two chips or more.
 * @warning  If the DigitalPort implementation does not support simultaneous
 *           operations, then the output state will change one pin at a time
 *           in an unspecified order. This can cause brief selection states on
 *           chips other than the one intended for selection.
 * @author  Jeff Jackowski
 */
class ChipMultiplexerSelectManager : public ChipSelectManager {
	/**
	 * Access used for parallel output.
	 */
	std::unique_ptr<DigitalPinSetAccess> outacc;
protected:
	virtual void select();
	virtual void deselect();
public:
	/**
	 * Default constructor.
	 */
	ChipMultiplexerSelectManager() = default;
	/**
	 * Constructs the manager and sets the access object used to output the
	 * number of the chip to select.
	 * @param acc  The access object to use. It must provide access to at least
	 *             one pin.
	 * @throw PinDoesNotExist    Either the access object does not have
	 *                           any pins, or @a acc is empty. If thrown, no
	 *                           changes to this chip select manager will be
	 *                           made.
	 * @throw DigitalPinCannotInputError  One of the provided pins is incapable
	 *                                    of output.
	 */
	ChipMultiplexerSelectManager(std::unique_ptr<DigitalPinSetAccess> &&acc);
	/**
	 * Valid chip IDs are greater than zero and can be represented in the same
	 * number of bits as there are pins provided to the multiplexer.
	 */
	virtual bool validChip(int chipId) const noexcept;
	/**
	 * Sets the access object used to output the number of the chip to
	 * select.
	 * @pre        No chip is currently selected by this manager.
	 * @param acc  The access object to use. It must provide access to at least
	 *             one pin.
	 * @throw ChipSelectInUse    A ChipAccess object provided by this
	 *                           manager currently exists.
	 * @throw PinDoesNotExist    Either the access object does not have
	 *                           any pins, or @a acc is empty. If thrown, no
	 *                           changes to this chip select manager will be
	 *                           made.
	 * @throw DigitalPinCannotInputError  One of the provided pins is incapable
	 *                                    of output.
	 */
	void setAccess(std::unique_ptr<DigitalPinSetAccess> &&acc);
	/**
	 * Returns the access object that was used by this chip select manager.
	 * @pre       No chip is currently selected by this manager.
	 * @post      This manager cannot select chips until another access object
	 *            is provided.
	 * @throw ChipSelectInUse    A ChipAccess object provided by this
	 *                           manager currently exists.
	 */
	std::unique_ptr<DigitalPinSetAccess> releaseAccess();
};

} } }
