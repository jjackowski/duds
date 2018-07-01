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

namespace duds { namespace hardware { namespace interface {

class PinConfiguration;

/**
 * Namespace for testing tools that implement hardware interfaces but function
 * without the hardware.
 */
namespace test {

/**
 * Base class for all errors specific to using the VirtualPort class.
 * @note  The set of all exception objects thrown by VirtualPort are all
 *        derived from PinError.
 */
struct VirtualPortError : PinError { };

/**
 * Partially implements a DigitalPort for use with testing without a port.
 *
 * @author  Jeff Jackowski
 */
class VirtualPort : public DigitalPortIndependentPins {
public:
	/**
	 * Make a VirtualPort object.
	 * @param numpins   The number of pins to make in the port.
	 * @param firstid   The gloabl ID that will be assigned to the first pin
	 *                  (local ID zero) of this port.
	 */
	VirtualPort(unsigned int numpins, unsigned int firstid = 0);
	/**
	 * Make a VirtualPort object.
	 * @param ids       The pin numbers to provide. The index of each
	 *                  inside @a ids will be the local pin ID used by this
	 *                  port. The value will be the global A value of -1 will create an unavailable pin and
	 *                  may be used multiple times. Other values must only be
	 *                  used once.
	 * @param firstid   The gloabl ID that will be assigned to the first pin
	 *                  (local ID zero) of this port.
	 * @throw DigitalPortDoesNotExistError  The device file could not be opened,
	 *                                      or the request for information on
	 *                                      the GPIO device ended in error.
	 * @throw DigitalPortLacksPinError      A request for information on a pin
	 *                                      ended in error.
	 */
	VirtualPort(
		const std::vector<unsigned int> &ids,
		unsigned int firstid = 0
	);
	/**
	 * Make a VirtualPort object according to the given configuration, and
	 * attach to the configuration.
	 * @param pc            The object with the port configuration data.
	 * @param name          The name of the port in the configuration.
	 * @throw PortDoesNotExistError         There is no port called @a name in
	 *                                      the given configuration.
	 * @throw DigitalPortDoesNotExistError  The device file could not be opened,
	 *                                      or the request for information on
	 *                                      the GPIO device ended in error.
	 * @throw DigitalPortLacksPinError      A request for information on a pin
	 *                                      ended in error.
	 */
	static std::shared_ptr<VirtualPort> makeConfiguredPort(
		PinConfiguration &pc,
		const std::string &name = "default"
	);
	virtual ~VirtualPort();
protected:
	/**
	 * Initializes a PinEntry object.
	 * @param offset  The line offset for the pin on the GPIO chip device.
	 * @param pid     The assigned port specific pin ID.
	 * @throw DigitalPortLacksPinError  The attempt to obtain information on
	 *                                  the GPIO line ended with an error.
	 */
	void initPin(std::uint32_t offset, unsigned int pid);
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

