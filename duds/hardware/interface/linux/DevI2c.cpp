/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/Conversation.hpp>
#include <duds/general/Errors.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <fcntl.h>      // for open and O_RDWR
#include <sys/ioctl.h>  // for ioctl
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

namespace duds { namespace hardware { namespace interface { namespace linux {

DevI2c::DevI2c(const std::string &devname, int devaddr) :
dev(devname), addr(devaddr) {
	fd = open(dev.c_str(), O_RDWR);
	if (fd < 0) {
		DUDS_THROW_EXCEPTION(I2cError() << boost::errinfo_errno(errno) <<
			boost::errinfo_file_name(dev) << I2cDeviceAddr(addr)
		);
	}
	if ((addr > 127) && (ioctl(fd, I2C_TENBIT, 1) < 0)) {
		close(fd);
		DUDS_THROW_EXCEPTION(I2cErrorUnsupported() <<
			boost::errinfo_file_name(dev) << I2cDeviceAddr(addr)
		);
	}
}

DevI2c::~DevI2c() {
	close(fd);
}

void DevI2c::io(i2c_rdwr_ioctl_data &idat) {
	if (ioctl(fd, I2C_RDWR, &idat) < 0) {
		int res = errno;
		switch (res) {
			case EBUSY:
				DUDS_THROW_EXCEPTION(I2cErrorBusy() <<
					boost::errinfo_file_name(dev) <<
					I2cDeviceAddr(addr)
				);
			case ENXIO:
			case ENODEV:
			case EREMOTEIO: // seems to be used for the same thing as above,
							// but not documented as such in Linux I2C docs
				DUDS_THROW_EXCEPTION(I2cErrorNoDevice() <<
					boost::errinfo_file_name(dev) <<
					boost::errinfo_errno(res) <<
					I2cDeviceAddr(addr)
				);
			case EOPNOTSUPP:
				DUDS_THROW_EXCEPTION(I2cErrorUnsupported() <<
					boost::errinfo_file_name(dev) <<
					I2cDeviceAddr(addr)
				);
			case EPROTO:
				DUDS_THROW_EXCEPTION(I2cErrorProtocol() <<
					boost::errinfo_file_name(dev) <<
					I2cDeviceAddr(addr)
				);
			case ETIMEDOUT:
				DUDS_THROW_EXCEPTION(I2cErrorTimeout() <<
					boost::errinfo_file_name(dev) <<
					I2cDeviceAddr(addr)
				);
			default:
				DUDS_THROW_EXCEPTION(I2cError() <<
					boost::errinfo_file_name(dev) <<
					boost::errinfo_errno(res) <<
					I2cDeviceAddr(addr)
				);
		}
	}
}

// The maximum number of supported I2C messages in a single ioctl call has a
// typo in some earlier kernels. A fix was put in by 4.4. Attempt to use
// the non-typo first so that when the typo is eventually removed this code
// doesn't break.
#ifdef I2C_RDWR_IOCTL_MAX_MSGS
#define I2C_MAX_MSGS  I2C_RDWR_IOCTL_MAX_MSGS
#elif defined(I2C_RDRW_IOCTL_MAX_MSGS)
#define I2C_MAX_MSGS  I2C_RDRW_IOCTL_MAX_MSGS
#else
#error Neither I2C_RDWR_IOCTL_MAX_MSGS nor I2C_RDRW_IOCTL_MAX_MSGS is defined.
#endif

void DevI2c::converse(Conversation &conv) {
	// empty conversation check
	if (conv.empty()) {
		return;  // nothing to do
	}
	// no larger than I2C_MAX_MSGS
	i2c_msg iparts[(conv.size() > I2C_MAX_MSGS) ? I2C_MAX_MSGS : conv.size()];
	i2c_rdwr_ioctl_data idat = {
		.msgs = iparts,
		.nmsgs = 0    // fill in later
	};
	// visit each conversation part
	Conversation::PartVector::iterator iter = conv.begin();
	for (int idx = 0; iter != conv.end(); ++iter, ++idx) {
		ConversationPart &cp = *(*iter);
		// check for a break in the conversation, but ignore first part
		if (idx && (cp.flags() & ConversationPart::MpfBreak)) {
			// do communication
			io(idat);
			// message count now zero
			idat.nmsgs = 0;
		} else if (idat.nmsgs == (I2C_MAX_MSGS - 1)) {
			DUDS_THROW_EXCEPTION(I2cErrorConversationLength() <<
				boost::errinfo_file_name(dev) << I2cDeviceAddr(addr) <<
				ConversationPartIndex(idx)
			);
		}
		__u16 flags;
		// input part?
		if (cp.input()) {
			flags = I2C_M_RD;
			if (cp.varyingLength()) {
				// Check for inadequate length.
				// Kernel header comments seem to imply that I2C_SMBUS_BLOCK_MAX
				// is not intended to be the correct value for this check
				// because I2C != SMBus, but kernel code seems to be using it
				// anyway.
				if (cp.length() <= 32) {
					DUDS_THROW_EXCEPTION(I2cErrorPartLength() <<
						boost::errinfo_file_name(dev) << I2cDeviceAddr(addr) <<
						ConversationPartIndex(idx)
					);
				}
				// possibility that the first byte of the buffer needs to be
				// set to 1 (number of bytes in addition to max recv len)
				flags |= I2C_M_RECV_LEN;
			} else {
				iparts[idat.nmsgs].flags = I2C_M_RD;
			}
		} else {
			flags = 0;
		}
		if (addr > 127) {
			flags |= I2C_TENBIT;
		}
		iparts[idat.nmsgs].addr = addr;
		iparts[idat.nmsgs].flags = flags;
		iparts[idat.nmsgs].len = cp.length();
		iparts[idat.nmsgs].buf = (__u8*)cp.start();
		// increment the message count
		++idat.nmsgs;
	}
	// should have one more ioctl call to complete the task
	io(idat);
}

int DevI2c::address() const {
	return addr;
}

} } } }
