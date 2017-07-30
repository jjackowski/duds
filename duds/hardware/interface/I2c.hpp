/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef I2C_HPP
#define I2C_HPP

#include <boost/noncopyable.hpp>
#include <duds/hardware/interface/Conversationalist.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * A basic I2C interface. Could use some convenience functions.
 * @author  Jeff Jackowski
 */
class I2c : public Conversationalist, boost::noncopyable {
public:
	/**
	 * Conducts I2C communication with a device.
	 *
	 * The @ref ConversationPart::MpfVarlen "MpfVarlen" flag of ConversationPart
	 * may be optionally honored. The part is expected to have an adequately
	 * long buffer allocated before this call. Implementations should throw
	 * I2cErrorPartLength if the buffer is inadequate. The I2C standard, as
	 * published by NXP, does not seem to address incoming messages of varying
	 * length with the length sent by the device (slave), so there doesn't seem
	 * to be a good general minimum. Linux requires more than 32 bytes.
	 *
	 * The @ref ConversationPart::MpfBreak "MpfBreak" flag of ConversationPart
	 * objects should be honored by causing a stop condition, but it is
	 * implementation defined as to whether the bus may be used by other threads
	 * or processes before continuing with this conversation. A multi-master
	 * bus will always allow another master to start communicating after a stop
	 * condition. Conversation parts between set MpfBreak flags should be
	 * sent with a single stop condition.
	 *
	 * @param conv  The conversation to have with the device on the other end.
	 *
	 * The exceptions listed below may not include some that are specific to an
	 * implementation, but those exception classes should derive from I2cError.
	 *
	 * @throw I2cErrorConversationLength  The conversation has too many parts
	 *                                    for the implementation to handle.
	 * @throw I2cErrorPartLength   A variable length input part had a buffer
	 *                             that was too short.
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
	virtual void converse(Conversation &conv) = 0;
};

} } }

#endif        //  #ifndef I2C_HPP
