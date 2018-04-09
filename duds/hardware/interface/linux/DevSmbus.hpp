/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/Smbus.hpp>
#include <string>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

// defined in linux/i2c-dev.h
struct i2c_smbus_ioctl_data;

namespace duds { namespace hardware { namespace interface { namespace linux {

/**
 * Implementation of the Smbus interface using the Linux kernel's user-space
 * support. This requires that the kernel be built with support for the SMBus
 * or I2C master, and support for user-space I2C access. If the support is in
 * kernel modules, they must be loaded along with the @a i2c-dev module. Using
 * the kernel's i2c-gpio driver should be more efficient than implementing the
 * SMBus protocol with user-space GPIO support.
 *
 * All thrown exceptions will include an attribute of boost::errinfo_file_name
 * with the device file name, along with
 * @ref duds::hardware::interface::SmbusDeviceAddr "SmbusDeviceAddr".
 *
 * The name follows SysFsGpio in naming the kernel interface, and it avoids
 * using the same name as the base class which I figured might lessen confusion.
 *
 * @author  Jeff Jackowski
 */
class DevSmbus : public duds::hardware::interface::Smbus {
	/**
	 * Stores the device file name for later error reporting.
	 */
	std::string dev;
	/**
	 * The file descriptor for the open device.
	 */
	int fd;
	/**
	 * The device (slave) address; used for error reporting.
	 */
	int addr;
	/**
	 * Sends I/O requests to the kernel, then checks for an error and if found
	 * throws the appropriate exception.
	 */
	void io(i2c_smbus_ioctl_data &sdat);
public:
	/**
	 * Opens the device file for the bus.
	 * @param devname  The path to the device file, usually @a /dev/i2c-N
	 *                 where N is the number assigned to the bus.
	 * @param devaddr  The device, or slave, address used as the destination of
	 *                 communications.
	 * @param pec      True (default) to enable use of Packet Error Checking.
	 *                 If the device supports PEC, this should be used to help
	 *                 prevent bad data over the bus from causing trouble.
	 * @throw SmbusErrorUnsupported   Either PEC requested but not supported, or
	 *                                a 10-bit address was requested but is not
	 *                                supported by the kernel's driver.
	 * @throw SmbusError              The device file could not be opened or
	 *                                failed to accept the device address.
	 */
	DevSmbus(const std::string &devname, int devaddr, bool pec = true);
	/**
	 * Opens the device file for the bus and specifies that Packet Error
	 * Checking (PEC) will be used.
	 * @param devname  The path to the device file, usually @a /dev/i2c-N
	 *                 where N is the number assigned to the bus.
	 * @param devaddr  The device, or slave, address used as the destination of
	 *                 communications.
	 * @throw SmbusErrorUnsupported   Either PEC requested but not supported, or
	 *                                a 10-bit address was requested but is not
	 *                                supported by the kernel's driver.
	 * @throw SmbusError              The device file could not be opened or
	 *                                failed to accept the device address.
	 */
	DevSmbus(const std::string &devname, int devaddr, UsePec) :
		DevSmbus(devname, devaddr, true) { }
	/**
	 * Opens the device file for the bus and specifies that Packet Error
	 * Checking (PEC) will not be used.
	 * @param devname  The path to the device file, usually @a /dev/i2c-N
	 *                 where N is the number assigned to the bus.
	 * @param devaddr  The device, or slave, address used as the destination of
	 *                 communications.
	 * @throw SmbusErrorUnsupported   A 10-bit address was requested but is not
	 *                                supported by the kernel's driver.
	 * @throw SmbusError              The device file could not be opened or
	 *                                failed to accept the device address.
	 */
	DevSmbus(const std::string &devname, int devaddr, NoPec) :
		DevSmbus(devname, devaddr, false) { }
	/**
	 * Closes the device file.
	 */
	~DevSmbus();
	// documentation for the functions below is in
	//  duds/hardware/interface/Smbus.hpp
	virtual std::uint8_t receiveByte();
	virtual std::uint8_t receiveByte(std::uint8_t cmd);
	virtual std::uint16_t receiveWord(std::uint8_t cmd);
	virtual int receive(
		std::uint8_t cmd,
		std::uint8_t *in,
		const int maxlen
	);
	virtual void receive(std::uint8_t cmd, std::vector<std::uint8_t> &in);
	virtual void transmitBool(bool out);
	virtual void transmitByte(std::uint8_t byte);
	virtual void transmitByte(std::uint8_t cmd, std::uint8_t byte);
	virtual void transmitWord(std::uint8_t cmd, std::uint16_t word);
	virtual void transmit(
		std::uint8_t cmd,
		const std::uint8_t *out,
		const int len
	);
	virtual std::uint16_t call(std::uint8_t cmd, std::uint16_t word);
	virtual void call(
		std::uint8_t cmd,
		const std::vector<std::uint8_t> &out,
		std::vector<std::uint8_t> &in
	);
	virtual int address() const;
};

} } } }
