/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/MasterSyncSerialErrors.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <duds/hardware/interface/linux/SpiMasterSyncSerial.hpp>
#include <cstring>

namespace duds { namespace hardware { namespace interface { namespace linux {

SpiMasterSyncSerial::SpiMasterSyncSerial() : spifd(-1) { }

SpiMasterSyncSerial::SpiMasterSyncSerial(
	const std::string &path,
	Flags flags,
	int freq
) : MasterSyncSerial(flags, 0) {
	open(path);
	setClockFrequency(freq);
}

SpiMasterSyncSerial::SpiMasterSyncSerial(Flags flags) :
MasterSyncSerial(flags, 0), spifd(-1) { }

SpiMasterSyncSerial::~SpiMasterSyncSerial() {
	forceClose();
	::close(spifd);
}

void SpiMasterSyncSerial::open(
	const std::string &path,
	Flags newflags,
	int freq
) {
	// do not open when in use
	if (flags & MssOpen) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	// close an open SPI device file
	if (spifd >= 0) {
		::close(spifd);
	}
	if (newflags != 0) {
		flags = newflags;
	}
	// open the SPI device file
	spifd = ::open(path.c_str(), O_RDWR);
	if (spifd < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError() <<
			boost::errinfo_file_name(path));
	}
	std::uint8_t byte = 0;
	// figure out SPI mode flags
	if (flags & MssClockIdleHigh) {
		byte = SPI_CPOL;
		if (flags & MssOutFallInRise) {
			byte |= SPI_CPHA;
		}
	} else if (~flags & MssOutFallInRise) {
		byte |= SPI_CPHA;
	}
	// set SPI mode flags
	if (ioctl(spifd, SPI_IOC_WR_MODE, &byte) < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError() <<
			boost::errinfo_file_name(path));
	}
	// set bit order
	if (flags & MssMSbFirst) {
		byte = 0;
	} else {
		byte = 1;
	}
	if (ioctl(spifd, SPI_IOC_WR_LSB_FIRST, &byte) < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError() <<
			boost::errinfo_file_name(path));
	}
	// only 8 bits per word supported
	byte = 8;
	if (ioctl(spifd, SPI_IOC_WR_BITS_PER_WORD, &byte) < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError() <<
			boost::errinfo_file_name(path));
	}
	// set clock
	setClockFrequency(freq);
	// clear out struct used to describe transfers
	std::memset(&xfer, 0, sizeof xfer);
	// ready for use
	flags |= MssReady;
}

void SpiMasterSyncSerial::setClockFrequency(unsigned int freq) {
	if (flags & MssCommunicating) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	if (ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &freq) < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError());
	}
	MasterSyncSerial::clockFrequency(freq);
}

void SpiMasterSyncSerial::setClockPeriod(unsigned int nanos) {
	if (flags & MssCommunicating) {
		BOOST_THROW_EXCEPTION(SyncSerialInUse());
	}
	// hold the current clock period in case of error
	unsigned int prev = minHalfPeriod;
	// change the clock period
	MasterSyncSerial::clockPeriod(nanos);
	// get the resultant frequency
	nanos = clockFrequency();
	// atempt to change the frequency
	if (ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &nanos) < 0) {
		// revert the stored clock period
		minHalfPeriod = prev;
		// issue error
		BOOST_THROW_EXCEPTION(SyncSerialIoError());
	}
}

void SpiMasterSyncSerial::open() { }

void SpiMasterSyncSerial::close() { }

void SpiMasterSyncSerial::start() { }

void SpiMasterSyncSerial::stop() { }

void SpiMasterSyncSerial::transfer(
	const std::uint8_t * __restrict__ out,
	std::uint8_t * __restrict__ in,
	int bits
) {
	if (~flags & MssCommunicating) {
		BOOST_THROW_EXCEPTION(SyncSerialNotCommunicating());
	}
	// only full bytes may be transfered; no partial bytes
	if (bits & 7) {
		BOOST_THROW_EXCEPTION(SyncSerialUnsupported());
	}
	xfer.tx_buf = (ptrdiff_t)out;
	xfer.rx_buf = (ptrdiff_t)in;
	xfer.len = bits >> 3;
	if (ioctl(spifd, SPI_IOC_MESSAGE(1), &xfer) < 0) {
		BOOST_THROW_EXCEPTION(SyncSerialIoError());
	}
}

} } } } // namespaces

