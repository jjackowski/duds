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
#include <duds/hardware/interface/DigitalPinAccess.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Selects a single chip using a single pin. This is the simplest chip select
 * implementation. It uses a DigitalPinAccess object to operate a single select
 * pin. By using a DigitalPinAccess object, the pin will be flagged as in-use
 * by it's DigitalPort to prevent the select pin from being used for some
 * other purpose.
 *
 * The pin used for chip selection must support output. This class allows
 * any output type, but will select the first output type supported in this
 * order:
 * -# @ref DigitalPinCap::OutputPushPull "Push-pull"
 * -# @ref DigitalPinCap::OutputDriveLow "Drive low"
 * -# @ref DigitalPinCap::OutputDriveHigh "Drive high"
 *
 * @author  Jeff Jackowski
 */
class ChipPinSelectManager : public ChipSelectManager {
	/**
	 * The access object for the select pin.
	 */
	std::unique_ptr<DigitalPinAccess> selpin;
public:
	/**
	 * The list of possible pin states that can be used to select a chip.
	 */
	enum SelectState {
		/**
		 * Indicates the chip is selected when the pin is driven low, or doesn't
		 * drive the line if the output type can only
		 * @ref DigitalPinCap::OutputDriveHigh "drive high".
		 */
		SelectLow,
		/**
		 * Indicates the chip is selected when the pin is driven high, or doesn't
		 * drive the line if the output type can only
		 * @ref DigitalPinCap::OutputDriveLow "drive low".
		 */
		SelectHigh
	};
protected:
	virtual void select();
	virtual void deselect();
	/**
	 * True when the chip is selected with a high logic level.
	 */
	SelectState  selstate;
public:
	/**
	 * Default constructor.
	 */
	ChipPinSelectManager() : selstate(SelectLow) { };
	/**
	 * Calls shutdown().
	 */
	~ChipPinSelectManager();
	/**
	 * Constructs a ChipPinSelectManager with a pin to use for selection.
	 * @param dpa          The access object for the select pin.
	 * @param selectState  The pin state that selects the pin.
	 * @pre   @a dpa is not empty; it has a valid DigitalPinAccess object.
	 * @post  @a dpa is empty. The object it contained is used to set the pin
	 *        state to output and to deselect the chip. Any exceptions from this
	 *        are @b not caught.
	 * @throw ChipSelectInUse  A ChipAccess object provided by this manager
	 *                         currently exists.
	 * @throw PinUnsupportedOperation  The given pin does not support output.
	 * @throw PinDoesNotExist          The DigitalPinAccess object does not
	 *                                 provide access to any pin.
	 */
	ChipPinSelectManager(
		std::unique_ptr<DigitalPinAccess> &dpa,
		SelectState selectState = SelectLow
	);
	/**
	 * The only valid chip ID for this manager is 1, and it is only valid once
	 * a DigitalPinAccess object has been provided.
	 */
	virtual bool validChip(int chipId) const noexcept;
	/**
	 * Sets the DigitalPinAccess object to use for the chip select line.
	 * @param dpa          The access object for the select pin.
	 * @param selectState  The pin state that selects the pin.
	 * @pre   @a dpa is not empty; it has a valid DigitalPinAccess object.
	 * @post  @a dpa is empty. The object it contained is used to set the pin
	 *        state to output and to deselect the chip. Any exceptions from this
	 *        are @b not caught.
	 * @throw ChipSelectInUse  A ChipAccess object provided by this manager
	 *                         currently exists.
	 * @throw PinUnsupportedOperation  The given pin does not support output.
	 * @throw PinDoesNotExist          The DigitalPinAccess object does not
	 *                                 provide access to any pin.
	 */
	void setSelectPin(
		std::unique_ptr<DigitalPinAccess> &dpa,
		SelectState selectState = SelectLow
	);
};

} } }

