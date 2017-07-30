/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/MasterSyncSerialAccess.hpp>
#include <duds/hardware/interface/MasterSyncSerialErrors.hpp>
#include <duds/hardware/interface/Conversation.hpp>

namespace duds { namespace hardware { namespace interface {

constexpr MasterSyncSerial::Flags MasterSyncSerial::MssUseSelect;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssClockIdleHigh;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssOutFallInRise;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssMSbFirst;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssFullDuplex;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssConfigMask;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssReady;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssOpen;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssCommunicating;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssFirstDerivedClassFlag;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode0;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode1;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode2;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode3;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode0LSb;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode1LSb;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode2LSb;
constexpr MasterSyncSerial::Flags MasterSyncSerial::MssSpiMode3LSb;

MasterSyncSerial::MasterSyncSerial(Flags f, int p) : minHalfPeriod(p >> 1) {
	flags = f & MssConfigMask;
}

MasterSyncSerial::~MasterSyncSerial() { }

void MasterSyncSerial::forceClose() {
	if (flags & MssOpen) {
		condStop();
	}
	close();
}

void MasterSyncSerial::retire(MasterSyncSerialAccess *acc) {
	if (acc == mssacc) {
		// don't retire the access object more than once
		acc->mss.reset();
		// end communication
		condStop();
		close();
		flags.clear(MssOpen);
		// lose the access object; it should be destructing
		mssacc = nullptr;
	} else {
		// panic!  maybe limit this to debug builds
		BOOST_THROW_EXCEPTION(SyncSerialInvalidAccess());
	}
}

unsigned int MasterSyncSerial::clockFrequency() const noexcept {
	return minHalfPeriod ? 500000000 / minHalfPeriod : 0;
}

void MasterSyncSerial::clockFrequency(unsigned int freq) {
	clockPeriod(freq ? 500000000 / freq : 0);
}

void MasterSyncSerial::clockPeriod(unsigned int period) {
	// this object must not be communicating
	if (flags & MssCommunicating) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	minHalfPeriod = period;
}

std::unique_ptr<MasterSyncSerialAccess> MasterSyncSerial::access() {
	if (~flags & MssReady) {
		BOOST_THROW_EXCEPTION(SyncSerialNotReady());
	}
	if ((flags & MssOpen) || mssacc) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	open();
	flags |= MssOpen;
	return std::unique_ptr<MasterSyncSerialAccess>(mssacc =
		new MasterSyncSerialAccess(shared_from_this()));
}

void MasterSyncSerial::access(MasterSyncSerialAccess &acc) {
	if (~flags & MssReady) {
		BOOST_THROW_EXCEPTION(SyncSerialNotReady());
	}
	if ((flags & MssOpen) || mssacc) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	if (acc.mss) {
		BOOST_THROW_EXCEPTION(SyncSerialAccessInUse());
	}
	open();
	flags |= MssOpen;
	acc.mss = shared_from_this();
	mssacc = &acc;
}

std::unique_ptr<MasterSyncSerialAccess> MasterSyncSerial::accessStart() {
	std::unique_ptr<MasterSyncSerialAccess> acc = access();
	condStart();
	return acc;
}

void MasterSyncSerial::accessStart(MasterSyncSerialAccess &acc) {
	access(acc);
	condStart();
}

void MasterSyncSerial::condStart() {
	/*
	if (~flags & MssReady()) {
		BOOST_THROW_EXCEPTION(SyncSerialNotReady());
	}
	*/
	if (~flags & MssOpen) {
		BOOST_THROW_EXCEPTION(SyncSerialNotOpen());
	}
	if (~flags & MssCommunicating) {
		start();
		flags |= MssCommunicating;
	}
}

void MasterSyncSerial::condStop() {
	/*
	if (~flags & MssReady()) {
		BOOST_THROW_EXCEPTION(SyncSerialNotReady());
	}
	*/
	if (~flags & MssOpen) {
		BOOST_THROW_EXCEPTION(SyncSerialNotOpen());
	}
	if (flags & MssCommunicating) {
		stop();
		flags.clear(MssCommunicating);
	}
}

void MasterSyncSerial::transmit(const std::uint8_t *buff, int bits) {
	transfer(buff, nullptr, bits);
}

void MasterSyncSerial::receive(std::uint8_t *buff, int bits) {
	transfer(nullptr, buff, bits);
}

void MasterSyncSerial::converseAlreadyOpen(Conversation &conv) {
	// visit each conversation part
	Conversation::PartVector::iterator iter = conv.begin();
	for (; iter != conv.end(); ++iter) {
		ConversationPart &cp = *(*iter);
		// check for need to provide a break in chip selection, or similar detail
		if (cp.flags() & ConversationPart::MpfBreak) {
			// deselect
			condStop();
		}
		// select chip if needed
		condStart();
		// input part?
		if (cp.input()) {
			receive((std::uint8_t*)cp.start(), cp.length() << 3);
		} else {
			transmit((std::uint8_t*)cp.start(), cp.length() << 3);
		}
	}
	// all done
	condStop();
}

void MasterSyncSerial::converse(Conversation &conv) {
	if (~flags & MssReady) {
		BOOST_THROW_EXCEPTION(SyncSerialNotReady());
	}
	if ((flags & MssOpen) || mssacc) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	open();
	// mark as in use without memory allocations or shared pointers
	flags |= MssOpen;
	//mssacc = (MasterSyncSerialAccess*)(-1);   // just needs to not be zero
	// do the communication
	try {
		converseAlreadyOpen(conv);
	} catch (...) {
		// close stuff and rethrow
		forceClose();
		flags.clear(MssOpen);
		throw;
	}
	// always close
	close();
	flags.clear(MssOpen);
}

} } } // namespaces
