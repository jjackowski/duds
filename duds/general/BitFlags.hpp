/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef BITFLAGS_HPP
#define BITFLAGS_HPP

namespace duds { namespace general {

/**
 * A type-safe bit flag storage class.
 *
 * This template stores bit flags in a data type upon which regular bitwise
 * operations function, such as integers. The size of the class is the same
 * as the underlying storage type (@a BitsType). When compiled by gcc using
 * "-O1", using this class with type @a int generates the same instructions
 * for the same operations as code using an @a int directly.
 *
 * This class defines the full set of bitwise operators to work with objects
 * all of this type. The operators intentionally do not work with other types,
 * including BitFlags types with different template parameters. The equality
 * and inequality operators also work when the second operand is a @a BitsType.
 * Objects of this class can be implicitly converted to @a BitsType, but may
 * not be assigned a value directly from a @a BitsType. Bit shift and other
 * arithmetic operators are not defined because they don't make a lot of sense
 * in the context of bit flags.
 *
 * For each set of interesting bit flags intended for a particular purpose,
 * a typedef should put a name to the template with a unique tag type. The
 * tag type need not have any definition or other mention in the code outside
 * of this typedef.
 * @code
 * typedef BitFlags<struct tag_one>  CapabilityFlags;
 * @endcode
 * A particular flag or set of flags can be given a name by defining a constant
 * value:
 * @code
 * // a non-member flag
 * constexpr CapabilityFlags CapabilitySwitch = CapabilityFlags(2);
 * // a class member flag
 * class Class {
 * 	public:
 * 	static constexpr CapabilityFlags MemberSwitch = CapabilityFlags(4);
 * 	// equivalent to above
 * 	static constexpr CapabilityFlags MemberSwitch = CapabilityFlags::Bit(2);
 * };
 * @endcode
 * Flags with multiple bits may also be defined:
 * @code
 * constexpr CapabilityFlags CapabilityMask = CapabilityFlags(15);
 * @endcode
 * Flag combinations using already defined constants can also be made:
 * @code
 * static constexpr CapabilityFlags CombinedSwitches =
 * 			CapabilitySwitch | Class::MemberSwitch;
 * @endcode
 * Bit flag values with different BitFlags types may not be implicity mixed.
 * @code
 * typedef BitFlags<struct tag_two>  OperationFlags;
 * // compile-time error on the next line
 * constexpr OperationFlags WontWork = CapabilitySwitch;
 * // works; it must clearly explicitly be converted
 * constexpr OperationFlags WillWork = OperationFlags(CapabilitySwitch.flags());
 * @endcode
 *
 * As a general rule, the names used for flag values should be for the
 * true/set state of the flag(s). For instance, "lightOn", or "buttonUp",
 * but not "lightOnOff", or "button". This will make your code more
 * [self explanitory](http://jjackowski.wordpress.com/2012/12/10/programming-mistakes-the-onoff-flag/).
 *
 * @tparam Tag        A tag type, normally given as a struct that isn't used
 *                    elsewhere. This provides type safety; other expansions of
 *                    this template will not work with each other if they have
 *                    different tag types, even if the underlying storage type
 *                    is the same.
 * @tparam BitsType   The underlying storage type. This should normally be
 *                    an integer that is a fundamental data type. It must be a
 *                    type that can be set from a constructor declared as
 *                    constexpr. If signed, the value -1 can be used to check
 *                    for all bits set. Other than that, signed and unsigned
 *                    types work the same, although mismatching them will
 *                    produce warnings with the exception of -1 literals.
 *
 * @todo  Write something that is not part of this class that can serialize the
 *        flags in a human readable manner with names for the flags that does
 *        not impose a particular order on the flags for input.
 * @todo  Write support for stream I/O, or at least stream output, with human
 *        readable flags. Even better if it has I18N support, at least for
 *        output.
 *
 * @author  Jeff Jackowski
 */
template <class Tag, class BitsType = int>
class BitFlags {
	/**
	 * The flags.
	 */
	BitsType bits;
	public:
	/**
	 * The datatype used to store the flags.
	 * @todo  I do not like the case of the typedef, but do like the case of
	 *        the template parameter. Such a bother.
	 */
	typedef BitsType bitsType;
	/**
	 * Construct an uninitialized bit flags container.
	 */
	BitFlags() = default;
	/**
	 * Construct a bit flags container with a specified value.
	 * @param b  The initial value. The type is the storage type @a BitsType.
	 */
	constexpr BitFlags(const BitsType &b) : bits(b) { }
	/**
	 * Construct a bit flags container with a copy of the contents of another
	 * container of the same type.
	 * @param f  The BitFlags object to copy.
	 */
	constexpr BitFlags(const BitFlags &f) : bits(f.bits) { }
	/**
	 * Makes a bit flags container with all flags cleared. Most useful when an
	 * implicit type conversion from @a BitsType to @a BitFlags is not allowed,
	 * but the type needs to be @a BitFlags.
	 */
	static constexpr BitFlags Zero() {
		return BitFlags((BitsType)0);
	}
	/**
	 * Makes a bit flags container with a single bit set that is specified
	 * by digit number rather than value.
	 * @param b  The bit number. 0 is the least significant bit.
	 */
	static constexpr BitFlags Bit(BitsType b) {
		return BitFlags(((BitsType)1) << b);
	}
	/**
	 * Returns the value stored in the object.
	 */
	constexpr BitsType flags() const {
		return bits;
	}
	/**
	 * Regular bitwise or.
	 */
	constexpr BitFlags operator | (const BitFlags &bf) const {
		return bits | bf.bits;
	}
	/**
	 * Regular bitwise or assignment.
	 */
	BitFlags operator |= (const BitFlags &bf) {
		return bits |= bf.bits;
	}
	/**
	 * Regular bitwise and.
	 */
	constexpr BitFlags operator & (const BitFlags &bf) const {
		return bits & bf.bits;
	}
	/**
	 * Regular bitwise and assignment.
	 */
	BitFlags operator &= (const BitFlags &bf) {
		return bits &= bf.bits;
	}
	/**
	 * Regular bitwise exclusive or.
	 */
	constexpr BitFlags operator ^ (const BitFlags &bf) const {
		return bits ^ bf.bits;
	}
	/**
	 * Regular bitwise exclusive or assignment.
	 */
	BitFlags operator ^= (const BitFlags &bf) {
		return bits ^= bf.bits;
	}
	/**
	 * Regular equality operator.
	 */
	constexpr bool operator == (const BitFlags &bf) const {
		return bits == bf.bits;
	}
	/**
	 * Regular not equivalent operator.
	 */
	constexpr bool operator != (const BitFlags &bf) const {
		return bits != bf.bits;
	}
	/**
	 * Regular equality operator.
	 */
	constexpr bool operator == (const BitsType &b) const {
		return bits == b;
	}
	/**
	 * Regular not equivalent operator.
	 */
	constexpr bool operator != (const BitsType &b) const {
		return bits != b;
	}
	/**
	 * Regular bitwise not.
	 */
	constexpr BitFlags operator ~ () const {
		return ~bits;
	}
	/**
	 * Regular logical not.
	 */
	constexpr bool operator ! () const {
		return !bits;
	}
	/**
	 * Evaluate as a boolean.
	 */
	constexpr operator bool () const {
		return bits != 0;
	}
	/**
	 * Regular assignment.
	 */
	BitFlags operator = (const BitFlags &bf) {
		return bits = bf.bits;
	}
	/**
	 * A bit mask operation; bits set in @a mask will remain unchanged in this
	 * object, while all other bits will be cleared.
	 * @param mask  The mask to apply by a bitwise and operation.
	 */
	BitFlags mask(const BitFlags &mask) {
		return bits &= mask.bits;
	}
	/**
	 * Clear all bits.
	 */
	BitFlags clear() {
		return bits = 0;
	}
	/**
	 * Clear all bits in this object that are set in @a bf.
	 * @param bf  The flags to clear.
	 */
	BitFlags clear(const BitFlags &bf) {
		return bits &= ~bf.bits;
	}
	/**
	 * Set all bits in this object that are set in @a bf.
	 * @param bf  The flags to set.
	 */
	BitFlags set(const BitFlags &bf) {
		return bits |= bf.bits;
	}
	/**
	 * Make all bits in @a bf set or clear based on the value of @a val.
	 * @param bf   The flags to change.
	 * @param val  The value to use for the bit to change.
	 */
	BitFlags setTo(const BitFlags &bf, bool val) {
		if (val) {
			return bits |= bf.bits;
		}
		return bits &= ~bf.bits;
	}
	/**
	 * Changes only the bits in a masked range.
	 * @param bf    The new values.
	 * @param mask  The mask of the bits to change.
	 */
	BitFlags setMasked(const BitFlags &bf, const BitFlags &mask) {
		return bits = (bits & (~mask.bits)) | (bf.bits & mask.bits);
	}
	/**
	 * Toggle the bits that are set in @a bf; all other bits remain unchanged.
	 * @param bf  The flags to toggle.
	 */
	BitFlags toggle(const BitFlags &bf) {
		return bits ^= bf.bits;
	}
	/**
	 * Returns true if the flags identified by @a mask have the same value as
	 * those flags do in @a value.
	 * @param value  The value to compare against this object.
	 * @param mask   Identifies the bit flags to test. Flags to test are set.
	 */
	constexpr bool test(const BitFlags &value, const BitFlags &mask) const {
		return (bits & mask.bits) == (value.bits & mask.bits);
	}
	/**
	 * Returns true if the flags identified by @a valuemask are all set.
	 * @param valuemask  The mask and value to compare against this object.
	 */
	constexpr bool test(const BitFlags &valuemask) const {
		return (bits & valuemask.bits) == valuemask.bits;
	}
};

} } // namespaces

#endif        //  #ifndef BITFLAGS_HPP
