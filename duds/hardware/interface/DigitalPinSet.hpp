/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPINSET_HPP
#define DIGITALPINSET_HPP

#include <duds/hardware/interface/DigitalPinBase.hpp>
#include <duds/hardware/interface/DigitalPinSetAccess.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Represents a set of pins on a single DigitalPort.
 * @author  Jeff Jackowski
 */
class DigitalPinSet : public DigitalPinBase {
	/**
	 * The port global pin IDs this object will represent.
	 */
	std::vector<unsigned int> pinvec;
public:
	/**
	 * Constructs DigitalPinSet object with nothing to represent.
	 */
	DigitalPinSet() = default;
	/**
	 * Constructs a DigitalPinSet object with the given port and pins.
	 * @param port  A shared pointer to the DigitalPort object that handles
	 *              the pins.
	 * @param pvec  A vector with the global pin IDs for all the pins to
	 *              represent. A value of -1 represents a gap, the lack  of a
	 *              pin at the corresponding position. The only value that may
	 *              be repeated is -1.
	 * @throw       PinDoesNotExist  A given pin does not exist in the
	 *                               given port.
	 * @throw       DigitalPortDoesNotExistError
	 */
	DigitalPinSet(
		const std::shared_ptr<DigitalPort> &port,
		const std::vector<unsigned int> &pvec
	);
	/**
	 * Constructs a DigitalPinSet object with the given port and pins.
	 * @param port  A shared pointer to the DigitalPort object that handles
	 *              the pins.
	 * @param pvec  A vector with the global pin IDs for all the pins to
	 *              represent. A value of -1 represents a gap, the lack  of a
	 *              pin at the corresponding position. The only value that may
	 *              be repeated is -1.
	 *              The value is moved to an internal vector. If an exception
	 *              is thrown, the value is moved back.
	 * @throw       PinDoesNotExist  A given pin does not exist in the
	 *                               given port.
	 * @throw       DigitalPortDoesNotExistError
	 */
	DigitalPinSet(
		const std::shared_ptr<DigitalPort> &port,
		std::vector<unsigned int> &&pvec
	);
	/**
	 * Obtains an access object for all the pins in this set.
	 * @return  The access object inside a unique_ptr.
	 */
	std::unique_ptr<DigitalPinSetAccess> access() const {
		return port()->access(pinvec);
	}
	/**
	 * Obtains an access object for all the pins in this set.
	 * @param acc  The access object that will be given control over the pins.
	 *             If it already grants access to pins, it will relinquish
	 *             that access first.
	 */
	void access(DigitalPinSetAccess &acc) const {
		acc.retire();
		port()->access(pinvec, acc);
	}
	/**
	 * Provides access to the internal vector of global pin IDs.
	 */
	const std::vector<unsigned int> &globalIds() const {
		return pinvec;
	}
	/**
	 * Returns a vector of port-local pin IDs for the pins represented by this
	 * object.
	 */
	std::vector<unsigned int> localIds() const {
		return port()->localIds(pinvec);
	}
	/**
	 * Returns true if this object has been given any pins to represent.
	 */
	bool havePins() const {
		return (port() != nullptr) && !pinvec.empty();
	}
	/**
	 * Returns true if the given position is for an existent pin rather than
	 * a gap or a position past the end.
	 * @param pos  The position to check.
	 */
	bool exists(unsigned int pos) const {
		return (pos < pinvec.size()) && (pinvec[pos] != -1);
	}
	/**
	 * Returns the number of pins in this object. The count includes pins set
	 * as -1; gaps in the pins to access.
	 */
	unsigned int size() const {
		return (unsigned int)pinvec.size();
	}
	/**
	 * Returns the local pin ID of the pin at the given position inside this
	 * set of pins.
	 * @param pos  The position of the pin in this set.
	 */
	unsigned int localId(unsigned int pos) const {
		// check range using at()
		return port()->localId(pinvec.at(pos));
	}
	/**
	 * Returns the global pin ID of the pin at the given position inside this
	 * set of pins.
	 * @param pos  The position of the pin in this set.
	 */
	unsigned int globalId(unsigned int pos) const {
		// check range using at()
		return pinvec.at(pos);
	}
	/**
	 * Returns the capabilities of the specified pin.
	 * @param pos  The position of the pin in this set.
	 */
	DigitalPinCap capabilities(unsigned int pos) const {
		return port()->capabilities(pinvec.at(pos));
	}
	/**
	 * Returns the capabilities of all the pins in this set. Any gaps in the
	 * set (ID is -1) will have a value of
	 * DigitalPinCap::Nonexistent.
	 */
	std::vector<DigitalPinCap> capabilities() const {
		return port()->capabilities(pinvec);
	}
	/**
	 * Returns the current configuration of the specified pin.
	 * @param pos  The position of the pin in this set.
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
	DigitalPinConfig configuration(unsigned int pos) const {
		return port()->configuration(pinvec.at(pos));
	}
	/**
	 * Returns the current configuration all pins in this set. Any gaps in the
	 * set (ID is -1) will have a configuration of
	 * DigitalPinConfig::OperationNoChange.
	 * @warning  While the configuration will be correct for the time of the
	 *           query inside the DigitalPort::configuration() function, it
	 *           may change after that, even by the time this function returns,
	 *           if the configuration is altered on another thread. The only way
	 *           to ensure the configuration doesn't change this way is to
	 *           either not use more than one thread with the port, or query
	 *           from an access object for the set of pins. For this reason,
	 *           convenience functions for querying the current configuration
	 *           are omitted from this class.
	 */
	std::vector<DigitalPinConfig> configuration() const {
		return port()->configuration(pinvec);
	}
	/**
	 * Propose a new configuration for the given pin using the current
	 * configuration as the initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param conf  The proposed configuration.
	 * @param pos   The position of the pin in this set.
	 * @return      The reason the configuration is bad, or
	 *              DigitalPinRejectedConfiguration::NotRejected if it can
	 *              be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int pos,
		DigitalPinConfig &conf
	) const {
		return port()->proposeConfig(pinvec.at(pos), conf);
	}
	/**
	 * Propose a new configuration for the given pin using a hypothetical
	 * given initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param proposed  The proposed configuration.
	 * @param initial   The initial configuration. It should be a valid
	 *                  configuration for the pin and port, but it doesn't
	 *                  need to be the current configuration.
	 * @param pos       The position of the pin in this set.
	 * @return          The reason the configuration is bad, or
	 *                  DigitalPinRejectedConfiguration::NotRejected if it
	 *                  can be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int pos,
		DigitalPinConfig &proposed,
		DigitalPinConfig &initial
	) const {
		return port()->proposeConfig(pinvec.at(pos), proposed, initial);
	}
	/**
	 * Propose a new configuration for the entire pin set using a hypothetical
	 * given initial configuration. The configuration is not changed; this
	 * only checks for a valid supported change.
	 * @param propConf      The proposed configuration.
	 * @param initConf      The initial configuration. It should be a valid
	 *                      configuration for the pins and port, but it doesn't
	 *                      need to be the current configuration.
	 * @param insertReason  A function that, if specified, will be called for
	 *                      each pin, in the order specified by this set, with
	 *                      the rejection reason for that pin. The reason will
	 *                      be DigitalPinRejectedConfiguration::NotRejected if
	 *                      the pin's proposed configuration is good. The
	 *                      function is optional.
	 * @return              True if the proposed configuration is good. False
	 *                      if there was any rejection.
	 */
	bool proposeConfig(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) {
		return port()->proposeConfig(
			pinvec,
			propConf,
			initConf,
			insertReason
		);
	}

