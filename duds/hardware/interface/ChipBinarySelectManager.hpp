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
 * Selects one of two chips using a single pin. This means one of the chips
 * will always be selected. Not all chips will function properly using this
 * scheme. At least some EEPROM's have no problem with it. The hardware
 * implementation may require an inverter. Chip ID 0 is selected with a logic
 * low, and ID 1 with a logic high.
 *
 * Even though a chip is always selected, an access object should always be
 * requested prior to using either chip. This will ensure the correct chip is
 * selected by a multi-threaded program, and will work properly if the chip
 * selection implementation is changed.
 *
 * The pin used for chip selection must support output. This class allows
 * any output type, but will select the first output type supported in this
 * order:
 * -# @ref DigitalPinCap::OutputPushPull "Push-pull"
 * -# @ref DigitalPinCap::OutputDriveLow "Drive low"
 * -# @ref DigitalPinCap::OutputDriveHigh "Drive high"
 *
 * @todo Change select() and deselect() to ensure that the operations force a
 *       state change, at least momentarily, on the select line.
 *
 * @author  Jeff Jackowski
 */
class ChipBinarySelectManager : public ChipSelectManager {
	/**
	 * The access object for the select pin.
	 */
	std::unique_ptr<DigitalPinAccess> selpin;
protected:
	virtual void select();
	virtual void deselect();
public:
	/**
	 * Default constructor.
	 */
	ChipBinarySelectManager();
	/**
	 * Constructs a ChipBinarySelectManager with a pin for selection.
	 */
	ChipBinarySelectManager(
		std::unique_ptr<DigitalPinAccess> &dpa,
		int initSel = 0
	);
	/**
	 * Calls shutdown().
	 */
	~ChipBinarySelectManager();
	/**
	 * The only valid chip IDs for this manager are 0 and 1, and they are only
	 * valid once a DigitalPinAccess object has been provided.
	 */
	virtual bool validChip(int chipId) const noexcept;
	/**
	 * Sets the DigitalPinAccess object to use for the chip select line.
	 * @param dpa      The access object for the select pin.
	 * @param initSel  The initially selected chip ID. Must be 0 or 1.
	 * @pre   @a dpa is not empty; it has a valid DigitalPinAccess object.
	 * @post  @a dpa is used to set the pin state to output and to deselect the
	 *        chip. Any exceptions from this are @b not caught.
	 */
	void setSelectPin(std::unique_ptr<DigitalPinAccess> &dpa, int initSel = 0);
};

} } }

