/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * Header for ConversationExtractor; includes all other conversation related
 * header files.
 */
#include <duds/hardware/interface/Conversation.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * An attempt was made to extract data past the end of a conversation or a
 * conversation part.
 */
struct ConversationReadPastEnd : ConversationError { };

/**
 * A ConversationExtractor was asked to operate on a Conversation, but one
 * is not set. This can happen if the default constructor is used and the
 * ConversationExtractor::reset(const Conversation &) is not called.
 */
struct ConversationNotSet : ConversationError { };

/**
 * Extracts data from a Conversation without modifying the Conversation or
 * copying from it.
 *
 * All conversation parts flagged as extractible
 * (see ConversationPart::MpfExtract) are visited in order. The conversation
 * data is extracted directly from the ConversationPart objects in the
 * originating Conversation object.
 *
 * @warning  The Conversation object that holds the data to extract must not
 *           be modified or deallocated while an extractor object exists and
 *           is still being used for extracting the conversation. An extractor
 *           can be deallocated after its associated Conversation.
 *
 * @author  Jeff Jackowski
 */
class ConversationExtractor {
	/**
	 * Iterator to the current part being read.
	 */
	Conversation::PartVector::const_iterator piter;
	/**
	 * The Conversation object with the parts to read.
	 */
	const Conversation *c;
	/**
	 * Pointer to the next byte to read.
	 */
	const char *pos;
	/**
	 * The remaining number of bytes to read in the part referenced by @a piter.
	 */
	std::size_t remain;
	/**
	 * Sets internal data to read from the begining of extractible data stored
	 * in Conversation @a c.
	 */
	void set();
public:
	/**
	 * Constructs with nothing to extract. Use reset(const Conversation &) to
	 * to extract from a Conversation.
	 * @post  end() returns false, and remaining() returns zero.
	 */
	ConversationExtractor() : c(nullptr), pos(nullptr), remain(0) { }
	/**
	 * Constructs to extract from the given Conversation.
	 * @param con  The Conversation to read.
	 */
	ConversationExtractor(const Conversation &con) : c(&con) {
		set();
	};
	/**
	 * Must use functions that specifiy either big or little endian.
	 */
	ConversationExtractor(const ConversationPart &cp) :
	c(nullptr), pos(cp.start()), remain(cp.length()) { }
	/**
	 * Prepares the object to extract another time from the same Conversation
	 * object used previously. This may be called before extracting all data
	 * from the conversation. It allows the same data to be re-extracted, or
	 * to extract new data if the Conversation was reused since the last
	 * extraction.
	 * @post  The given Conversation must not change before this object is
	 *        used to extract all the data that will be extracted.
	 * @throw ConversationNotSet  A Conversation was never set. This can happen
	 *                            if the default constructor was used.
	 */
	void reset() {
		set();
	};
	/**
	 * Prepares the object to extract from the given Conversation. This may
	 * be called before extracting all data from a previous conversation, or
	 * after the previous conversation no longer exists. It allows a single
	 * ConversationExtractor object to be used multiple times, and it can make
	 * use of an extractor made using the default constructor.
	 * @post  The given Conversation must not change before this object is
	 *        used to extract all the data that will be extracted.
	 * @param con  The conversation to extract.
	 */
	void reset(const Conversation &con) {
		c = &con;
		set();
	};
	/**
	 * Must use functions that specify either big or little endian.
	 */
	void reset(const ConversationPart &cp) {
		c = nullptr;
		pos = cp.start();
		remain = cp.length();
	}
	/**
	 * Returns true when all the extractible conversation data has been
	 * extracted.
	 */
	bool end() const {
		return !pos;
	}
	/**
	 * Returns the number of bytes remaining in the current conversation
	 * part. A conversation may have multiple extractible parts, so this
	 * value can be different from the total extractible bytes across all
	 * parts.
	 */
	std::size_t remaining() const {
		return remain;
	}
	/**
	 * Advance the given number of bytes within the current conversation part.
	 * This can be used to skip data rather than read all extractible data.
	 * If the number of bytes to advance equals the number of bytes left in the
	 * part, it will advance to the begining of the next extractible part, or
	 * set the end conditions. It cannot be used in a single call to advance to
	 * the end of a part and past the begining of the next.
	 * @param bytes  The number of bytes to advance within the current part.
	 *               It must not be greater than the value returned by
	 *               remaining().
	 * @throw ConversationReadPastEnd  The requested number of bytes is greater
	 *                                 than the number left in the current part.
	 *                                 The read position is left unchanged.
	 */
	void advance(std::size_t bytes);
	/**
	 * Advances to the next extractible part. If there isn't another extractible
	 * part, the end conditions will be set and end() will return true.
	 * @throw ConversationReadPastEnd  The extractor is already at the end.
	 * @throw ConversationNotSet  A Conversation was never set. This can happen
	 *                            if the default constructor was used.
	 */
	void nextPart();
	/**
	 * Reads an integer in little-endian form.
	 * The integer may not span conversation parts; it must be contained in a
	 * single part.
	 * @pre    The remaining bytes to read is at least the size of @a Int.
	 * @post   Advances the number of bytes in @a Int by calling advance().
	 * @tparam Int  The integer type.
	 * @param  i    The integer that will receive the value.
	 * @throw  ConversationReadPastEnd  The integer type is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void readLe(Int &i) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be read by this function.");
		if (remain < sizeof(Int)) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		const char *p = pos;
		for (std::size_t loop = sizeof(Int); loop; --loop, ++p) {
			// use unsigned type for shift to LSb; signed gives different result
			i = (((typename std::make_unsigned<Int>::type)i >> 8) |
				(((Int)*p) << (8 * (sizeof(Int) - 1)))
			);
		}
		advance(sizeof(Int));
	}
	/**
	 * Reads an integer in big-endian form.
	 * The integer may not span conversation parts; it must be contained in a
	 * single part.
	 * @pre    The remaining bytes to read is at least the size of @a Int.
	 * @post   Advances the number of bytes in @a Int by calling advance().
	 * @tparam Int  The integer type.
	 * @param  i    The integer that will receive the value.
	 * @throw  ConversationReadPastEnd  The integer type is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void readBe(Int &i) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be read by this function.");
		if (remain < sizeof(Int)) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		const char *p = pos;
		for (std::size_t loop = sizeof(Int); loop; --loop, ++p) {
			i = (i << 8) | *p;
		}
		advance(sizeof(Int));
	}
	/**
	 * Reads an integer in the endianess flagged in the conversation part.
	 * The integer may not span conversation parts; it must be contained in a
	 * single part.
	 * @pre    The remaining bytes to read is at least the size of @a Int.
	 * @post   Advances the number of bytes in @a Int by calling advance().
	 * @tparam Int  The integer type.
	 * @param  i    The integer that will receive the value.
	 * @throw  ConversationReadPastEnd  The integer type is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void read(Int &i) {
		// if at end of all parts, the iterator is c->cend() and pos is null
		if (!pos) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		if ((*piter)->bigEndian()) {
			readBe(i);
		} else {
			readLe(i);
		}
	}
	/**
	 * Reads an array of integers in little-endian form.
	 * The integer array may not span conversation parts; it must be contained
	 * in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @param  a      The start of the destination integer array.
	 * @param  count  The number of elements in the array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void readLe(Int *a, std::size_t count) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be read by this function.");
		int len = (int)sizeof(Int) * count;
		if (remain < len) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		const char *p = pos;
		for (; count; --count, ++a) {
			for (std::size_t loop = sizeof(Int); loop; --loop, ++p) {
				// use unsigned type for shift to LSb
				*a = ((((typename std::make_unsigned<Int>::type)*a) >> 8) |
					(((Int)*p) << (8 * (sizeof(Int) - 1)))
				);
			}
		}
		advance(len);
	}
	/**
	 * Reads an array of integers in big-endian form.
	 * The integer array may not span conversation parts; it must be contained
	 * in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @param  a      The start of the destination integer array.
	 * @param  count  The number of elements in the array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void readBe(Int *a, std::size_t count) {
		static_assert(std::is_integral<Int>::value,
			"Only integer values can be read by this function.");
		int len = (int)sizeof(Int) * count;
		if (remain < len) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		const char *p = pos;
		for (; count; --count, ++a) {
			for (std::size_t loop = sizeof(Int); loop; --loop, ++p) {
				*a = (*a << 8) | *p;
			}
		}
		advance(len);
	}
	/**
	 * Reads an array of integers in little-endian form.
	 * The integer array may not span conversation parts; it must be contained
	 * in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @tparam N      The number of elements in the array.
	 * @param  a      The start of the destination integer array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int, std::size_t N>
	void readLe(Int (&a)[N]) {
		readLe(a, N);
	}
	/**
	 * Reads an array of integers in big-endian form.
	 * The integer array may not span conversation parts; it must be contained
	 * in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @tparam N      The number of elements in the array.
	 * @param  a      The start of the destination integer array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int, std::size_t N>
	void readBe(Int (&a)[N]) {
		readBe(a, N);
	}
	/**
	 * Reads an array of integers in the endianess flagged in the conversation
	 * part. The integer array may not span conversation parts; it must be
	 * contained in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @param  a      The start of the destination integer array.
	 * @param  count  The number of elements in the array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int>
	void read(Int *a, std::size_t count) {
		// if at end of all parts, the iterator is c->cend() and pos is null
		if (!pos) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		if ((*piter)->bigEndian()) {
			readBe(a, count);
		} else {
			readLe(a, count);
		}
	}
	/**
	 * Reads an array of integers in the endianess flagged in the conversation
	 * part. The integer array may not span conversation parts; it must be
	 * contained in a single part.
	 * @pre    The remaining bytes to read is at least the size of the array.
	 * @post   Advances the number of bytes in the array by calling advance().
	 * @tparam Int    The integer type.
	 * @tparam N      The number of elements in the array.
	 * @param  a      The start of the destination integer array.
	 * @throw  ConversationReadPastEnd  The array is too large to fit in
	 *                                  the remaining bytes of the current
	 *                                  part. The read position is left
	 *                                  unchanged.
	 */
	template <typename Int, std::size_t N>
	void read(Int (&a)[N]) {
		// if at end of all parts, the iterator is c->cend() and pos is null
		if (!pos) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		if ((*piter)->bigEndian()) {
			readBe(a, N);
		} else {
			readLe(a, N);
		}
	}
	/**
	 * Reads by using std::memcpy() to copy data.
	 * The data may not span conversation parts; it must be contained in a
	 * single part.
	 * @pre    The remaining bytes to read is at least @a len.
	 * @post   Advances @a len bytes by calling advance().
	 * @param  dest   The start of the destination buffer.
	 * @param  len    The number of bytes to copy.
	 * @throw  ConversationReadPastEnd  Fewer than @a len bytes remain in
	 *                                  the current part. The read position is
	 *                                  left unchanged.
	 */
	void read(char *dest, std::size_t len);
	/**
	 * Reads by inserting bytes into a container.
	 * The data may not span conversation parts; it must be contained in a
	 * single part.
	 * @pre    The remaining bytes to read is at least @a len.
	 * @post   Advances @a len bytes by calling advance().
	 * @tparam Cont   The destination container type.
	 * @tparam Iter   The iterator type of the container.
	 * @param  cont   The destination container.
	 * @param  start  The insertion position for @a cont.
	 * @param  len    The number of bytes to insert into @a cont.
	 * @throw  ConversationReadPastEnd  Fewer than @a len bytes remain in
	 *                                  the current part. The read position is
	 *                                  left unchanged.
	 */
	template <class Cont, typename Iter>
	void read(Cont &cont, Iter &start, std::size_t len) {
		if (remain < len) {
			BOOST_THROW_EXCEPTION(ConversationReadPastEnd());
		}
		cont.insert(start, pos, pos + len);
		advance(len);
	}
};

