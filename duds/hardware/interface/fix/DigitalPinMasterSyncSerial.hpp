/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinSet.hpp>
#include <duds/hardware/interface/MasterSyncSerial.hpp>
#include <duds/hardware/interface/ChipSelect.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * A synchronous serial implementation using DigitalPin objects. The DigitalPin
 * objects provide an abstraction to the hardware. The pins must all be managed
 * by the same PinStore object. The PinStore provides thread-safe operation;
 * multiple DigitalPinMasterSyncSerial objects and other things may share pins.
 *
 * @author  Jeff Jackowski.
 */
class DigitalPinMasterSyncSerial : public MasterSyncSerial {
	/**
	 * Chip selection for serial interfaces that require it.
	 * @todo  Maybe use multiple classes so that the ChipSelect object exists
	 *        only for serial interfaces that require it.
	 */
	ChipSelect sel;
	/**
	 * Chip access for for serial interfaces that require chip selection.
	 * Placing the object here prevents the need to dynamically allocate it
	 * every time it is needed.
	 * @todo  Maybe use multiple classes so that the ChipSelect object exists
	 *        only for serial interfaces that require it.
	 */
	ChipAccess chipAcc;
	
	DigitalPinSet pins;
	// must be very careful to retire on all failures
	DigitalPinSetAccess acc;
	/**
	 * Pin positions of the items in the pin set.
	 */
	enum PinPos {
		/**
		 * The clock pin.
		 */
		ClockPin,
		/**
		 * The data pin for half-duplex. Used for full-duplex output.
		 */
		DataPin,
		/**
		 * The input pin for full-duplex.
		 */
		InputPin,
		/**
		 * The output pin for full-duplex.
		 */
		OutputPin = DataPin
	};
	/**
	 * The store of pins that manages the pins used by this serial interface.
	 */
	std::shared_ptr<PinStore> store;
	union {
		/**
		 * The pin IDs to use.
		 */
		unsigned int pins[3];
		struct {
			/**
			 * The pin ID for the clock.
			 */
			unsigned int clk;
			/**
			 * The pin ID for data, output if full-duplex.
			 */
			unsigned int dat;
			/**
			 * The pin ID for data input if full-duplex.
			 */
			unsigned int datI;
		};
	};
	/**
	 * Access objects for the pins. Putting them here avoids dynamically
	 * allocating them.
	 */
	DigitalPinAccess acc[3];
	/**
	 * Checks the pins' capabilities to assure they can be used in the intended
	 * role. This is not a member function because it must not update the data
	 * in a DigitalPinMasterSyncSerial object, and the data it might use should
	 * not be set until after this function succeeds, so there is no need to
	 * use member variables or functions. This assists with a proper error
	 * response.
	 * @param ps   The pin set with all the pins to use indexed as in PinPos.
	 * @param cfg  The configuration flags. Presently only needed to check
	 *             for the intent to use full or half duplex communication.
	 * @throw PinRangeError            Half-duplex needs 3 pins. Full-duplex
	 *                                 needs 4. If this is thrown, the number
	 *                                 of pins supplied in @a ps is wrong.
	 * @throw PinUnsupportedOperation  One of the pins lacks the needed input
	 *                                 and/or output capability. Which pin is
	 *                                 mentioned in the data attached to the
	 *                                 exception by usage name and ID.
	 */
	static void checkPins(const DigitalPinSet &ps, Flags cfg);
protected:
	/**
	 * Gets the required access objects.
	 * @note  This function may block.
	 */
	virtual void open();
	/**
	 * Relinquishes the access objects.
	 */
	virtual void close();
	/**
	 * Selects the device, which may be the same as doing nothing.
	 */
	virtual void start();
	/**
	 * Deselects the device and assures the clock is in the idle state.
	 */
	virtual void stop();
	/**
	 * Moves data about.
	 */
	virtual void transfer(
		const std::uint8_t * __restrict__ out,
		std::uint8_t * __restrict__ in,
		int bits
	);
public:
	DigitalPinMasterSyncSerial();
	/**
	 * @param flags    The flags specifying the low-level details of the serial
	 *                 protocol.
	 * @param period   The minimum clock period in nanoseconds.
	 */
	DigitalPinMasterSyncSerial(Flags flags, int period);
	DigitalPinMasterSyncSerial(const DigitalPinSet &pset, Flags flags, int period);
	DigitalPinMasterSyncSerial(DigitalPinSet &&pset, Flags flags, int period);
	~DigitalPinMasterSyncSerial();
	/**
	 * Sets the pins to use based on names already set in the PinIndex.
	 * The names are:
	 *  - clock
	 *  - data (half-duplex)
	 *  - input (full-duplex)
	 *  - output (full-duplex)
	 * @param pi  The index with the pin names.
	 * @throw PinDoesNotExist  One of the names is not present in @a pi.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void setPins(const PinIndex &pi);
	/**
	 * Sets the pins to use based on names already set in the PinIndex.
	 * The names are:
	 *  - clock
	 *  - data (half-duplex)
	 *  - input (full-duplex)
	 *  - output (full-duplex)
	 * @param pi  The index with the pin names.
	 * @throw PinDoesNotExist  One of the names is not present in @a pi.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void setPins(const std::shared_ptr<PinIndex> &pi) {
		setPins(*pi);
	}
	/**
	 * Sets the pins to use for half-duplex operation.
	 * @param store  The PinStore that manages the pins to use.
	 * @param clock  The ID for the clock pin.
	 * @param data   The ID for the data pin.
	 * @throw SyncSerialNotFullDuplex  This object is configured for
	 *                                 full-duplex operation.
	 * @throw PinDoesNotExist          One of the pins does not exist in
	 *                                 @a store.
	 * @throw PinUnsupportedOperation  The input and output capabilities of
	 *                                 one of the pins is not suitable for
	 *                                 its role, like a clock that cannot
	 *                                 output.
	 * @throw SyncSerialInUse          Called while communication is in progress.
	 */
	void setPins(
		const std::shared_ptr<PinStore> &store,
		unsigned int clock,
		unsigned int data
	);
	/**
	 * Sets the pins to use for full-duplex operation.
	 * @param store  The PinStore that manages the pins to use.
	 * @param clock   The ID for the clock pin.
	 * @param output  The ID for the output data pin.
	 * @param input   The ID for the intput data pin.
	 * @throw SyncSerialNotHalfDuplex  This object is configured for
	 *                                 half-duplex operation.
	 * @throw PinDoesNotExist          One of the pins does not exist in
	 *                                 @a store.
	 * @throw PinUnsupportedOperation  The input and output capabilities of
	 *                                 one of the pins is not suitable for
	 *                                 its role, like a clock that cannot
	 *                                 output.
	 * @throw SyncSerialInUse          Called while communication is in progress.
	 */
	void setPins(
		const std::shared_ptr<PinStore> &store,
		unsigned int clock,
		unsigned int output,
		unsigned int input
	);
	/**
	 * Sets the ChipSelect object to use for selections.
	 * @pre  The MssUseSelect flag is set.
	 * @param cs  The ChipSelect object to use. It must be configured and
	 *            have a manager. This means the following members of @a cs
	 *            will all return true:
	 *             - ChipSelect::isUsable()
	 *             - ChipSelect::isConfigured()
	 *             - ChipSelect::haveManager()
	 * @throw SyncSerialSelectNotUsed  This object is not configured to use
	 *                                 chip selection.
	 * @throw ChipSelectBadManager     @a cs does not have an associated
	 *                                 ChipSelectManager.
	 * @throw ChipSelectInvalidChip    @a cs does not have a chip ID to select.
	 * @throw SyncSerialInUse          Called while communication is in progress.
	 */
	void setChipSelect(const ChipSelect &cs);
	/**
	 * Changes the maximum clock frequency.
	 * @pre  No communication must be taking place; the object must not be in
	 *       the MssCommunicating state.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param freq  The requested maximum clock frequence in hertz.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void clockFrequency(unsigned int freq);
	/**
	 * Changes the minimum clock period.
	 * @pre  No communication must be taking place; the object must not be in
	 *       the MssCommunicating state.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param nanos  The requested minimum clock period in nanoseconds.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void clockPeriod(unsigned int nanos);
};

} } } // namespaces

