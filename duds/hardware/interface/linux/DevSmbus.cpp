/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/linux/DevSmbus.hpp>
#include <duds/hardware/interface/SmbusErrors.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <cstring>      // for memcpy
#include <algorithm>    // for max
#include <thread>       // for yield
#include <fcntl.h>      // for open and O_RDWR
#include <sys/ioctl.h>  // for ioctl
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

namespace duds { namespace hardware { namespace interface { namespace linux {

DevSmbus::DevSmbus(const std::string &devname, int devaddr, bool pec) :
dev(devname), addr(devaddr) {
	fd = open(dev.c_str(), O_RDWR);
	if (fd < 0) {
		DUDS_THROW_EXCEPTION(SmbusError() << boost::errinfo_errno(errno) <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
	if ((addr > 127) && (ioctl(fd, I2C_TENBIT, 1) < 0)) {
		close(fd);
		DUDS_THROW_EXCEPTION(SmbusErrorUnsupported() <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
	if (ioctl(fd, I2C_SLAVE, addr) < 0) {
		int err = errno;
		close(fd);
		DUDS_THROW_EXCEPTION(SmbusError() << boost::errinfo_errno(err) <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
	if (ioctl(fd, I2C_PEC, pec ? 1 : 0) < 0) {
		close(fd);
		DUDS_THROW_EXCEPTION(SmbusErrorUnsupported() <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
}

DevSmbus::~DevSmbus() {
	close(fd);
}

void DevSmbus::io(i2c_smbus_ioctl_data &sdat) {
	int res;
	do {
		if (ioctl(fd, I2C_SMBUS, &sdat) < 0) {
			res = errno;
			switch (res) {
				case EAGAIN:  // is this possible?
					// brief wait before next attempt
					std::this_thread::yield();
					break;
				case EBADMSG:
					DUDS_THROW_EXCEPTION(SmbusErrorPec() <<
						boost::errinfo_file_name(dev) <<
						SmbusDeviceAddr(addr)
					);
				case EBUSY:
					DUDS_THROW_EXCEPTION(SmbusErrorBusy() <<
						boost::errinfo_file_name(dev) <<
						SmbusDeviceAddr(addr)
					);
				case ENXIO:
				case ENODEV:
				case EREMOTEIO: // seems to be used for the same thing as above,
					            // but not documented as such in Linux I2C docs
					DUDS_THROW_EXCEPTION(SmbusErrorNoDevice() <<
						boost::errinfo_file_name(dev) <<
						boost::errinfo_errno(res) <<
						SmbusDeviceAddr(addr)
					);
				case EOPNOTSUPP:
					DUDS_THROW_EXCEPTION(SmbusErrorUnsupported() <<
						boost::errinfo_file_name(dev) <<
						SmbusDeviceAddr(addr)
					);
				case EPROTO:
					DUDS_THROW_EXCEPTION(SmbusErrorProtocol() <<
						boost::errinfo_file_name(dev) <<
						SmbusDeviceAddr(addr)
					);
				case ETIMEDOUT:
					DUDS_THROW_EXCEPTION(SmbusErrorTimeout() <<
						boost::errinfo_file_name(dev) <<
						SmbusDeviceAddr(addr)
					);
				default:
					DUDS_THROW_EXCEPTION(SmbusError() <<
						boost::errinfo_file_name(dev) <<
						boost::errinfo_errno(res) <<
						SmbusDeviceAddr(addr)
					);
			}
		}
	} while (res == EAGAIN);  // is this possible?
}

void DevSmbus::transmitBool(bool out) {
	i2c_smbus_ioctl_data sdat = {
		.read_write = (std::uint8_t)(out ? I2C_SMBUS_READ : I2C_SMBUS_WRITE),
		.command = 0,
		.size = I2C_SMBUS_QUICK,
		.data = NULL
	};
	io(sdat);
}

std::uint8_t DevSmbus::receiveByte() {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_READ,
		.command = 0,
		.size = I2C_SMBUS_BYTE,
		.data = &msg
	};
	io(sdat);
	return msg.byte;
}

void DevSmbus::transmitByte(std::uint8_t byte) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = 0,
		.size = I2C_SMBUS_BYTE,
		.data = &msg
	};
	msg.byte = byte;
	io(sdat);
}

std::uint8_t DevSmbus::receiveByte(std::uint8_t cmd) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_READ,
		.command = cmd,
		.size = I2C_SMBUS_BYTE_DATA,
		.data = &msg
	};
	io(sdat);
	return msg.byte;
}

void DevSmbus::transmitByte(std::uint8_t cmd, std::uint8_t byte) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = cmd,
		.size = I2C_SMBUS_BYTE_DATA,
		.data = &msg
	};
	msg.byte = byte;
	io(sdat);
}

std::uint16_t DevSmbus::receiveWord(std::uint8_t cmd) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_READ,
		.command = cmd,
		.size = I2C_SMBUS_WORD_DATA,
		.data = &msg
	};
	io(sdat);
	return msg.word;
}

void DevSmbus::transmitWord(std::uint8_t cmd, std::uint16_t word) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = cmd,
		.size = I2C_SMBUS_WORD_DATA,
		.data = &msg
	};
	msg.word = word;
	io(sdat);
}

int DevSmbus::receive(std::uint8_t cmd, std::uint8_t *in, const int maxlen) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_READ,
		.command = cmd,
		.size = I2C_SMBUS_BLOCK_DATA,
		.data = &msg
	};
	io(sdat);
	std::memcpy(in, msg.block + 1, std::max(maxlen, (int)msg.block[0]));
	if (maxlen < (int)msg.block[0]) {
		DUDS_THROW_EXCEPTION(SmbusErrorMessageLength() <<
			boost::errinfo_file_name(dev) <<
			SmbusDeviceAddr(addr)
		);
	}
	return msg.block[0];
}

