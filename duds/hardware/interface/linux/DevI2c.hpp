/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/I2c.hpp>
#include <string>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

struct i2c_rdwr_ioctl_data;

namespace duds { namespace hardware { namespace interface { namespace linux {

/**
 * Implementation of the I2c interface using the Linux kernel's user-space
 * support. This requires that the kernel be built with support for the I2C
 * master, and support for user-space I2C access. If the support is in
 * kernel modules, they must be loaded along with the i2c-dev module. Using
 * the kernel's i2c-gpio driver should be more efficient than implementing the
 * I2C protocol with user-space GPIO support.
 *
 * All thrown exceptions will include an attribute of boost::errinfo_file_name
 * with the device file name, along with
 * @ref duds::hardware::interface::I2cDeviceAddr "I2cDeviceAddr".
 *
 * @author  Jeff Jackowski
 */
class DevI2c : public duds::hardware::interface::I2c {
	/**
	 * Stores the device file name for later error reporting.
	 */
	std::string dev;
	/**
	 * The file descriptor for the open device.
	 */
	int fd;
	/**
	 * The device (slave) address.
	 */
	int addr;
	/**
	 * Calls ioctl to request the kernel do the I2C communication, the check
	 * for error conditions and throw corresponding exception on error.
	 */
	void io(i2c_rdwr_ioctl_data &idat);
public:
	/**
	 * Opens the device file for the bus.
	 * @param devname  The path to the device file, usually @a /dev/i2c-N
	 *                 where N is the number assigned to the bus.
	 * @param devaddr  The device, or slave, address used as the destination of
	 *                 communications.
	 * @throw I2cErrorUnsupported   A 10-bit address was requested but is not
	 *                              supported by the kernel's driver.
	 * @throw I2cError              The device file could not be opened.
	 */
	DevI2c(const std::string &devname, int devaddr);
	/**
	 * Closes the device file.
	 */
	~DevI2c();
	/**
	 * Conducts I2C communication with a device using the Linux i2c-dev driver.
	 *
	 * The @ref ConversationPart::MpfVarlen "MpfVarlen" flag of ConversationPart
	 * is honored. Input parts using this must have a buffer longer than 32
	 * bytes; 32 for data, and 1 for the length.
	 *
	 * The @ref ConversationPart::MpfBreak "MpfBreak" flag of ConversationPart
	 * objects is honored by separating the conversation into multiple ioctl()
	 * calls. The combination of the kernel's scheduling and other running
	 * software will determine if any other I2C communication from the same
	 * master will occur between the ioctl() calls here.
	 *
	 * @param conv  The conversation to have with the device on the other end.
	 *
	 * @throw I2cErrorConversationLength  The conversation has too many parts
	 *                                    for the implementation to handle.
	 * @throw I2cErrorPartLength   A variable length input part had a buffer
	 *                             that was not longer than 32 bytes.
	 * @throw I2cErrorBusy         The bus was in use for an inordinate length
	 *                             of time. This is not caused by scheduling
	 *                             on the same host computer. It can be caused
	 *                             by another I2C master on a mulit-master bus.
	 * @throw I2cErrorNoDevice     The device did not respond to its address.
	 * @throw I2cErrorUnsupported  An operation is unsupported by the master.
	 * @throw I2cErrorProtocol     Data from the device does not conform to
	 *                             the I2C protocol.
	 * @throw I2cErrorTimeout      The operation took too long resulting in a
	 *                             bus timeout.
	 * @throw I2cError             A general error that doesn't fit one of the
	 *                             other exceptions.
	 */
	virtual void converse(Conversation &conv);
};

} } } }
