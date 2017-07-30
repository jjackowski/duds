/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPINBASE_HPP
#define DIGITALPINBASE_HPP

#include <memory>

namespace duds { namespace hardware { namespace interface {

class DigitalPort;

/**
 * A base class for classes that represent one or more pins on a single
 * DigitalPrt, but do not provide access. There will likely be no need or
 * reason to keep objects of this type.
 * @author  Jeff Jackowski
 */
class DigitalPinBase {
	/**
	 * The port object that can grant access to the pin(s).
	 */
	std::shared_ptr<DigitalPort> dp;
protected:
	/**
	 * Cannot be constructed using this base class, but allows the construction
	 * of a useless object without a DigitalPort.
	 */
	DigitalPinBase() = default;
	/**
	 * Cannot be destructed using this base class to avoid the need for a
	 * virtual destructor. There is no point to keeping objects of this
	 * base class.
	 */
	~DigitalPinBase() = default;
	/**
	 * Initializes the port.
	 */
	DigitalPinBase(const std::shared_ptr<DigitalPort> &port) : dp(port) { }
public:
	/**
	 * Returns the port that grants access to the pin(s) referenced by this
	 * object.
	 */
	const std::shared_ptr<DigitalPort> &port() const {
		return dp;
	}
};

} } }

#endif        //  #ifndef DIGITALPINBASE_HPP