/**
 * Extraction operator to read an integer from a Conversation through a
 * ConversationExtractor object.
 * The endianess is specified by the part's MpfBigendian flag.
 * @pre    The remaining bytes to read is at least the size of @a Int.
 * @tparam Int  The integer type.
 * @param  ce   The source ConversationExtractor object.
 * @param  i    The integer to that will receive the extracted value.
 * @throw  ConversationReadPastEnd  The integer type is too large to fit in
 *                                  the remaining bytes of the current
 *                                  part. The read position is left
 *                                  unchanged.
 * @return      A reference to @a ce.
 * @relatesalso ConversationExtractor
 */
template <typename Int>
ConversationExtractor &operator >> (ConversationExtractor &ce, Int &i) {
	ce.read(i);
	return ce;
}

/**
 * Extraction operator to read an array of integers from a Conversation through
 * a ConversationExtractor object.
 * The endianess is specified by the part's MpfBigendian flag.
 * @pre    The remaining bytes to read is at least the size of the array.
 * @tparam Int  The integer type.
 * @tparam N    The number of elements in the array.
 * @param  ce   The source ConversationExtractor object.
 * @param  a    The start of the destination integer array.
 * @throw  ConversationReadPastEnd  The integer type is too large to fit in
 *                                  the remaining bytes of the current
 *                                  part. The read position is left
 *                                  unchanged.
 * @return      A reference to @a ce.
 * @relatesalso ConversationExtractor
 */
template <typename Int, std::size_t N>
ConversationExtractor &operator >> (ConversationExtractor &ce, Int (&a)[N]) {
	ce.read(a, N);
	return ce;
}

} } }
