/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CONVERSATIONALIST_HPP
#define CONVERSATIONALIST_HPP

namespace duds { namespace hardware { namespace interface {

class Conversation;

/**
 * Allows a common interface for using Conversation objects for communication.
 * @author  Jeff Jackowski
 */
class Conversationalist {
public:
	/**
	 * Allow proper destruction using a Conversationalist pointer.
	 */
	virtual ~Conversationalist() = 0;
	/**
	 * Begins a half-duplex Conversation with a device.
	 * @param conv  The conversation to have with the device on the other end.
	 */
	virtual void converse(Conversation &conv) = 0;
};

} } }

#endif        //  #ifndef CONVERSATIONALIST_HPP
