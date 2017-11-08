/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CONVERSATIONVECTOR_HPP
#define CONVERSATIONVECTOR_HPP

#include <type_traits>
#include <vector>
#include <duds/hardware/interface/ConversationPart.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * An attempt was made to add data to a conversation part flagged for input.
 */
struct ConversationBadAdd : ConversationError { };

/**
 * An operation requiring a varible length conversation part was attempted
 * on a part not flagged as having a variable length.
 */
struct ConversationFixedLength : ConversationError { };

/**
 * An attempt was made to change the starting offset of a ConversationVector
 * to an invalid value. A valid value must leave other data in the vector and
 * must not be negative. The ConversationVector must be flagged as having a
 * varying length or any offset change is invalid.
 */
struct ConversationBadOffset : ConversationError { };

/**
 * Holds a conversation part inside a vector.
 *
 * Output data can be placed in the part using the various add() functions.
 * Templates are used to avoid the need to specify the data size when arrays
 * are used. addBe() functions will put the given data in big-endian form,
 * while addLe() functions will use little-endian. Other add functions will
 * use the endianess indicated in the part flags (ConversationPart::flags()).
 * None of the add functions modify the endianess flag. The default endianess
 * is little-endian.
 *
 * The insertion operator is defined for this class and uses the add functions.
 * It is also defined to use structs within this class to modify the part in
 * other ways. The BigEndian struct will set the big-endian flag and affect
 * data added later using means other than the addLe() functions. LittleEndian
 * does the same, but selects little-endian data. The Reserve struct will cause
 * reserve() to be called.
 *
 * Variable length inputs are implemented by allowing resizing of the
 * internal vector. The start pointer can be offset from the start of the
 * vector using setStartOffset(). The offset is stored in @a val16. This allows
 * input using a large enough vector (defined by communication protocol)
 * that starts with the size of the input.
 *
 * @author  Jeff Jackowski
 */
