/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ChipSelectErrors.hpp>
#include <duds/hardware/interface/DigitalPinMasterSyncSerial.hpp>
#include <duds/hardware/interface/MasterSyncSerialErrors.hpp>
#include <chrono>
#include <thread>

// nanos: the time of fate
static void nanodelay(int nanos) {
	std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
}

namespace duds { namespace hardware { namespace interface {

DigitalPinMasterSyncSerial::DigitalPinMasterSyncSerial() :
clk(-1), dat(-1), datI(-1) { }

DigitalPinMasterSyncSerial::DigitalPinMasterSyncSerial(Flags flags, int period) :
MasterSyncSerial(flags, period) { }

DigitalPinMasterSyncSerial::DigitalPinMasterSyncSerial(
	const DigitalPinSet &pset,
	Flags flags,
	int period
) : MasterSyncSerial(flags, period), pins(pset) { }

DigitalPinMasterSyncSerial::DigitalPinMasterSyncSerial(
	DigitalPinSet &&pset,
	Flags flags,
	int period
) : MasterSyncSerial(flags, period), pins(std::move(pset)) { }

DigitalPinMasterSyncSerial::~DigitalPinMasterSyncSerial() {
	forceClose();
}

void DigitalPinMasterSyncSerial::checkPins(const DigitalPinSet &ps, Flags cfg) {
	// there must be 2 pins for half-duplex, 3 for full duplex
	if (((cfg & MssFullDuplex) && (ps.size() != 3)) || (ps.size() != 2)) {
		DUDS_THROW_EXCEPTION(PinRangeError());
	}
	// get the capabilities for inspection
	std::vector<DigitalPinCap> caps = ps.capabilities();
	// clock
	if (!caps[ClockPin].canBeOutput()) {
		DUDS_THROW_EXCEPTION(PinUnsupportedOperation() << PinErrorName("clock")
			<< PinErrorId(ps.globalId(ClockPin))
		);
	}
	if (cfg & MssFullDuplex) {
		if (!caps[OutputPin].canBeOutput()) {
			DUDS_THROW_EXCEPTION(PinUnsupportedOperation() <<
				PinErrorName("output") << PinErrorId(ps.globalId(OutputPin))
			);
		}
		if (!caps[InputPin].canBeInput()) {
			DUDS_THROW_EXCEPTION(PinUnsupportedOperation() <<
				PinErrorName("input") << PinErrorId(ps.globalId(InputPin))
			);
		}
	} else { // half-duplex
		if (!caps[DataPin].canBeInput() || !caps[DataPin].canBeOutput()) {
			DUDS_THROW_EXCEPTION(PinUnsupportedOperation() <<
				PinErrorName("data") << PinErrorId(ps.globalId(DataPin))
			);
		}
	}
}

// ----- old below this line -----

void DigitalPinMasterSyncSerial::setPins(const PinIndex &pi) {
	if (flags & MssOpen) {
		DUDS_THROW_EXCEPTION(SyncSerialInUse());
	}
	static const std::string names[4] = {
		std::string("clock"),
		std::string("data"),
		std::string("output"),
		std::string("input"),
	};
	std::string find[3];
	find[0] = names[0];
	if (flags & MssFullDuplex) {
		find[1] = names[2];
		find[2] = names[3];
		pi.pinNumbers(find + 0, find + 3, pins, 3);
	} else {
		find[1] = names[1];
		pi.pinNumbers(find + 0, find + 2, pins, 2);
		pins[2] = -1;
	}
	std::shared_ptr<PinStore> nps = pi.store();
	try {
		// assure usable pins
		checkPins(nps, clk, dat, datI);
	} catch (...) {
		// remove pin IDs
		clk = dat = datI = -1;
		throw;
	}
	store = std::move(nps);
	// ready?
	if ((flags & MssUseSelect) && sel.usable()) {
		flags |= MssReady;
	}
}

void DigitalPinMasterSyncSerial::setPins(
	const std::shared_ptr<PinStore> &ps,
	unsigned int clock,
	unsigned int data
) {
	// this function is for half-duplex operation; fail on full-duplex
	if (flags & MssFullDuplex) {
		DUDS_THROW_EXCEPTION(SyncSerialNotFullDuplex());
	}
	if (flags & MssOpen) {
		DUDS_THROW_EXCEPTION(SyncSerialInUse());
	}
	checkPins(ps, clock, data, -1);
	clk = clock;
	dat = data;
	datI = -1;
	store = ps;
	// ready?
	if ((flags & MssUseSelect) && sel.usable()) {
		flags |= MssReady;
	}
}

void DigitalPinMasterSyncSerial::setPins(
	const std::shared_ptr<PinStore> &ps,
	unsigned int clock,
	unsigned int output,
	unsigned int input
) {
	// this function is for full-duplex operation; fail on half-duplex
	if (~flags & MssFullDuplex) {
		DUDS_THROW_EXCEPTION(SyncSerialNotHalfDuplex());
	}
	if (flags & MssOpen) {
		DUDS_THROW_EXCEPTION(SyncSerialInUse());
	}
	checkPins(ps, clock, output, input);
	clk = clock;
	dat = output;
	datI = input;
	store = ps;
	// ready?
	if ((flags & MssUseSelect) && sel.usable()) {
		flags |= MssReady;
	}
}

void DigitalPinMasterSyncSerial::setChipSelect(const ChipSelect &cs) {
	if (~flags & MssUseSelect) {
		DUDS_THROW_EXCEPTION(SyncSerialSelectNotUsed() <<
			ChipSelectIdError(cs.chipId()));
	}
	if (flags & MssOpen) {
		DUDS_THROW_EXCEPTION(SyncSerialInUse());
	}
	if (!cs.haveManager()) {
		DUDS_THROW_EXCEPTION(ChipSelectBadManager() <<
			ChipSelectIdError(cs.chipId()));
	}
	if (!cs.configured()) {
		DUDS_THROW_EXCEPTION(ChipSelectInvalidChip() <<
			ChipSelectIdError(cs.chipId()));
	}
	sel = cs;
	// ready?
	if ((clk != -1) && (dat != -1) &&
	((~flags & MssFullDuplex) || (datI != -1))) {
		flags |= MssReady;
	}
}

void DigitalPinMasterSyncSerial::clockFrequency(unsigned int freq) {
	MasterSyncSerial::clockFrequency(freq);
}

void DigitalPinMasterSyncSerial::clockPeriod(unsigned int nanos) {
	MasterSyncSerial::clockPeriod(nanos);
}

void DigitalPinMasterSyncSerial::open() {
	// obtain access to pins
	store->access(acc, pins, 3);
	// obtain select
	if (sel) {
		sel.access(chipAcc);
	}
	// set clock idle state
	acc[0].setOutput();
	acc[0].setState(flags & MssClockIdleHigh);
	// for full-duplex . . .
	if (flags & MssFullDuplex) {
		// . . . set I/O direction for data pins
		acc[1].setOutput();
		acc[2].setInput();
	}
}

void DigitalPinMasterSyncSerial::close() {
	for (int loop = 2; loop >= 0; loop--) {
		acc[loop].retire();
	}
	chipAcc.retire();
}

void DigitalPinMasterSyncSerial::start() {
	// using a select line?
	if (sel) {
		// select the gizmo-thingy
		chipAcc.select();
		nanodelay(minHalfPeriod);
	}
}

void DigitalPinMasterSyncSerial::stop() {
	// using a select line?
	if (sel) {
		// deselect the gizmo-thingy
		chipAcc.deselect();
	}
	// set clock idle state
	acc[0].setState(flags & MssClockIdleHigh);
	nanodelay(minHalfPeriod);
}

void DigitalPinMasterSyncSerial::transfer(
	const std::uint8_t * __restrict__ out,
	std::uint8_t * __restrict__ in,
	int bits
) {
	if (sel && (~flags & MssCommunicating)) {
		DUDS_THROW_EXCEPTION(SyncSerialNotCommunicating());
	}
	DigitalPinAccess &inpin = (flags & MssFullDuplex) ? acc[2] : acc[1];
	const std::uint8_t *outpos = out;
	int extrabits = 0;
	std::uint8_t *inpos = in;
	std::uint8_t bitpos;
	bool inbit;
	// half-duplex?
	if (~flags & MssFullDuplex) {
		// either full duplex communication is used, or one of the buffers is NULL
		if (in && out) {
			DUDS_THROW_EXCEPTION(SyncSerialNotFullDuplex());
		}
		// set data pin I/O direction
		if (out) {
			acc[1].setOutput();
		} else {
			acc[1].setInput();
		}
	}
	// MSb first?
	if (flags & MssMSbFirst) {
		bitpos = 0x80;
	} else {  // LSb first
		inpos += bits / 8;
		outpos += bits / 8;
		extrabits = bits % 8;
		if (extrabits == 0) {
			inpos--;
			outpos--;
			bitpos = 1;
		} else {
			bitpos = 0x80 >> (extrabits - 1);
		}
	}
	for (; bits; bits--) {
		// output next bit
		if (out) {
			acc[1].setState(*outpos & bitpos);
		}
		// transition the clock
		acc[0].setState(~flags & MssOutFallInRise);
		// provide time
		nanodelay(minHalfPeriod);
		// in case of input
		if (in) {
			// check input state
			if (inpin.getState()) {  // set bit
				*inpos |= bitpos;
			} else {                  // clear bit
				*inpos &= ~bitpos;
			}
		}
		// transition the clock
		acc[0].setState(flags & MssOutFallInRise);
		// provide time
		nanodelay(minHalfPeriod);
		// advance to next bit
		if (flags & MssMSbFirst) {
			if (!(bitpos >>= 1)) {
				bitpos = 0x80;
				inpos++;
				outpos++;
			}
		} else {
			if (!(bitpos <<= 1)) {
				bitpos = 1;
				inpos--;
				outpos--;
			}
		}
	}
	// in case of LSb first extra bits . . .
	if (in && extrabits) {
		// . . . shift down the MSB so the MSb's are not used and the LSb's are
		// all received bits
		in[0] >>= 8 - extrabits;
	}
}

} } } // namespaces

