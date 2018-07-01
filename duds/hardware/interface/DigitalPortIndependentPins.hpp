/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPORTINDEPENDENTPINS_HPP
#define DIGITALPORTINDEPENDENTPINS_HPP

#include <duds/hardware/interface/DigitalPort.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * A partial DigitalPort implementation for ports where the configuration of
 * each pin is independent of the configuration of all other pins.
 * @author  Jeff Jackowski
 */
class DigitalPortIndependentPins : public DigitalPort {
public:
	/**
	 * Always true.
	 */
	virtual bool independentConfig() const;
	/**
	 * Always true.
	 */
	virtual bool independentConfig(
		unsigned int,
		const DigitalPinConfig &,
		const DigitalPinConfig &
	) const;
protected:
	DigitalPortIndependentPins(unsigned int numpins, unsigned int firstid) :
	DigitalPort(numpins, firstid) { }
	/**
	 * Considers the proposed configuration for one pin. Port
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
		const std::vector<unsigned int> &localPinIds,
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	/**
	 * Considers the proposed configuration one pin at a time for all pins in
	 * the port. Port implementations do not have to redefine
	 * this function, but may if advantageous.
	 */
	virtual bool proposeFullConfigImpl(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	/**
	 * Changes the hardware configuration for the whole port by calling
	 * configurePort(const DigitalPinConfig &, unsigned int, DigitalPinAccessBase::PortData *)
	 * for each pin.
	 * This makes sense for ports that do not support simultaneous operations,
	 * ports that do not benifit from them for configuration, and for tesing
	 * an implementation prior to fully implementing simultaneous operations.
	 * @param cfgs   The new configuration. The indices are the local pin IDs.
	 *               The size must match the size of @a pins.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pins.
	 */
	virtual void configurePort(
		const std::vector<DigitalPinConfig> &cfgs,
		DigitalPinAccessBase::PortData *pdata
	);
	// a reminder to C++ that this function has been declared; bother
	virtual void configurePort(
		unsigned int localPinId,
		const DigitalPinConfig &cfg,
		DigitalPinAccessBase::PortData *pdata
	) = 0;
};

} } }

#endif        //  #ifndef DIGITALPORTINDEPENDENTPINS_HPP