class ConversationVector : public ConversationPart {
	/**
	 * The internal buffer.
	 */
	std::vector<char> data;
public:
	/**
	 * Used as a parameter to constructors to specify an input conversation part.
	 */
	struct Input { };
	/**
	 * Used as a parameter to constructors to specify an output conversation part.
	 */
	struct Output { };
	/**
	 * Used as a parameter to constructors to specify that communication will
	 * not change the length of the conversation part. This means that an
	 * input part's length is known prior to starting the conversation. It does
	 * not mean that the part's length cannot be changed prior to being used
	 * to communicate data.
	 */
	struct FixedLength { };
	/**
	 * Used as a parameter to constructors to specify that communication may
	 * change the length of the part. This is valid only for input. The length
	 * of the input should be set to the maximum that could be received. After
	 * communication, the part should be resized to the amount of received data.
	 * This resizing is normally handled by the communication code, such as
	 * I2C support, rather than the code that builds the conversation objects.
	 */
	struct VaribleLength { };
	/**
	 * Copies are OK.
	 */
	ConversationVector(const ConversationVector &) = default;
	/**
	 * Construct for fixed-length input without allocating space for the input.
	 */
	ConversationVector(Input) : ConversationPart(MpfInput | MpfExtract , 0) { };
	/**
	 * Construct for output.
	 */
	ConversationVector(Output) : ConversationPart(Flags::Zero(), 0) { };
	/**
	 * Construct for either output or fixed-length input.
	 * @param input  True for input.
	 */
	ConversationVector(bool input) : ConversationPart(
		input ? MpfInput | MpfExtract : Flags::Zero(), 0
	) { };
	/**
	 * Construct for output and copy the given vector into this object.
	 */
	ConversationVector(const std::vector<char> &v) :
		ConversationPart(0), data(v) { }
	/**
	 * Construct for output and move the given vector into this object.
	 */
	ConversationVector(std::vector<char> &&v) :
		ConversationPart(0), data(std::move(v)) { }
	//ConversationVector(char * array, int len, bool input) :
	//	ConversationPart(input), data(v) { }
	/**
	 * Construct for fixed-length input and allocate space for the input.
	 * @param length  The number of bytes to allocate for the input.
	 */
	ConversationVector(std::size_t length, FixedLength) :
		ConversationPart(
			MpfInput | MpfExtract, 0
		), data(length + val16) { }
	/**
	 * Construct for variable-length input and allocate space for the input.
	 * @param length  The number of bytes to allocate for the input. It should
	 *                be the maximum size possible for the input.
	 */
	ConversationVector(std::size_t length, VaribleLength) :
		ConversationPart(
			MpfInput | MpfExtract | MpfVarlen, sizeof(std::size_t)
		), data(length + val16) { }
	/**
	 * Construct for fixed or variable-length input and allocate space for the
	 * input.
	 * @param length  The number of bytes to allocate for the input.
	 * @param varlen  True for a variable-length input; false for fixed-length.
	 */
	ConversationVector(std::size_t length, bool varlen = false) :
		ConversationPart(
			MpfInput | MpfExtract | (varlen ? MpfVarlen : Flags::Zero()),
			varlen ? sizeof(std::size_t) : 0
		), data(length + val16) { }
	/**
	 * Construct for output and reserve the given length within the internal
	 * vector.
	 * @param length  The number of bytes to reserve.
	 */
	ConversationVector(std::size_t length, Output) :
		ConversationPart(Flags::Zero(), 0) {
			data.reserve(length);
		};
	/**
	 * Begining iterator to inspect the contained data.
	 */
	std::vector<char>::const_iterator begin() const {
		return data.begin();
	}
	/**
	 * Ending iterator to inspect the contained data.
	 */
	std::vector<char>::const_iterator end() const {
		return data.end();
	}
	/**
	 * Adds a byte to an output part.
	 * @param i  The character to add to the end of the part.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	void add(char i);
	/**
	 * Adds an integer in little-endian form to an output part.
	 * @tparam Int  The integer type.
	 * @param  i    The integer value to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void addLe(Int i) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be added by this function.");
		if (input()) {
			throw ConversationBadAdd();
		}
		for (std::size_t loop = sizeof(Int); loop; --loop) {
			data.push_back((char)(i & 0xFF));
			i >>= 8;
		}
	}
	/**
	 * Adds an integer in big-endian form to an output part.
	 * @tparam Int  The integer type.
	 * @param  i    The integer value to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void addBe(Int i) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be added by this function.");
		if (input()) {
			throw ConversationBadAdd();
		}
		for (std::size_t loop = sizeof(Int); loop; --loop) {
			data.push_back((char)(i >> (8 * (sizeof(Int) - 1))));
			i <<= 8;
		}
	}
	/**
	 * Adds a byte to an output part. Avoids extra endian code in cases where
	 * template code is given a byte type as a parameter.
	 * @param i  The character to add to the end of the part.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	void addLe(std::int8_t i) {
		add((char)i);
	}
	/**
	 * Adds a byte to an output part. Avoids extra endian code in cases where
	 * template code is given a byte type as a parameter.
	 * @param i  The character to add to the end of the part.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	void addBe(std::int8_t i) {
		add((char)i);
	}
	/**
	 * Adds an integer to the end an output part. The endianess is chosen based
	 * on the part's MpfBigendian flag.
	 * @tparam Int  The integer type.
	 * @param  i    The integer value to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void add(Int i) {
		if (bigEndian()) {
			addBe(i);
		} else {
			addLe(i);
		}
	}
	/**
	 * Adds a string as binary data to the end an output part.
	 * @param  str  The string to add to the end of the part.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	void add(const std::string &str);
	/**
	 * Adds binary data to the end of an output part.
	 * @param a   The start of the binary data to add to the end of the part.
	 * @param l   The number of bytes to copy.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	void add(std::int8_t *a, std::size_t l);
	/**
	 * Adds an array to the end an output part.
	 * @tparam N    The length of the array.
	 * @param  a    The array to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <std::size_t N>
	void add(std::int8_t (&a)[N]) {
		add(a, N);
	}
	/**
	 * Adds an array of integers to the end of an output part in little-endian
	 * form.
	 * @tparam Int   The integer type.
	 * @param  a      The start of the array to add to the end of the part.
	 * @param  count  The number of items to copy.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void addLe(const Int *a, std::size_t count) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be added by this function.");
		if (input()) {
			throw ConversationBadAdd();
		}
		data.reserve(data.size() + sizeof(Int) * count);
		for (; count; --count) {
			Int i = *(a++);
			for (std::size_t loop = sizeof(Int); loop; --loop) {
				data.push_back((char)(i & 0xFF));
				i >>= 8;
			}
		}
	}
	/**
	 * Adds an array of integers to the end of an output part in little-endian
	 * form.
	 * @tparam Int  The integer type.
	 * @tparam N    The length of the array.
	 * @param  a    The array to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int, std::size_t N>
	void addLe(const Int (&a)[N]) {
		addLe(a, N);
	}
	/**
	 * Adds an array of integers to the end of an output part in big-endian
	 * form.
	 * @tparam Int   The integer type.
	 * @param  a      The start of the array to add to the end of the part.
	 * @param  count  The number of items to copy.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void addBe(const Int *a, std::size_t count) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be added by this function.");
		if (input()) {
			throw ConversationBadAdd();
		}
		data.reserve(data.size() + sizeof(Int) * count);
		for (; count; --count) {
			Int i = *(a++);
			for (std::size_t loop = sizeof(Int); loop; --loop) {
				data.push_back((char)(i >> (8 * (sizeof(Int) - 1))));
				i <<= 8;
			}
		}
	}
	/**
	 * Adds an array of integers to the end of an output part in big-endian
	 * form.
	 * @tparam Int  The integer type.
	 * @tparam N    The length of the array.
	 * @param  a    The array to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int, std::size_t N>
	void addBe(const Int (&a)[N]) {
		addBe(a, N);
	}
	/**
	 * Adds an array of integers to the end of an output part. The endianess is
	 * chosen based on the part's MpfBigendian flag.
	 * @tparam Int   The integer type.
	 * @param a      The start of the array to add to the end of the part.
	 * @param count  The number of items to copy.
	 * @throw ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int>
	void add(const Int *a, std::size_t count) {
		if (bigEndian()) {
			addBe(a, count);
		} else {
			addLe(a, count);
		}
	}
	/**
	 * Adds an array of integers to the end of an output part. The endianess is
	 * chosen based on the part's MpfBigendian flag.
	 * @tparam Int  The integer type.
	 * @tparam N    The length of the array.
	 * @param  a    The array to add to the end of the part.
	 * @throw  ConversationBadAdd  The part is flagged for input.
	 */
	template <typename Int, std::size_t N>
	void add(const Int (&a)[N]) {
		if (bigEndian()) {
			addBe(a, N);
		} else {
			addLe(a, N);
		}
	}
	/**
	 * Sets a new start offset, in bytes, for a variable length input part.
	 * The pointer returned by start() will be @a offset bytes after the
	 * begining of the internal vector.
	 * @pre   The size of the internal vector is larger than @a offset bytes.
	 * @pre   The part is flagged for a varying length.
	 * @param offset  The number of bytes to reserve at the begining of the
	 *                vector. It must be larger than the current size of the
	 *                vector, and must not be negative.
	 * @throw ConversationBadOffset    The offset value was invalid.
	 * @throw ConversationFixedLength  This conversation part is not flagged
	 *                                 as having a varying length.
	 */
	void setStartOffset(std::int16_t offset);
	/**
	 * Sets a new length, including the data prior to the start pointer, for a
	 * conversation part with a varying length.
	 * @pre   The part is flagged for a varying length.
	 * @param len      The new length for the part. The offset value is not
	 *                 used in this operation.
	 * @throw ConversationFixedLength  This conversation part is not flagged
	 *                                 as having a varying length.
	 */
	void setLength(std::size_t len);
	/**
	 * Reserves space in the internal vector. This can be used regardless of
	 * the part's flags, but makes much more sense for output parts.
	 */
	void reserve(std::size_t len);
	/**
	 * Use with the insertion operator to specify big-endian data will follow.
	 */
	struct BigEndian { };
	/**
	 * Use with the insertion operator to specify little-endian data will follow.
	 */
	struct LittleEndian { };
	/**
	 * Use with the insertion operator to reserve space in the vector.
	 */
	struct Reserve {
		int len;
		/**
		 * Specify how many bytes to reserve.
		 * @param l  Length to reserve.
		 */
		Reserve(int l) : len(l) { }
	};
	virtual char *start() const;
	virtual std::size_t length() const;
};

