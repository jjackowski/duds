/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef VALUE_HPP
#define VALUE_HPP

// Avoids dependency on the Boost type traits library. Not sure this is a good
// idea, but it must have made some sense back when I wrote this.
#define BOOST_UUID_NO_TYPE_TRAITS
#include <duds/data/QuantityArray.hpp>
#include <duds/general/LanguageTaggedString.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>
//#include <array>
#undef BOOST_UUID_NO_TYPE_TRAITS

namespace duds { namespace data {

// forward declaration to make the following typedef work
class GenericValueTable;

/**
 * A general value of a type can be serialized for transmission over a
 * network and can be used with a regular C++ I/O stream. This value can
 * be a map (a bunch of name-value pairs) to allow storing sub-values to
 * make heirarchical structures. It is 32 bytes on 32-bit and 64-bit targets.
 *
 * Allowed types in index order: <OL start="0">
 * <LI>String
 * <LI>A language tagged string
 * <LI>A map of language tagged strings
 * <!-- <LI>An array of 8 signed integers, 2 bytes each -->
 * <LI>An array of 4 signed integers, 4 bytes each
 * <LI>An array of 2 signed integers, 8 bytes each
 * <LI>A LargeIntWrapper holding a 16 byte signed integer (int128_w)
 * <LI>An array of 4 floating point numbers, single precision
 * <LI>An array of 2 floating point numbers, double precision
 * <LI>Double precision floating point number (direct numeric non-array
 *     assignments use this type, including integers)
 * <LI>A Quantity
 * <LI>A Quantity array with a dynamic size and number of dimensions
 * <!-- <LI>An ExtendedQuantity (good idea?) -->
 * <LI>A duration in femtoseconds
 * <LI>A duration in nanoseconds
 * <LI>A time with femtosecond precision
 * <LI>A time with nanosecond precision
 * <LI>UUID
 * <LI>A std::map with strings for keys and this variant type for values
 * <LI>Arbitrary data stored as a vector of char
 * </OL>
 * @todo  Maybe a map with UUID keys, too?
 *
 * A boolean value is not included because it causes the assignment of string
 * literals to assign boolean values.
 *
 * @par Integers
 * I chose to include a 16 byte integer as one of the types since it is the
 * same size as a UUID, which is also included, so it will fit. Because gcc
 * has no 128-bit integer support for 32-bit targets, and because this variant
 * type needs to be serializable for use between 32-bit and 64-bit hosts, the
 * 128-bit integer is wrapped up in an int128_w. This prevents easy use with
 * an int128_t, but attempting to allow such use results in doubles being
 * stored as integers.
 * @note   Assigning an integer will cause the value to be stored as a double.
 *         Making the variant act this way prevents it from implicitly
 *         converting floating point values to integers for storage, which I
 *         think is much worse. This is the benifit of the not-so-easy to use
 *         int128_w type: it cannot be used directly with other interger
 *         types, so doubles remain doubles.
 *
 * boost::recursive_wrapper is used for a couple of the variant's types to
 * store them dynamically instead of inside the memory of the variant. In the
 * case of duds::general::GenericValueTable, this prevents the variant
 * from being even larger.
 *
 * @author  Jeff Jackowski
 */
typedef boost::variant<
	// strings
	std::string,
	duds::general::LanguageTaggedString,
	// integers
	//std::array<std::int16_t, 8>,  // limited usefulness
	std::array<std::int32_t, 4>,
	std::array<std::int64_t, 2>,
	duds::data::int128_w, // no implicit conversions; doubles do not become ints
	// floating point
	std::array<float, 4>,
	std::array<double, 2>,    // maybe remove
	double,          // numeric assignments go here, even integers
	duds::data::Quantity,
	boost::recursive_wrapper<duds::data::QuantityNddArray>,
	//duds::ExtendedQuantity<double>,
	// time
	duds::time::interstellar::Femtoseconds,
	duds::time::interstellar::Nanoseconds,
	duds::time::interstellar::FemtoTime,
	duds::time::interstellar::NanoTime,
	// identification
	boost::uuids::uuid,
	// complex hierarchical data
	boost::recursive_wrapper<duds::data::GenericValueTable>,
	//std::shared_ptr<GenericValueTable>,

	// maybe a table with UUID keys?

	// arbitrary data; remove?
	std::shared_ptr< std::vector<char> >

> GenericValue;

typedef std::map<std::string, GenericValue>  generic_value_table_base_t;

/**
 * A set of key-value pairs where the value can be one of several types.
 * This is intended for storing arbitrary data that can be readily serialized
 * for storage or network transmission.
 *
 * This isn't a typedef to help make the @ref GenericValue typedef compile.
 * @author  Jeff Jackowski
 */
class GenericValueTable : public generic_value_table_base_t {
	friend class boost::serialization::access;
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_BASE_OBJECT_NVP(generic_value_table_base_t);
	}
};

} }

#endif        //  #ifndef VALUE_HPP
