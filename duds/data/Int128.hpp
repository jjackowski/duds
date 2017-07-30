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
 * 128-bit integer support.
 * I attempted to use some fancy template stuff from C++11, but it all failed.
 * It could make for a simpiler implementation, so much of the attempt is still
 * here and commeneted out.
 */
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <duds/BuildConfig.h>
#include <cstdint>
#include <sstream>

/*
#include <type_traits>
namespace std {
	template <>
	struct is_integral<__int128> : public true_type {
		//static constexpr bool value = true;
	};
	template <>
	struct is_integral<unsigned __int128> : public true_type {
		//static constexpr bool value = true;
	};
	template <>
	struct is_integral<boost::multiprecision::int128_t> : public true_type {
		//static constexpr bool value = true;
	};
	template <>
	struct is_integral<boost::multiprecision::uint128_t> : public true_type {
		//static constexpr bool value = true;
	};
}
*/

namespace duds { namespace data {

/**
 * The type used for 128-bit integers. The type varies: gcc will use __int128
 * if available (currently only 64-bit targets), otherwise Boost's 128-bit
 * fixed size integer from the multiprecision library is used. It seems
 * Boost added this in version 1.53, and several bug fixes were added up to
 * the most current version when this text was writen, 1.55.
 *
 * @note     This type should not be used for serialization; instead use
 *           int128_w. This type is intended for internal storage and usage
 *           that does not escape the process.
 */
#ifdef HAVE_INT128
typedef __int128  int128_t;

std::istream &operator >> (std::istream &is, int128_t &b);
std::ostream &operator << (std::ostream &os, int128_t const &b);

#else
typedef boost::multiprecision::int128_t  int128_t;
#endif

/**
 * Wraps an integer to allow the Boost multiprecision library to provide
 * insertion and extraction operators, and to provide Boost serialization
 * support that will produce the same result, while using either a native
 * integer type or a Boost multiprecision type. This is intended to allow
 * the use of gcc's __int128 type on 64-bit targets and
 * boost::multiprecision::int128_t on 32-bit targets, while allowing the
 * integer in serialized form to be exchanged between the targets. For larger
 * integers, it makes more sense to use something like Boost multiprecision
 * on all targets.
 * @tparam  I  The underlying integer type used for storage.
 * @tparam  M  The Boost multiprecision type used for stream I/O. This
 *             parameter may be the same as @a I.
 * @author  Jeff Jackowski
 */
template <class I, class M>
struct LargeIntWrapper {
	/*
private:
	LargeIntWrapper(const double &v);
	LargeIntWrapper &operator = (double const &b);
	*/
public:
	I value;
	LargeIntWrapper() = default;
	LargeIntWrapper(const LargeIntWrapper &liw) = default;
	//template <class N, typename std::enable_if<std::is_integral<N>::value>::type >
	//constexpr LargeIntWrapper(const N &v) : value(v) { }
	constexpr LargeIntWrapper(const I &v) : value(v) { }
	/*
	operator I const & () const {
		return value;
	}
	*/
	LargeIntWrapper &operator = (LargeIntWrapper const &b) {
		value = b.value;
		return *this;
	}
	/*
	template <class N, typename std::enable_if<std::is_integral<N>::value>::type >
	LargeIntWrapper &operator = (N const &b) {
		value = b;
		return *this;
	}
	*/
	//template <>
	LargeIntWrapper &operator = (I const &b) {
		value = b;
		return *this;
	}
	bool operator == (LargeIntWrapper const &b) const {
		return value == b.value;
	}
	bool operator != (LargeIntWrapper const &b) const {
		return value != b.value;
	}
	bool operator > (LargeIntWrapper const &b) const {
		return value > b.value;
	}
	bool operator < (LargeIntWrapper const &b) const {
		return value < b.value;
	}
	bool operator >= (LargeIntWrapper const &b) const {
		return value >= b.value;
	}
	bool operator <= (LargeIntWrapper const &b) const {
		return value <= b.value;
	}
	LargeIntWrapper operator + (LargeIntWrapper const &b) const {
		return value + b.value;
	}
	LargeIntWrapper operator - (LargeIntWrapper const &b) const {
		return value - b.value;
	}
	LargeIntWrapper operator * (LargeIntWrapper const &b) const {
		return value * b.value;
	}
	LargeIntWrapper operator / (LargeIntWrapper const &b) const {
		return value / b.value;
	}
	LargeIntWrapper operator % (LargeIntWrapper const &b) const {
		return value % b.value;
	}
	LargeIntWrapper &operator += (LargeIntWrapper const &b) {
		return value += b.value;
	}
	LargeIntWrapper &operator -= (LargeIntWrapper const &b) {
		return value -= b.value;
	}
	LargeIntWrapper &operator *= (LargeIntWrapper const &b) {
		return value *= b.value;
	}
	LargeIntWrapper &operator /= (LargeIntWrapper const &b) {
		return value /= b.value;
	}
	LargeIntWrapper &operator %= (LargeIntWrapper const &b) {
		return value %= b.value;
	}
	friend std::istream &operator >> (std::istream &is, LargeIntWrapper &b) {
		return is >> b.value;
	}
	friend std::ostream &operator << (std::ostream &os, LargeIntWrapper const &b) {
		return os << b.value;
	}
private:
	/**
	 * Serialization support.
	 */
	friend class boost::serialization::access;
	/**
	 * Serialize the unit data.
	 * @warning  The archive (type @a A) must handle endianness. Some, like
	 *           text formats, do. Not sure about others.
	 */
	template <class A>
	void save(A &a, const unsigned int) const {
		std::ostringstream os;
		os << std::hex << *this;
		std::string s(os.str());
		a << boost::serialization::make_nvp("value", s);
	}
	template <class A>
	void load(A &a, const unsigned int) {
		std::string s;
		a >> boost::serialization::make_nvp("value", s);
		std::istringstream is(s);
		is >> std::hex >> *this;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER();
};


/**
 * A 128-bit integer wrapped by LargeIntWrapper to make the Boost serialized
 * result interchangable between 32 and 64-bit targets.
 */
typedef LargeIntWrapper<int128_t, boost::multiprecision::int128_t>  int128_w;

} }
