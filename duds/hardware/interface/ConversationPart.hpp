/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CONVERSATIONPART_HPP
#define CONVERSATIONPART_HPP

#include <cstdint>
#include <duds/general/BitFlags.hpp>
#include <boost/exception/info.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Base class for conversation related errors.
 */
struct ConversationError : virtual std::exception, virtual boost::exception { };

/**
 * Represents a section of a half-duplex conversation with a device.
 *
 * The conversation part is stored in contiguous memory. It may either be used
 * for input or output.
 *
 * Several flags are defined that modify how a conversation part works or is
 * used. All flags are named for the set state and are clear (not set) by
 * default.
 *  - MpfInput: The part is used to hold data received from the other end.
 *  - MpfExtract: The part's data will be extracted by ConversationExtractor
 *                rather than skipped. This is normally set for input from
 *                a ConversationVector object, but clear for input from
 *                a ConversationExternal object.
 *  - MpfVarlen: Indicates a variable length input. Intended for use by
 *               communication code that takes Conversation objects.
 *  - MpfBigendian: Where not otherwise specified, data will be treated as
 *                  big-endian. Used by ConversationVector and
 *                  ConversationExtractor.
 *
 * @author  Jeff Jackowski
 */
class ConversationPart {
public:
	/**
	 * The type used to store flags that modify the operation of the classes
	 * derived from this class, or how the objects are used.
	 */
	typedef duds::general::BitFlags<struct ConversationPartFlags, std::uint16_t>
		Flags;
	/**
	 * True/set for input; false for output.
	 */
	static constexpr Flags MpfInput = Flags::Bit(0);
	/**
	 * True/set to extract message part contents with the ConversationExtractor.
	 * This is honored for both input and output messages.
	 */
	static constexpr Flags MpfExtract = Flags::Bit(1);
	/**
	 * True/set for a varying length; valid only for input. Intended for use by
	 * communication code that takes Conversation objects; they can use the flag
	 * to tell there is a need to properly set the input length.
	 */
	static constexpr Flags MpfVarlen = Flags::Bit(2);
	/**
	 * True/set to expect data to be big-endian.
	 */
	static constexpr Flags MpfBigendian = Flags::Bit(3);
	/**
	 * True/set to indicate that any kind of selection signal should be toggled,
	 * or a stop condition should occur, before communicating the part with this
	 * flag. The flag is only used by code that implements the communication.
	 * Implementations are not required to honor the flag. Some implementations
	 * may honor it, but be unable to guarantee that another communication from
	 * another thread or process will not occur before resuming the
	 * conversation. Implementations should specify these details in the
	 * documentation.
	 */
	static constexpr Flags MpfBreak = Flags::Bit(4);
private:
	/**
	 * A set of flags that alter the behavior of the message part.
	 */
	Flags mpf;
protected:
	/**
	 * A small integer for derived classes to use. Placed here because it won't
	 * increase the size of this class thanks to memory alignment.
	 */
	std::int16_t val16;
	/**
	 * Construct with the given flags.
	 */
	ConversationPart(Flags f) : mpf(f) { }
	/**
	 * Construct with the given flags and an initial value for @a val16.
	 */
	ConversationPart(Flags f, std::int16_t v) : mpf(f), val16(v) { }
public:
	/**
	 * Copies are OK.
	 */
	ConversationPart(const ConversationPart &) = default;
	virtual ~ConversationPart();
	/**
	 * Returns the flags. Values are:
	 *  - MpfInput
	 *  - MpfExtract
	 *  - MpfVarlen
	 *  - MpfBigendian
	 */
	Flags flags() const {
		return mpf;
	}
	/**
	 * True if this part is flagged for input use.
	 */
	bool input() const {
		return mpf & MpfInput;
	}
	/**
	 * True if this part is flagged for output use.
	 */
	bool output() const {
		return !(mpf & MpfInput);
	}
	/**
	 * True if this part is flagged for extraction by ConversationExtractor.
	 */
	bool extract() const {
		return mpf & MpfExtract;
	}
	/**
	 * True if this part is flagged as having a variable length. It is only
	 * valid for input.
	 */
	bool varyingLength() const {
		return (mpf & MpfVarlen) && (mpf & MpfInput);
	}
	/**
	 * True if this part is flagged as having data in big-endian form.
	 */
	bool bigEndian() const {
		return mpf & MpfBigendian;
	}
	/**
	 * Changes the flagged endianess of this part.
	 * @param big  True to flag as big-endian, false for little-endian.
	 */
	void bigEndian(bool big) {
		mpf.setTo(MpfBigendian, big);
	}
	/**
	 * True if this part is flagged as having data in little-endian form.
	 */
	bool littleEndian() const {
		return !(mpf & MpfBigendian);
	}
	/**
	 * Changes the flagged endianess of this part.
	 * @param little  True to flag as little-endian, false for big-endian.
	 */
	void littleEndian(bool little) {
		mpf.setTo(MpfBigendian, !little);
	}
	/**
	 * Returns a pointer to the begining of the conversation part's buffer.
	 * The return type is not const because use for input will require a write
	 * operation. The implementation must not make changes to the part object,
	 * so the function is const.
	 */
	virtual char *start() const = 0;
	/**
	 * Returns the length of the buffer following the start pointer.
	 */
	virtual std::size_t length() const = 0;
};

} } }

#endif        //  #ifndef CONVERSATIONPART_HPP
