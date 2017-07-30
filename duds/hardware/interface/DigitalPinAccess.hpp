/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPINACCESS_HPP
#define DIGITALPINACCESS_HPP

#include <duds/hardware/interface/DigitalPort.hpp>
#include <duds/hardware/interface/DigitalPinAccessBase.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Provides access to a single pin on a DigitalPort.
 * @author  Jeff Jackowski
 */
class DigitalPinAccess : public DigitalPinAccessBase {
	/**
	 * Global pin ID.
	 */
	unsigned int gid;
	/**
	 * Used by DigitalPort.
	 * @param port         The port making this object.
	 * @param globalPinId  The global pin ID of the pin to represent.
	 */
	DigitalPinAccess(DigitalPort *port, unsigned int globalPinId) :
		DigitalPinAccessBase(port), gid(globalPinId) { }
	friend DigitalPort;
public:
	/**
	 * Constructs an access object with nothing to access.
	 * @note  The pin ID will be uninitialized.
	 */
	DigitalPinAccess() : DigitalPinAccessBase() { }
	/*
	 * A move constructor. Causes troubles internal to DigitalPort.
	 */
	//DigitalPinAccess(DigitalPinAccess &&old) noexcept;
	/**
	 * A move assignment. This requires a call to DigitalPort::updateAccess(),
	 * which needs to synchronize on its internal data. As a result, move
	 * assignments are not speedy. However, they assure pin access is transfered
	 * without being lost.
	 */
	DigitalPinAccess &operator=(DigitalPinAccess &&old);
	/**
	 * Relinquish access on destruction.
	 */
	~DigitalPinAccess() {
		retire();
	}
	/**
	 * Relinquish access.
	 */
	void retire() noexcept;
	/**
	 * Returns true if this object has been given a pin to access.
	 */
	bool havePin() const {
		return port() != nullptr;
	}
	/**
	 * Returns the local pin ID of the accessed pin.
	 */
	unsigned int localId() const {
		return port()->localId(gid);
	}
	/**
	 * Returns the global pin ID of the accessed pin.
	 */
	unsigned int globalId() const {
		return gid;
	}
	/**
	 * Returns the capabilities of the accessed pin.
	 */
	DigitalPinCap capabilities() const {
		return port()->capabilities(gid);
	}
	/**
	 * Returns the current configuration of the accessed pin.
	 */
	DigitalPinConfig configuration() const {
		return port()->configuration(gid);
	}
	/**
	 * Propose a new configuration for the accessed pin using the current
	 * configuration as the initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param conf  The proposed configuration.
	 * @return      The reason the configuration is bad, or
	 *              DigitalPinRejectedConfiguration::NotRejected if it can
	 *              be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		DigitalPinConfig &conf
	) const {
		return port()->proposeConfig(gid, conf);
	}
	/**
	 * Propose a new configuration for the accessed pin using a hypothetical
	 * given initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param proposed  The proposed configuration.
	 * @param initial   The initial configuration. It should be a valid
	 *                  configuration for the pin and port, but it doesn't
	 *                  need to be the current configuration.
	 * @return          The reason the configuration is bad, or
	 *                  DigitalPinRejectedConfiguration::NotRejected if it
	 *                  can be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		DigitalPinConfig &proposed,
		DigitalPinConfig &initial
	) const {
		return port()->proposeConfig(gid, proposed, initial);
	}
	/**
	 * Modifies the configuration of the pin. If the port implementation is a
	 * derivative of DigitalPortDependentPins, the change may affect multiple
	 * pins, and the configuration of other pins may prevent the requested
	 * change.
	 * @param conf  The requested new configuration.
	 * @return      The actual new configuration.
	 */
	DigitalPinConfig modifyConfig(const DigitalPinConfig &conf) const {
		return port()->modifyConfig(gid, conf);
	}
	/**
	 * Samples the input state of the pin.
	 * @throw PinWrongDirection    This pin is not configured as an input.
	 */
	bool input() const {
		return port()->input(gid);
	}
	/**
	 * Changes the output state of the pin. If the pin is not currently
	 * configured to output, the configuration will not change, but the new
	 * output state will be used when the pin becomes an output in the future.
	 * @param state  The new output state.
	 */
	void output(bool state) const {
		port()->output(gid, state);
	}

	// convenience functions -- may expand later

	/**
	 * Returns true if the pin is configured as an input.
	 */
	bool isInput() const {
		return configuration() & DigitalPinConfig::DirInput;
	}
	/**
	 * Returns true if the pin is configured as an output.
	 */
	bool isOutput() const {
		return configuration() & DigitalPinConfig::DirOutput;
	}
	/**
	 * Returns true if the pin can operate as an input.
	 */
	bool canBeInput() const {
		return capabilities() & DigitalPinCap::Input;
	}
	/**
	 * Returns true if the pin can operate as an output.
	 */
	bool canBeOutput() const {
		return capabilities().canOutput();
	}
	/**
	 * Returns true if the pin can provide a non-input high impedence state
	 * (or maybe allow input state?).
	 */
	bool canFloat() const {
		return capabilities() & DigitalPinCap::OutputHighImpedance;
	}
};

} } }

#endif        //  #ifndef DIGITALPINACCESS_HPP