/**
 * Insertion operator to add an integer to a ConversationVector object.
 * The endianess is specified by the part's MpfBigendian flag.
 * @pre    The conversation part is flagged for output.
 * @tparam Int  The integer type.
 * @param  cv   The destination conversation part.
 * @param  i    The integer to add to the end of @a cv.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
template <typename Int>
ConversationVector &operator << (ConversationVector &cv, const Int &i) {
	cv.add(i);
	return cv;
}

/**
 * Insertion operator to add an array of integers to a ConversationVector
 * object. The endianess is specified by the part's MpfBigendian flag.
 * @pre    The conversation part is flagged for output.
 * @tparam Int  The integer type.
 * @tparam N    The length of the array.
 * @param  cv   The destination conversation part.
 * @param  a    The array of integers to add to the end of @a cv.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
template <typename Int, std::size_t N>
ConversationVector &operator << (ConversationVector &cv, const Int (&a)[N]) {
	cv.add(a, N);
	return cv;
}

/**
 * Insertion operator to reserve space in a ConversationVector.
 * @param cv    The conversation part to modify.
 * @param cvr   The object that specifies how much space to reserve.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
inline ConversationVector &operator << (
	ConversationVector &cv,
	const ConversationVector::Reserve &cvr
) {
	cv.reserve(cvr.len);
	return cv;
}

/**
 * Insertion operator to make the ConversationVector handle any following adds
 * using big-endian format.
 * @param cv    The conversation part to modify.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
inline ConversationVector &operator << (
	ConversationVector &cv,
	const ConversationVector::BigEndian
) {
	cv.bigEndian(true);
	return cv;
}

/**
 * Insertion operator to make the ConversationVector handle any following adds
 * using little-endian format.
 * @param cv    The conversation part to modify.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
inline ConversationVector &operator << (
	ConversationVector &cv,
	const ConversationVector::LittleEndian
) {
	cv.bigEndian(false);
	return cv;
}

/**
 * Insertion operator to add a string as binary data to the end of an output
 * ConversationVector.
 * @param cv    The conversation part to modify.
 * @param str   The string to add.
 * @return      A reference to @a cv.
 * @relatesalso ConversationVector
 */
inline ConversationVector &operator << (
	ConversationVector &cv,
	const std::string &str
) {
	cv.add(str);
	return cv;
}

} } }

#endif        //  #ifndef CONVERSATIONVECTOR_HPP
