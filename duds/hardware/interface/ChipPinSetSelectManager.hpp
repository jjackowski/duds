/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipSelectManager.hpp>
#include <duds/hardware/interface/DigitalPinSetAccess.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Selects a single chip at a time using one pin from a set. It uses a
 * DigitalPinSetAccess object to operate the select pins. By using a
 * DigitalPinSetAccess object, the pins will be flagged as in-use
 * by it's DigitalPort to prevent the pins from being used for some
 * other purpose.
 *
 * The pins used must support output. This class allows
 * any output type, but will select the first output type supported in this
 * order:
 * -# @ref DigitalPinCap::OutputPushPull "Push-pull"
 * -# @ref DigitalPinCap::OutputDriveLow "Drive low"
 * -# @ref DigitalPinCap::OutputDriveHigh "Drive high"
 *
 * @author  Jeff Jackowski
 */
class ChipPinSetSelectManager : public ChipSelectManager {
	/**
	 * The access object for the select pin.
	 */
	std::unique_ptr<DigitalPinSetAccess> selpins;
	/**
	 * The select logic states for each pin.
	 */
	std::uint32_t selstates;
protected:
	virtual void select();
	virtual void deselect();
public:
	/**
	 * Default constructor.
	 */
	ChipPinSetSelectManager() : selstates(0) { };
	/**
	 * Calls shutdown().
	 */
	~ChipPinSetSelectManager();
	/**
	 * Constructs a ChipPinSetSelectManager with a pin set to use for selection.
	 * @param dpsa             The access object for the select pins. Up to 32
	 *                         pins may be used.
	 * @param selectStates     The pin states that selects each device. The bit
	 *                         position corresponds to the index in @a dpsa.
	 *                         (1 << index)
	 * @throw ChipSelectInUse  A ChipAccess object provided by this manager
	 *                         currently exists.
	 * @throw PinDoesNotExist          The DigitalPinAccess object does not
	 *                                 provide access to any pin.
	 * @throw ChipSelectTooManyPins    More than 32 pins are in the access
	 *                                 object.
	 * @throw DigitalPinCannotOutputError  At least one pin cannot output.
	 */
	ChipPinSetSelectManager(
		std::unique_ptr<DigitalPinSetAccess> &&dpsa,
		std::uint32_t selectStates = 0
	);
	/**
	 * Valid IDs are those with a pin in the DigitalPinSetAccess object given
	 * to this manager.
	 */
	virtual bool validChip(int chipId) const noexcept;
	/**
	 * Sets the DigitalPinSetAccess object to use for the chip select lines, and
	 * the selection states for each chip.
	 * @param dpsa             The access object for the select pins. Up to 32
	 *                         pins may be used.
	 * @param selectStates     The pin states that selects each device. The bit
	 *                         position corresponds to the index in @a dpsa.
	 *                         (1 << index)
	 * @throw ChipSelectInUse  A ChipAccess object provided by this manager
	 *                         currently exists.
	 * @throw PinDoesNotExist          The DigitalPinAccess object does not
	 *                                 provide access to any pin.
	 * @throw ChipSelectTooManyPins    More than 32 pins are in the access
	 *                                 object.
	 * @throw DigitalPinCannotOutputError  At least one pin cannot output.
	 */
	void setSelectPins(
		std::unique_ptr<DigitalPinSetAccess> &&dpsa,
		std::uint32_t selectStates = 0
	);
};

} } }

