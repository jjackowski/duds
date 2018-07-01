/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPortIndependentPins.hpp>
//#include <fstream>

// !@?!#?!#?
// It was bad enough to find an MS header had "#define interface struct".
// I was hoping such things wouldn't be here, but I was wrong.
#undef linux

namespace duds { namespace hardware { namespace interface {

class PinConfiguration;

namespace linux {

/**
 * Base class for all errors specific to using the Linux GPIO character
 * device. If the error is reported by the kernel, the attribute
 * boost::errinfo_errno will be included in the error.
 * @note  The set of all exception objects thrown by GpioDevPort are all
 *        derived from PinError. Not all errors are specific to the Linux
 *        device.
 */
struct GpioDevPortError : PinError { };

/**
 * An error was reported from a GPIO_GET_LINEHANDLE_IOCTL operation.
 */
struct GpioDevGetLinehandleError : GpioDevPortError { };

/**
 * An error was reported from a GPIOHANDLE_GET_LINE_VALUES_IOCTL operation.
 */
struct GpioDevGetLineValuesError : GpioDevPortError { };

/**
 * An error was reported from a GPIOHANDLE_SET_LINE_VALUES_IOCTL operation.
 */
struct GpioDevSetLineValuesError : GpioDevPortError { };

/**
 * A GPIO implementation using the Linux kernel's GPIO character devices.
 *
 * Limitations:
 * - Input change events (interrupt-like response) are not yet supported.
 * - Port resources are not allocated and kept for the lifespan of
 *   DigitalPinAccess and DigitalPinSetAccess objects. Changing pin
 *   configuration with the GPIO kernel device requires losing the resource
 *   and requesting it again in a non-atomic manner. Another process could
 *   hypothetically get the resource, which will result in an exception and a
 *   broken access object. I see no way to fully resolve this issue given the
 *   current kernel interface.
 * - Kernel interface lacks ability to query pin capabilities.
 *   - This driver assumes all pins have input and output capability.
 *   - Open drain and open source support could be determined by attempting
 *     to reconfigure a pin, but this would require changing the state of
 *     the port, which may be bad for some hardware.
 *   - Currently the open drain and open source modes are not supported by
 *     this driver.
 * - Cannot determine initial output state.
 *   - The driver assumes false (logic zero) is the initial output state. It
 *     does not alter the initial state.
 *   - Requesting state of pins cannot be done without first altering them.
 *     - A request for an output pin makes the pin an output and sets its
 *       output state. The kernel does not support maintaining a pin's output
 *       state.
 *   - Kernel interface provides no way to query a set output state of a pin
 *     configured as an input, and requires the software to provide a new
 *     initial output state when configuring a pin as an output.
 * - Kernel interface lacks support for controllable pull-up and pull-down
 *   resistors.
 * - Kernel interface offers no query or configuration of output current.
 *
 * While some hardware may have some or all of these limitations, other
 * hardware does not. Using the Linux kernel GPIO character device will
 * impose these limitations on the system.
 *
 * It is assumed that the process using this object for a given pin will be the
 * only process on the host using the pin. This should be a fairly safe
 * assumption since it should only be violated by bad behavior.
 *
 * @author  Jeff Jackowski
 */
class GpioDevPort : public DigitalPortIndependentPins {
	/**
	 * The reported name of the GPIO chip device.
	 */
	std::string name;
	/**
	 * The consumer name given to the kernel when requesting the use of
	 * GPIO lines.
	 */
	std::string consumer;
	/**
	 * The path of the device file; retained only for error reporting purposes.
	 */
	std::string devpath;
	/**
	 * File descriptor for GPIO chip device file.
	 */
	int chipFd;
	/**
	 * Initializes a PinEntry with data on a GPIO line.
	 * @param offset  The line offset for the pin on the GPIO chip device.
	 * @param pid     The assigned port specific pin ID.
	 * @throw DigitalPortLacksPinError  The attempt to obtain information on
	 *                                  the GPIO line ended with an error.
	 */
	void initPin(std::uint32_t offset, unsigned int pid);
protected:
	virtual void madeAccess(DigitalPinAccess &acc);
	virtual void madeAccess(DigitalPinSetAccess &acc);
	virtual void retiredAccess(const DigitalPinAccess &acc) noexcept;
	virtual void retiredAccess(const DigitalPinSetAccess &acc) noexcept;
public:
	/**
	 * Make a GpioDevPort object with all the pins available to the device.
	 * If the program will be using a known subset of the pins, the
	 * GpioDevPort(const std::vector<unsigned int> &, const std::string &, unsigned int)
	 * constructor should be used instead to prevent and find unintentional
	 * use of other pins.
	 * @param path      The path to the GPIO device file.
	 * @param firstid   The gloabl ID that will be assigned to the first pin
	 *                  (local ID zero) of this port.
	 * @param username  The name provided to the GPIO device for the consumer.
	 * @throw DigitalPortDoesNotExistError  The device file could not be opened,
	 *                                      or the request for information on
	 *                                      the GPIO device ended in error.
	 * @throw DigitalPortLacksPinError      A request for information on a pin
	 *                                      ended in error.
	 */
	GpioDevPort(
		const std::string &path = "/dev/gpiochip0",
		unsigned int firstid = 0,
		const std::string &username = "DUDS"
	);
	/**
	 * Make a GpioDevPort object with the given pins.
	 * @todo            The doc on @a ids is incorrect; copied from Sysfs.
	 *                  Fix that. Also, maybe change the behavior. Could just
	 *                  specify pins to use and ignore position of pin in the
	 *                  vector.
	 * @param ids       The pin numbers from the filesystem. The index of each
	 *                  inside @a ids will be the local pin ID used by this
	 *                  port. A value of -1 will create an unavailable pin and
	 *                  may be used multiple times. Other values must only be
	 *                  used once.
	 * @param path      The path to the GPIO device file.
	 * @param firstid   The gloabl ID that will be assigned to the first pin
	 *                  (local ID zero) of this port.
	 * @param username  The name provided to the GPIO device for the consumer.
	 * @throw DigitalPortDoesNotExistError  The device file could not be opened,
	 *                                      or the request for information on
	 *                                      the GPIO device ended in error.
	 * @throw DigitalPortLacksPinError      A request for information on a pin
	 *                                      ended in error.
	 */
	GpioDevPort(
		const std::vector<unsigned int> &ids,
		const std::string &path = "/dev/gpiochip0",
		unsigned int firstid = 0,
		const std::string &username = "DUDS"
	);
	/**
	 * Make a GpioDevPort object according to the given configuration, and
	 * attach to the configuration.
	 * @param pc            The object with the port configuration data.
	 * @param name          The name of the port in the configuration.
	 * @param defaultPath   The default path to the port's device file. This
	 *                      will be used if not specified in the configuration.
	 * @param forceDefault  If true, the value in @a defaultPath will be used
	 *                      even if the device file is specified in the
	 *                      configuration.
	 * @throw PortDoesNotExistError         There is no port called @a name in
	 *                                      the given configuration.
	 * @throw DigitalPortDoesNotExistError  The device file could not be opened,
	 *                                      or the request for information on
	 *                                      the GPIO device ended in error.
	 * @throw DigitalPortLacksPinError      A request for information on a pin
	 *                                      ended in error.
	 */
	static std::shared_ptr<GpioDevPort> makeConfiguredPort(
		PinConfiguration &pc,
		const std::string &name = "default",
		const std::string &defaultPath = "/dev/gpiochip0",
		bool forceDefault = false
	);
	virtual ~GpioDevPort();
protected:
	// virtual functions required by Digitalport
	virtual void configurePort(
		unsigned int localPinId,
		const DigitalPinConfig &cfg,
		DigitalPinAccessBase::PortData *pdata
	);
	virtual bool inputImpl(
		unsigned int gid,
		DigitalPinAccessBase::PortData *pdata
	);
	virtual std::vector<bool> inputImpl(
		const std::vector<unsigned int> &pvec,
		DigitalPinAccessBase::PortData *pdata
	);
	virtual void outputImpl(
		unsigned int lid,
		bool state,
		DigitalPinAccessBase::PortData *pdata
	);
	virtual void outputImpl(
		const std::vector<unsigned int> &pvec,
		const std::vector<bool> &state,
		DigitalPinAccessBase::PortData *pdata
	);
public:
	/**
	 * Simultaneous operations are supported; returns true.
	 */
	virtual bool simultaneousOperations() const;
};

} } } } // namespaces

