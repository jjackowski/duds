/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef UNIT_HPP
#define UNIT_HPP

#include <boost/serialization/nvp.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <duds/general/SignExtend.hpp>
#include <stdexcept>

namespace duds { namespace data {

using duds::general::SignExtend;

/**
 * A general unit related error. Only errors derived from this one are ever
 * thrown.
 */
struct UnitError : virtual std::exception, virtual boost::exception { };

/**
 * Indicates that a value is beyond the range allowed by the Unit or
 * ExtendedUnit class. Only Unit functions starting with "set" will throw
 * this exception.
 */
struct UnitRangeError : UnitError { };

/**
 * The out-of-range exponent.
 */
typedef boost::error_info<struct tag_unitexp, int>  BadUnitExponent;

/**
 * The name of the bad unit.
 */
typedef boost::error_info<struct tag_unitname, std::string>  BadUnit;

/**
 * Indicates that two different Unit objects were used in an operation that
 * requires identical Unit objects.
 */
struct UnitMismatch : UnitError { };

/**
 * A conversion between units was attempted that cannot be performed.
 * This is used when going between a value of units determined by a Unit,
 * such as Quantity, and another data type. Time types, like
 * std::chrono::duration, specify their units in the type. A conversion
 * to a duration type from a value with a Unit must fail if the Unit is not
 * duds::data::units::Second. The failure should throw this exception.
 */
struct UnitBadConversion : UnitError { };

/**
 * Represents an SI unit, either base or derived.
 *
 * @todo  Rename to SiUnit? Not sure.
 *
 * Each base SI unit has a nibble sized field inside a 32-bit integer. There
 * are an additional two bits each for radians and steradians. Each
 * of these fields is a signed integer that is the exponent for the unit.
 * All the units are multiplied, with their exponent applied, to produce the
 * final unit.
 *
 * Functions named for the units are available to get the exponent for the
 * unit and to change the exponent. The functions that begin with "set" will
 * perform range checking, while the other functions will not. Without range
 * checking, the exponent to change will be incorrect if it is outside of the
 * allowable range (see Bit fields below), but all other exponents will always
 * remain unchanged.
 *
 * Multiply and divide operators are supplied for combining Unit objects.
 * These operators perform range checking and will throw if any exponent goes
 * beyond the allowable range.
 *
 * @par  Inconsistency with kilogram
 * In order to avoid having a bunch of unusual derived SI units, mass is in
 * kilograms instead of grams. As a result, an exponent of 1 for mass denotes
 * kilograms. This makes for a special case to avoid a user interface from
 * showing something like "millikilograms" when the scale is adjusted.
 *
 * @par  Time
 * Time is typically stored in types defined by C++ in the std::chrono
 * namespace. These types explicitly define the unit in use. If a Unit is
 * associated with one of these types, the Unit should indicate seconds
 * and the software should use the type for the actual units. While this is
 * a special case, time is already a big and annoying special case.
 *
 * @par  Bit fields
 * Access to the fields are coded explicitly to improve portability. The 32-bit
 * integer containing all the fields is stored in native endianness.
 * | Base unit | Byte | Bit range  | Exponent range |
 * | :-------- | :--: | :--------: | :------------: |
 * | Ampere    | 0    | 0-3        | -8 to 7        |
 * | Candela   | 0    | 4-7        | -8 to 7        |
 * | Kelvin    | 1    | 8-11, 0-3  | -8 to 7        |
 * | Kilogram  | 1    | 12-15, 4-7 | -8 to 7        |
 * | Meter     | 2    | 16-19. 0-3 | -8 to 7        |
 * | Mole      | 2    | 20-23, 4-7 | -8 to 7        |
 * | Second    | 3    | 24-27, 0-3 | -8 to 7        |
 * | Radian    | 3    | 28-29, 4-5 | -2 to 1        |
 * | Steradian | 3    | 30-31, 6-7 | -2 to 1        |
 *
 * Possible change to 64-bit integer (bad):
 * | %Unit     | Byte | Bit range | Exponent range | Notes
 * | :-------- | :--: | :-------: | :------------: | :-----------------------
 * | Ampere    | 0    | 0-4       | -16 to 15      | SI base unit
 * | Candela   | 0-1  | 5-9       | -16 to 15      | SI base unit
 * | Kelvin    | 1    | 10-14     | -16 to 15      | SI base unit
 * | Kilogram  | 1-2  | 15-19     | -16 to 15      | SI base unit
 * | Meter     | 2-3  | 20-24     | -16 to 15      | SI base unit
 * | Mole      | 3    | 25-29     | -16 to 15      | SI base unit
 * | Second    | 3-4  | 30-34     | -16 to 15      | SI base unit
 * | Radian    | 4    | 35-38     | -8 to 7        | Complements steradian
 * | Steradian | 4-5  | 39-42     | -8 to 7        | Needed for lumen
 * | Becquerel | 5    | 43-46     | -8 to 7        | Not the same as hertz
 * | Gray      | 5-6  | 47-50     | -8 to 7        | Not the same as sievert
 * | Sievert   | 6    | 51-54     | -8 to 7        | Not the same as gray
 * | Rel humid | 6-7  | 55-58     | -8 to 7        | Unitless if not included
 *
 * Another possible change (better):
 * | %Unit     | Byte | Bit range | Exponent range | Notes
 * | :-------- | :--: | :-------: | :------------: | :-----------------------
 * | Ampere    | 0    | 0-4       | -16 to 15      | SI base unit
 * | Candela   | 0-1  | 5-9       | -16 to 15      | SI base unit
 * | Kelvin    | 1    | 10-14     | -16 to 15      | SI base unit
 * | Kilogram  | 1-2  | 15-19     | -16 to 15      | SI base unit
 * | Meter     | 2-3  | 20-24     | -16 to 15      | SI base unit
 * | Mole      | 3    | 25-29     | -16 to 15      | SI base unit
 * | Second    | 3-4  | 30-34     | -16 to 15      | SI base unit
 * | Radian    | 4    | 35-38     | -8 to 7        | Complements steradian
 * | Steradian | 4-5  | 39-42     | -8 to 7        | Needed for lumen
 * | Property  | 5    | 43-46     | NA             | Arbitrary physical propery value
 * | Scale/off | 6-7  | 47-63     | NA             | Scale or offset?
 * The propery value is an arbitrary value that, when taken with the context
 * of the other values, indicates the physical property being measured. The
 * above suggestion allows for 16 different properties for each possible
 * combination of the base SI units.
 *
 * @bug  Some units, like Watts cubed, cannot be represented. Consider larger
 *       fields. Maybe use larger fields on another type used for intermediate
 *       results and unusual units and allow implicit conversions.
 *
 * @author  Jeff Jackowski
 */
class Unit {
	/**
	 * Stores the exponent fields.
	 */
	std::int32_t u;
	/**
	 * Serialization support.
	 */
	friend class boost::serialization::access;
	/**
	 * Serialize the unit data.
	 * @warning  The archive (type @a A) must handle endianness, or be endian
	 *           agnostic. Some do, like text formats. Not sure about others.
	 */
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(u);
	}
public:
	/**
	 * Returns the internal exponent fields.
	 */
	std::int32_t value() const {
		return u;
	}
	/**
	 * Construct the same as a std::int32_t; expect the value to be
	 * uninitalized.
	 */
	Unit() = default;
	/**
	 * Copy construct the same as a std::int32_t.
	 */
	Unit(const Unit &) = default;
	/**
	 * Make a new Unit set to a specific unit value; intended for making
	 * common unit constants with ::DUDS_UNIT_VALUE and initializing to zero
	 * for no unit or abstract unit.
	 * @warning  No range checking can be performed because this constructor
	 *           is declared constexpr. The advantage is that Unit objects for
	 *           any supported specific unit can also be declared constexpr.
	 */
	constexpr Unit(std::int32_t U) : u(U) { }
	/**
	 * Make a new Unit and set all the exponents to the given values with
	 * range checking.
	 * @param A    The exponent for amperes.
	 * @param cd   The exponent for candelas.
	 * @param K    The exponent for kelvin.
	 * @param kg   The exponent for kilograms.
	 * @param m    The exponent for meters.
	 * @param mol  The exponent for moles.
	 * @param s    The exponent for seconds.
	 * @param rad  The exponent for radians.
	 * @param sr   The exponent for steradians.
	 * @throw UnitRangeError  An exponent is out of the allowable range.
	 */
	Unit(int A, int cd, int K, int kg, int m, int mol, int s,
		int rad = 0, int sr = 0);
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int ampere() const {
		return SignExtend<4>(u & 0xF);
	}
	/**
	 * Returns the exponent for the candela (luminous intensity) dimension.
	 */
	int candela() const {
		return SignExtend<4>((u & 0xF0) >> 4);
	}
	/**
	 * Returns the exponent for the kelvin (thermodynamic temperature) dimension.
	 */
	int kelvin() const {
		return SignExtend<4>((u & 0xF00) >> 8);
	}
	/**
	 * Returns the exponent for the kilogram (mass) dimension.
	 */
	int kilogram() const {
		return SignExtend<4>((u & 0xF000) >> 12);
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int meter() const {
		return SignExtend<4>((u & 0xF0000) >> 16);
	}
	/**
	 * Returns the exponent for the mole (amount of substance) dimension.
	 */
	int mole() const {
		return SignExtend<4>((u & 0xF00000) >> 20);
	}
	/**
	 * Returns the exponent for the second (time) dimension.
	 */
	int second() const {
		return SignExtend<4>((u & 0xF000000) >> 24);
	}
	/**
	 * Returns the exponent for radians (angle).
	 */
	int radian() const {
		return SignExtend<2>((u & 0x30000000) >> 28);
	}
	/**
	 * Returns the exponent for steradians (solid angle).
	 */
	int steradian() const {
		return SignExtend<2>((u & 0xC0000000) >> 30);
	}
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int amp() const {
		return ampere();
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int metre() const {
		return meter();
	}
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int A() const {
		return ampere();
	}
	/**
	 * Returns the exponent for the candela (luminous intensity) dimension.
	 */
	int cd() const {
		return candela();
	}
	/**
	 * Returns the exponent for the kelvin (thermodynamic temperature) dimension.
	 */
	int K() const {
		return kelvin();
	}
	/**
	 * Returns the exponent for the kilogram (mass) dimension.
	 */
	int kg() const {
		return kilogram();
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int m() const {
		return meter();
	}
	/**
	 * Returns the exponent for the mole (amount of substance) dimension.
	 */
	int mol() const {
		return mole();
	}
	/**
	 * Returns the exponent for the second (time) dimension.
	 */
	int s() const {
		return second();
	}
	/**
	 * Returns the exponent for radians (angle).
	 */
	int rad() const {
		return radian();
	}
	/**
	 * Returns the exponent for steradians (solid angle).
	 */
	int sr() const {
		return steradian();
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void ampere(int e) {
		u = (u & 0xFFFFFFF0) | (e & 0xF);
	}
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void candela(int e) {
		u = (u & 0xFFFFFF0F) | ((e & 0xF) << 4);
	}
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension
	 * without range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kelvin(int e) {
		u = (u & 0xFFFFF0FF) | ((e & 0xF) << 8);
	}
	/**
	 * Sets the exponent for the ampere (current)kilogram (mass) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kilogram(int e) {
		u = (u & 0xFFFF0FFF) | ((e & 0xF) << 12);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void meter(int e) {
		u = (u & 0xFFF0FFFF) | ((e & 0xF) << 16);
	}
	/**
	 * Sets the exponent for the mole (amount of substance) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void mole(int e) {
		u = (u & 0xFF0FFFFF) | ((e & 0xF) << 20);
	}
	/**
	 * Sets the exponent for the second (time) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void second(int e) {
		u = (u & 0xF0FFFFFF) | ((e & 0xF) << 24);
	}
	/**
	 * Sets the exponent for radians (angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void radian(int e) {
		u = (u & 0xCFFFFFFF) | ((e & 0x3) << 28);
	}
	/**
	 * Sets the exponent for steradians (solid angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void steradian(int e) {
		u = (u & 0x3FFFFFFF) | ((e & 0x3) << 30);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void amp(int e) {
		ampere(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void metre(int e) {
		meter(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void A(int e) {
		ampere(e);
	}
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void cd(int e) {
		candela(e);
	}
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension
	 * without range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void K(int e) {
		kelvin(e);
	}
	/**
	 * Sets the exponent for the kilogram (mass) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kg(int e) {
		kilogram(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void m(int e) {
		meter(e);
	}
	/**
	 * Sets the exponent for the mole (amount of substance) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void mol(int e) {
		mole(e);
	}
	/**
	 * Sets the exponent for the second (time) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void s(int e) {
		second(e);
	}
	/**
	 * Sets the exponent for radians (angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void rad(int e) {
		radian(e);
	}
	/**
	 * Sets the exponent for steradians (solid angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void sr(int e) {
		steradian(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setAmpere(int e);
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setCandela(int e);
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setKelvin(int e);
	/**
	 * Sets the exponent for the kilogram (mass) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setKilogram(int e);
	/**
	 * Sets the exponent for the meter (distance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMeter(int e);
	/**
	 * Sets the exponent for the mole (amount of substance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMole(int e);
	/**
	 * Sets the exponent for the second (time) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setSecond(int e);
	/**
	 * Sets the exponent for radians (angle).
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setRadian(int e);
	/**
	 * Sets the exponent for steradians (solid angle).
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setSteradian(int e);
	/**
	 * Sets the exponent for the ampere (current) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setAmp(int e) {
		setAmpere(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMetre(int e) {
		setMeter(e);
	}
	/**
	 * Returns true if the Unit represents no units. This is normal for some
	 * values, such as ratios.
	 */
	bool unitless() const {
		return u == 0;
	}
	/**
	 * Makes the Unit unitless.
	 */
	void clear() {
		u = 0;
	}
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range.
	 */
	const Unit operator * (const Unit &U) const;
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range.
	 */
	const Unit operator / (const Unit &U) const;
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range. All
	 *                           work is done on a temporary so that if an
	 *                           exception is thrown only the temporary is
	 *                           modified.
	 */
	Unit &operator *= (const Unit &U);
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range. All
	 *                           work is done on a temporary so that if an
	 *                           exception is thrown only the temporary is
	 *                           modified.
	 */
	Unit &operator /= (const Unit &U);
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator < (const Unit &U) const {
		return u < U.u;
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator > (const Unit &U) const {
		return u > U.u;
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator <= (const Unit &U) const {
		return u <= U.u;
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator >= (const Unit &U) const {
		return u >= U.u;
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator == (const Unit &U) const {
		return u == U.u;
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator != (const Unit &U) const {
		return u != U.u;
	}
};

} }

/**
 * Creates the internal value used by duds::Unit in a way that allows the
 * compiler to generate a constant value. The Unit constructor that takes
 * the same set of parameters cannot be declared as constexpr, but using
 * this macro along with duds::Unit::Unit(std::int32_t) makes a constant
 * when all the macro's parameters are constants.
 * @param A    The exponent for amperes.
 * @param cd   The exponent for candelas.
 * @param K    The exponent for kelvin.
 * @param kg   The exponent for kilograms.
 * @param m    The exponent for meters.
 * @param mol  The exponent for moles.
 * @param s    The exponent for seconds.
 * @param rad  The exponent for radians.
 * @param sr   The exponent for steradians.
 */
#define DUDS_UNIT_VALUE(A, cd, K, kg, m, mol, s, rad, sr) \
	(((A) & 0xF) | (((cd) & 0xF) << 4) | (((K) & 0xF) << 8) | \
	(((kg) & 0xF) << 12) | (((m) & 0xF) << 16) | (((mol) & 0xF) << 20) | \
	(((s) & 0xF) << 24) | (((rad) & 3) << 28) | (((sr) & 3) << 30))

#endif        //  #ifndef UNIT_HPP
