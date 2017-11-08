/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ConversationPart.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * References a conversation part in an externally controlled buffer. The
 * contents of the buffer are not copied for output parts, nor are they ever
 * modified. Input parts will have new data writen directly into the buffer.
 * @todo  Rename to ConversationBuffer.
 * @author  Jeff Jackowski
 */
class ConversationExternal : public ConversationPart {
	/**
	 * Points to the start of the external buffer.
	 */
	char *data;
	/**
	 * Length of the external buffer.
	 */
	std::size_t len;
public:
	/**
	 * Copies are ok.
	 */
	ConversationExternal(const ConversationExternal &) = default;
	/**
	 * Creates an output part from the given buffer.
	 * @param a       The start of the buffer with the output data. It is not
	 *                copied; any changes to the buffer will affect the
	 *                containing Conversation.
	 * @param length  The length of the buffer in bytes.
	 * @param flags   Flags that modify how the part will be used.
	 */
	ConversationExternal(
		const char *a,
		std::size_t length,
		Flags flags = Flags::Zero()
	) : ConversationPart(flags), data(const_cast<char*>(a)), len(length) { }
	/**
	 * Creates an input part from the given buffer.
	 * @param a       The start of the buffer that will take the intput data.
	 * @param length  The length of the buffer in bytes.
	 * @param flags   Flags that modify how the part will be used.
	 */
	ConversationExternal(char *a, std::size_t length, Flags flags = MpfInput) :
		ConversationPart(flags), data(a), len(length) { }
	/**
	 * Creates an output part from the given array.
	 * @tparam T  The array element type. It must be an integral type.
	 * @tparam N  The length of the array.
	 * @param  a  The array with the output data. It is not copied; any changes
	 *            will affect the containing Conversation.
	 * @param  f  Flags that modify how the part will be used.
	 */
	template <typename T, std::size_t N>
	ConversationExternal(const T (&a)[N], Flags f = Flags::Zero()) :
		ConversationPart(f),
		data((char*)(const_cast<T(&)[N]>(a))),
		len(N * sizeof(T)) { }
	/**
	 * Creates an input part from the given array.
	 * @tparam T  The array element type. It must be an integral type.
	 * @tparam N  The length of the array.
	 * @param  a  The array where the input data will be written. It is handled
	 *            internally as bytes. Variable length input may require
	 *            writing a length to the start of the array. Since the size of
	 *            the length data is implementation dependent, it is best to
	 *            not use this class for variable length input.
	 * @param  f  The flags that modify the operation of the part.
	 */
	template <typename T, std::size_t N>
	ConversationExternal(T (&a)[N], Flags f = MpfInput) :
		ConversationPart(f), data((char*)(a)), len(N * sizeof(T)) { }
	virtual char *start() const;
	virtual std::size_t length() const;
};

} } }
