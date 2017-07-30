/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPIN_HPP
#define DIGITALPIN_HPP

#include <duds/hardware/interface/DigitalPinBase.hpp>
#include <duds/hardware/interface/DigitalPinAccess.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Represents a single pin on a DigitalPort.
 * @author  Jeff Jackowski
 */
class DigitalPin : public DigitalPinBase {
	/**
	 * Global pin ID.
	 */
	unsigned int gid;
public:
	/**
	 * Constructs a DigitalPin object without a port or pin.
	 * @note  The pin ID will be uninitialized.
	 */
	DigitalPin() = default;
	/**
	 * Constructs a DigitalPin object with the given port and pin.
	 * @param port  A shared pointer to the DigitalPort object that handles
	 *              the pin.
	 * @param pin   The global pin ID for the pin to represent.
	 * @throw       PinDoesNotExist  The given pin does not exist in the
	 *                               given port.
	 */
	DigitalPin(const std::shared_ptr<DigitalPort> &port, unsigned int pin);
	/**
	 * Obtains an access object to the pin.
	 * @return  The access object inside a unique_ptr.
	 */
	std::unique_ptr<DigitalPinAccess> access() const {
		return port()->access(gid);
	}
	/**
	 * Obtains an access object to the pin.
	 * @param acc  A pointer to where the access object will be constructed. It
	 *             may be default constructed or uninitialized memory. It
	 *             <B>must not</B> be an access object that currently grants
	 *             access to any pin.
	 */
	void access(DigitalPinAccess *acc) const {
		port()->access(&gid, 1, acc);
	}
	/**
	 * Returns true if this object has been given a pin to represent.
	 */
	bool havePin() const {
		return (bool)port();
	}
	/**
	 * Returns the local pin ID of the represented pin.
	 */
	unsigned int localId() const {
		return port()->localId(gid);
	}
	/**
	 * Returns the global pin ID of the represented pin.
	 */
	unsigned int globalId() const {
		return gid;
	}
	/**
	 * Returns the global pin ID of the represented pin.
	 */
	operator unsigned int () const {
		return gid;
	}
	/**
	 * Returns the capabilities of the represented pin.
	 */
	DigitalPinCap capabilities() const {
		return port()->capabilities(gid);
	}
	/**
	 * Returns the current configuration of the represented pin.
	 * @warning  While the configuration will be correct for the time of the
	 *           query inside the DigitalPort::configuration() function, it
	 *           may change after that, even by the time this function returns,
	 *           if the configuration is altered on another thread. The only way
	 *           to ensure the configuration doesn't change this way is to
	 *           either not use more than one thread with the port, or query
	 *           from an access object for the pin. For this reason, convenience
	 *           functions for querying the current configuration are omitted
	 *           from this class.
	 */
	DigitalPinConfig configuration() const {
		return port()->configuration(gid);
	}
	/**
	 * Propose a new configuration for the represented pin using the current
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
	 * Propose a new configuration for the represented pin using a hypothetical
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

	// convenience functions -- may expand later

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
		return capabilities() & (
			DigitalPinCap::OutputPushPull |
			DigitalPinCap::OutputDriveLow |
			DigitalPinCap::OutputDriveHigh
		);
	}
	/**
	 * Returns true if the pin can provide a non-input high impedence state
	 * (or maybe allow input state?).
	 */
	bool canFloat() const {
		return capabilities() & DigitalPinCap::OutputHighImpedance;
	}
};

} } } // namespaces

#endif        //  #ifndef DIGITALPIN_HPP
