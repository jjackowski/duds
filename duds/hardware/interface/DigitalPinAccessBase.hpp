/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPINACCESSBASE_HPP
#define DIGITALPINACCESSBASE_HPP

#include <boost/noncopyable.hpp>

namespace duds { namespace hardware { namespace interface {

class DigitalPort;

// do not keep objects of this type
/**
 * The base class for the digital pin access classes. This base class stores
 * a pointer to the DigitalPort handling the pins. A shared pointer is not used
 * to speed up the creation and destruction of the access objects. Outside of
 * DigitalPin, this class should not be used directly.
 * @author  Jeff Jackowski
 */
class DigitalPinAccessBase : boost::noncopyable {
	/**
	 * A pointer to the port object handling the pin(s).
	 */
	DigitalPort *dp;
	friend DigitalPort;
protected:
	/**
	 * Cannot be constructed using this base class, but allows the construction
	 * of a useless access object without a DigitalPort.
	 */
	DigitalPinAccessBase() : dp(nullptr) { };
	/**
	 * Initializes the port pointer.
	 */
	DigitalPinAccessBase(DigitalPort *port) : dp(port) { }
	/**
	 * Cannot be destructed using this base class to avoid the need for a
	 * virtual destructor. There is no point to keeping objects of this
	 * base class.
	 */
	~DigitalPinAccessBase() = default;
	/**
	 * Allows moving access objects.
	 */
	DigitalPinAccessBase &operator=(DigitalPinAccessBase &&old) noexcept;
	/**
	 * Loses the pointer to the DigitalPort rendering the access object useless.
	 */
	void reset() {
		dp = nullptr;
	}
public:
	/**
	 * Returns a pointer to the port that controls the pin(s) that are operated
	 * through this object. The result will be nullptr if the access object
	 * was constructed without a DigitalPort. During the life of this object,
	 * the result must never change, and must remain valid. These requirements
	 * allow the use of the pointer without checking for validity. Further,
	 * the pin itslef must remain available. This allows some operations to
	 * occur without thread synchronization. The result is relatively fast
	 * access to the port and the pin.
	 */
	DigitalPort *port() const {
		/** @todo Throw if dp is null? */
		return dp;
	}
};

} } }

#endif        //  #ifndef DIGITALPINACCESSBASE_HPP