	// convenience functions -- may expand later

	/**
	 * True if the port supports operating on multiple pins simultaneously. If
	 * false, the pins may be modified on over a period of time in an
	 * implementation defined order.
	 */
	bool simultaneousOperations() const {
		return port()->simultaneousOperations();
	}
	/**
	 * Returns true if all pins on the port always have an independent
	 * configuration from all other pins.
	 */
	bool independentConfig() const {
		return port()->independentConfig();
	}
	/**
	 * Returns true if the pin can operate as an input.
	 * @param pos  The position of the pin in this set.
	 */
	bool canBeInput(unsigned int pos) const {
		return capabilities(pos) & DigitalPinCap::Input;
	}
	/**
	 * Returns true if the pin can operate as an output.
	 * @param pos  The position of the pin in this set.
	 */
	bool canBeOutput(unsigned int pos) const {
		return capabilities(pos) & DigitalPinCap::OutputDriveMask;
	}
	/**
	 * Returns true if the pin can provide a non-input high impedence state
	 * (or maybe allow input state?).
	 * @param pos  The position of the pin in this set.
	 */
	bool canFloat(unsigned int pos) const {
		return capabilities(pos) & DigitalPinCap::OutputHighImpedance;
	}
};

} } }

#endif        //  #ifndef DIGITALPINSET_HPP
