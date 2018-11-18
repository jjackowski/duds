/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/MasterSyncSerial.hpp>
#include <initializer_list>

namespace duds { namespace hardware { namespace interface {

/**
 * Provides access for communicating using a MasterSyncSerial object.
 *
 * Not derived from Conversationalist to avoid a virtual function table.
 * Not sure if that is a good or bad decision. The objects are expected to be
 * made on the stack just before use and then destroyed, giving them a short
 * lifespan, so this may reduce overhead a little. The use of Conversationalist
 * seems to be more sensible on MasterSyncSerial, which is a Conversationalist.
 * A converse() function with the same prototype and usage as defined in
 * Conversationalist is part of this class.
 *
 * @author  Jeff Jackowski
 */
class MasterSyncSerialAccess : boost::noncopyable {
	/**
	 * MasterSyncSerial::access() calls the constructor.
	 */
	friend std::unique_ptr<MasterSyncSerialAccess> MasterSyncSerial::access();
	/**
	 * MasterSyncSerial::access(MasterSyncSerialAccess &) changes @a mss.
	 */
	friend void MasterSyncSerial::access(MasterSyncSerialAccess &);
	friend void MasterSyncSerial::retire(MasterSyncSerialAccess *);
	/**
	 * The serial interface used by this access object.
	 */
	std::shared_ptr<MasterSyncSerial> mss;
	/**
	 * Constructs an access object; called by MasterSyncSerial::access().
	 */
	MasterSyncSerialAccess(std::shared_ptr<MasterSyncSerial> m) : mss(m) { }
public:
	typedef std::initializer_list<std::uint8_t>  ByteList;
	//typedef std::vector<std::uint8_t>  ByteVector;
	/**
	 * Makes a useless access object.
	 */
	MasterSyncSerialAccess() { }
	/**
	 * Ends the conversation and closes communication.
	 */
	~MasterSyncSerialAccess() {
		retire();
	}
	/**
	 * Ends the conversation and closes communication.
	 */
	void retire() {
		if (mss) {
			mss->retire(this);
		}
	}
	/**
	 * Starts a conversation; transitions from the open state to
	 * the communicating state.
	 */
	void start() {
		mss->condStart();
	}
	/**
	 * Ends the conversation; transitions from the communicating
	 * state to the open state.
	 */
	void stop() {
		mss->condStop();
	}
	/**
	 * Sends and/or receives @a bits of data. If full duplex communication is
	 * not supported, one of the buffers should be given a NULL pointer. The
	 * buffers must not overlap.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  out   The data to transmit, or NULL to not transmit.
	 * @param  in    The buffer that will receive data, or NULL if nothing will
	 *               be received.
	 * @param  bits  The number of bits to transfer. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotFullDuplex  Both @a out and @a in are non-NULL, but
	 *                                  the serial interface is half-duplex.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transfer(
		const std::uint8_t * __restrict__ out,
		std::uint8_t * __restrict__ in,
		duds::general::Bits bits
	) {
		mss->transfer(out, in, bits);
	}
	/**
	 * Sends and/or receives @a bits of data using signed bytes for convenience.
	 * If full duplex communication is
	 * not supported, one of the buffers should be given a NULL pointer. The
	 * buffers must not overlap.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  out   The data to transmit, or NULL to not transmit.
	 * @param  in    The buffer that will receive data, or NULL if nothing will
	 *               be received.
	 * @param  bits  The number of bits to transfer. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotFullDuplex  Both @a out and @a in are non-NULL, but
	 *                                  the serial interface is half-duplex.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transfer(
		const std::int8_t * __restrict__ out,
		std::int8_t * __restrict__ in,
		duds::general::Bits bits
	) {
		transfer((std::uint8_t*)out, (std::uint8_t*)in, bits);
	}
	/**
	 * Sends and/or receives @a bits of data. If full duplex communication is
	 * not supported, one of the buffers should be given a NULL pointer. The
	 * buffers must not overlap.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  out   The data to transmit in an initializer list.
	 * @param  in    The buffer that will receive data, or NULL if nothing will
	 *               be received.
	 * @param  bits  The number of bits to transfer. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotFullDuplex  Both @a out and @a in are non-NULL, but
	 *                                  the serial interface is half-duplex.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transfer(
		const ByteList &out,
		std::uint8_t * __restrict__ in,
		duds::general::Bits bits
	) {
		mss->transfer(out.begin(), in, bits);
	}
	/**
	 * Sends @a bits of data. If full duplex communication is used,
	 * received data is lost.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  buff  The data to transmit.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transmit(const std::uint8_t *buff, duds::general::Bits bits) {
		mss->transmit(buff, bits);
	}
	/**
	 * Sends @a bits of data using signed bytes for convenience. If full duplex
	 * communication is used, received data is lost.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  buff  The data to transmit.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transmit(const std::int8_t *buff, duds::general::Bits bits) {
		transmit((std::uint8_t*)buff, bits);
	}
	/**
	 * Sends bytes stored in a temporary value. If full duplex
	 * communication is used, received data is lost.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  buff  The data to transmit as a list of bytes.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void transmit(const ByteList &buff) {
		mss->transmit(buff.begin(), duds::general::Bytes(buff.size()));
	}
	/**
	 * Receives @a bits of data. If full duplex communication is used,
	 * transmitted data is undefined unless an implementation cares to have a
	 * definition.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  buff  The buffer that will receive the data.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void receive(std::uint8_t *buff, duds::general::Bits bits) {
		mss->receive(buff, bits);
	}
	/**
	 * Receives @a bits of data using signed bytes for convenience. If full
	 * duplex communication is used,
	 * transmitted data is undefined unless an implementation cares to have a
	 * definition.
	 * @pre    The object is in the communicating state from a prior call to
	 *         start().
	 * @param  buff  The buffer that will receive the data.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	void receive(std::int8_t *buff, duds::general::Bits bits) {
		receive((std::uint8_t*)buff, bits);
	}
	/**
	 * Has a half-duplex Conversation with the connected device. The
	 * Conversation object defines all input and output parameters. On the
	 * ConversationPart objects, the @ref ConversationPart::MpfBreak "MpfBreak"
	 * flag is honored, but the @ref ConversationPart::MpfVarlen "MpfVarlen"
	 * flag is ignored. The transmit() and receive() functions are called to
	 * move the data.
	 * @pre   The object is in the open state or the communicating state.
	 * @post  The object is in the open state, but not the communicating state.
	 * @param conv  The conversation to have with the device on the other end.
	 * @throw SyncSerialIoError     An error prevented the communication.
	 */
	void converse(Conversation &conv) {
		mss->converseAlreadyOpen(conv);
	}
};

} } }