void DevSmbus::receive(std::uint8_t cmd, std::vector<std::uint8_t> &in) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_READ,
		.command = cmd,
		.size = I2C_SMBUS_BLOCK_DATA,
		.data = &msg
	};
	io(sdat);
	in.resize(msg.block[0]);
	std::memcpy(&(in[0]), msg.block + 1, msg.block[0]);
}

void DevSmbus::transmit(
	std::uint8_t cmd,
	const std::uint8_t *out,
	const int len
) {
	if ((len <= 0) || (len > 32)) {
		DUDS_THROW_EXCEPTION(SmbusErrorMessageLength() <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = cmd,
		.size = I2C_SMBUS_BLOCK_DATA,
		.data = &msg
	};
	msg.block[0] = len;
	std::memcpy(msg.block + 1, out, len);
	io(sdat);
}

std::uint16_t DevSmbus::call(std::uint8_t cmd, std::uint16_t word) {
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = cmd,
		.size = I2C_SMBUS_BLOCK_DATA,
		.data = &msg
	};
	msg.word = word;
	io(sdat);
	return msg.word;
}

void DevSmbus::call(
	std::uint8_t cmd,
	const std::vector<std::uint8_t> &out,
	std::vector<std::uint8_t> &in
) {
	if (out.size() > 32) {
		DUDS_THROW_EXCEPTION(SmbusErrorMessageLength() <<
			boost::errinfo_file_name(dev) << SmbusDeviceAddr(addr)
		);
	}
	i2c_smbus_data msg;
	i2c_smbus_ioctl_data sdat = {
		.read_write = I2C_SMBUS_WRITE,
		.command = cmd,
		.size = I2C_SMBUS_BLOCK_DATA,
		.data = &msg
	};
	msg.block[0] = out.size();
	std::memcpy(msg.block + 1, &(out[0]), out.size());
	io(sdat);
	in.resize(msg.block[0]);
	std::memcpy(&(in[0]), msg.block + 1, msg.block[0]);
}

int DevSmbus::address() const {
	return addr;
}

} } } }
