/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPORTDEPENDENTPINS_HPP
#define DIGITALPORTDEPENDENTPINS_HPP
#include <duds/hardware/interface/DigitalPort.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * A partial DigitalPort implementation for ports where the configuration
 * of one pin may affect the configuration of one or more other pins on the
 * same port. This is intended for use with hardware like the 74595 series;
 * they can manage high impedence on their outputs, but for all or none of
 * the pins.
 *
 * independentConfig(unsigned int, const DigitalPinConfig &) must be defined
 * by implementors since some configurations for some pins may not require
 * changes to any other pin.
 *
 * @todo  Implement a port made of a string of 74595s as an example.
 *
 * @author  Jeff Jackowski
 */
class DigitalPortDependentPins : public DigitalPort {
protected:
	DigitalPortDependentPins(unsigned int numpins, unsigned int firstid) :
	DigitalPort(numpins, firstid) { }
	/**
	 * Inspects a proposed change to the pin configuration to assure it meets
	 * all requirements and constraints of the proposal and the current
	 * configuration. This function is called once for each pin who's
	 * configuration has been requested to change and for which
	 * independentConfig() indicates will require changing other pin
	 * configurations to succeed. If all pin configurations are completely
	 * independent (all possible configurations for any pin one affects only
	 * that pin), this function does not need to be implemented.
	 *
	 * By the time this function is called, the configuration has been
	 * checked against the pin's DigitalPinCap and succeeded. However,
	 * this function must assure that changes to other dependent pin
	 * configurations are also within the hardware capabilities.
	 *
	 * @note  This function should not throw exceptions save for issues with
	 *        the function being buggy.
	 * @param localPinId  The local pin ID; matches the index in the @a pins
	 *                    and @a proposed vectors. It is the pin who's
	 *                    configuration is under consideration.
	 * @param proposed    A vector containing the resulting configuration with
	 *                    the proposal(s) applied. This function must modify
	 *                    the value based on the requested change to pin
	 *                    @a id. If a modification fails to meet the
	 *                    requirements, then the errant change should not be
	 *                    recorded in this vector.
	 * @param initial     A vector containing the initial configuration for the
	 *                    port. It may be different than the current port
	 *                    configuration. The change from the initial
	 *                    configuration held here to the proposed configuration
	 *                    is what the function must consider.
	 *                    @todo Are the configs here concrete, or may no-change
	 *                          values still be present?
	 * @return            The error flags for a bad configuration for this pin.
	 *                    If the port configuration is bad because of another
	 *                    pin's proposed configuration, the error should not be
	 *                    returned. This function will be called multiple times
	 *                    for the pins that will be changed.
	 */
	virtual DigitalPinRejectedConfiguration::Reason inspectProposal(
		unsigned int localPinId,
		std::vector<DigitalPinConfig> &proposed,
		std::vector<DigitalPinConfig> &initial
	) const = 0;
	/**
	 * Considers the proposed configuration for one pin, but may consider
	 * changes to other pins if the change is not independent. Port
	 * implementations do not have to redefine this function, but may if
	 * advantageous.
	 */
	virtual DigitalPinRejectedConfiguration::Reason proposeConfigImpl(
		unsigned int gid,
		DigitalPinConfig &pconf,
		DigitalPinConfig &iconf
	) const;
	/**
	 * Considers the proposed configuration one pin at a time in the order
	 * specified in @a pins. Port implementations do not have to redefine this
	 * function, but may if advantageous.
	 */
	virtual bool proposeConfigImpl(
		const std::vector<unsigned int> &pins,
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	/**
	 * Considers the proposed configuration one pin at a time for all pins in
	 * the port. It is faster and less complex than working over a subset of
	 * pins in an arbitrary order. Port implementations do not have to redefine
	 * this function, but may if advantageous.
	 */
	virtual bool proposeFullConfigImpl(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
public:
	/**
	 * Some pins have a configuration that is dependent on other pins, so this
	 * function always returns false.
	 */
	virtual bool independentConfig() const;
	// a reminder to C++ that this function has been declared; bother
	virtual bool independentConfig(
		unsigned int gid,
		const DigitalPinConfig &newcfg,
		const DigitalPinConfig &initcfg
	) const = 0;
};

} } }

#endif        //  #ifndef DIGITALPORTDEPENDENTPINS_HPP
