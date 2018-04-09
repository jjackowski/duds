/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef EXTENDEDUNIT_HPP
#define EXTENDEDUNIT_HPP

#include <duds/general/Errors.hpp>
#include <duds/data/Unit.hpp>
#include <boost/serialization/split_member.hpp>

namespace duds { namespace data {

// needed for celsius, liter

/**
 * redo: Use floats for scale and offset. Use 32-bits for 10 3-bit and 1 2-bit
 *       additional unit flags (put relative humidity there?), and a Unit.
 *       Should be 16 bytes, 128-bits. Include in GenericValue.
 */
class ExtendedUnit {
	Unit unit;
	union {
		/**
		 * A scaling and offset value packed into four bytes. The scalar works
		 * for most of the metric prefixes. The offset works for 273.15. All this
		 * fits in the space of a single float. The offset needs more precision
		 * than a half (from IEEE 754).
		 */
		std::uint32_t scaloff;
		struct {
			/**
			 * The mantissa, save for the impled leading bit.
			 */
			unsigned int mant  : 20;
			/**
			 * The exponent as a signed integer rather than unsigned with bias.
			 */
			int exp            : 7;
			/**
			 * The sign flag.
			 */
			int sign           : 1;
			/**
			 * A value used to represnt most SI prefixes. The prefixes
			 * represent a scalar value. To produce this prefix scalar,
			 * multiply @a scale by three, then rasie ten to the power of
			 * the product. Kilograms has a prefix built into the base unit
			 * resulting in inconsistency. This scalar modifies kilo in
			 * kilograms. Thus, if @a unit is Kilogram, then a @a scale of -1
			 * produces the unit gram.
			 */
			int scale : 4;
		};
	};
	// serialization support
	friend class boost::serialization::access;
	template <class A>
	void save(A &a, const unsigned int) const {
		a & BOOST_SERIALIZATION_NVP(unit);
		// prevent differences in bitfields across platforms from causing
		// different values for scaloff; assume for now that the archive will
		// handle endianness differences
		std::uint32_t packed = mant | (exp << 20) | (sign << 27) | (scale << 28);
		a & BOOST_SERIALIZATION_NVP(packed);
	}
	template <class A>
	void load(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(unit);
		std::uint32_t packed;
		a & BOOST_SERIALIZATION_NVP(packed);
		// prevent differences in bitfields across platforms from causing
		// different values for scaloff; assume for now that the archive will
		// handle endianness differences
		mant = packed & 0xFFFFF;
		exp = (packed >> 20) & 0x7F;
		sign = (packed >> 27) & 1;
		scale = packed >> 28;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER();
	/**
	 * An simple internal constructor that takes the values of it members.
	 * @todo  Maybe make this public along with some macro to set @a s? Doing
	 *        so would allow easy creation of constant ExtendedUnit objects,
	 *        like the Units in Units.hpp. May have to specify the offset it
	 *        pieces, though.
	 */
	constexpr ExtendedUnit(Unit u, std::uint32_t s) : unit(u), scaloff(s) { }
public:
	/**
	 * Makes an uninitialized ExtendedUnit.
	 */
	ExtendedUnit() = default;
	/**
	 * Makes a copy.
	 */
	ExtendedUnit(const ExtendedUnit &) = default;
	/**
	 * Makes an ExtendedUnit that is equivalent to the given base Unit.
	 * @param u  The base unit.
	 */
	constexpr ExtendedUnit(Unit u) : unit(u), scaloff(0) { }
	/**
	 * Constructs an ExtendedUnit with the given values.
	 * @tparam O  The offset type; must be either @a float or @a double. In most
	 *            cases, the compiler should deduce this type.
	 * @param  u  The initial Unit value.
	 * @param  o  The initial offset value.
	 * @param  s  The initial scalar value.
	 * @todo   Document the offset and scalar better.
	 * @throw  UnitRangeError  Either the scalar or offset value is beyond the
	 *                         bounds of the internal storage.
	 */
	template <typename O>
	ExtendedUnit(Unit u, O o, int s) : unit(u), scale(s) {
		if ((s > 7) || (s < -8)) {
			DUDS_THROW_EXCEPTION(UnitRangeError());
		}
		offset(o);
	}
	/**
	 * True if the unit represented by this object can also be represented by
	 * a Unit object.
	 */
	bool canConvertToUnit() const;
	/**
	 * Change the offset.
	 * @param o  The new offset. Is must not be subnormal, infinite, or NaN.
	 * @throw  UnitRangeError  The offset value is beyond the bounds of the
	 *                         internal storage.
	 */
	void offset(float o);
	/**
	 * Query the offset as a float.
	 */
	float offsetf() const;
	/**
	 * Change the offset.
	 * @param o  The new offset. Is must not be subnormal, infinite, or NaN.
	 * @throw  UnitRangeError  The offset value is beyond the bounds of the
	 *                         internal storage.
	 */
	void offset(double o);
	/**
	 * Query the offset as a double.
	 */
	double offset() const;
	/**
	 * Returns the base unit as a Unit object.
	 */
	const Unit &base() const {
		return unit;
	}
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int ampere() const {
		return unit.ampere();
	}
	/**
	 * Returns the exponent for the candela (luminous intensity) dimension.
	 */
	int candela() const {
		return unit.candela();
	}
	/**
	 * Returns the exponent for the kelvin (thermodynamic temperature) dimension.
	 */
	int kelvin() const {
		return unit.kelvin();
	}
	/**
	 * Returns the exponent for the kilogram (mass) dimension.
	 */
	int kilogram() const {
		return unit.kilogram();
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int meter() const {
		return unit.meter();
	}
	/**
	 * Returns the exponent for the mole (amount of substance) dimension.
	 */
	int mole() const {
		return unit.mole();
	}
	/**
	 * Returns the exponent for the second (time) dimension.
	 */
	int second() const {
		return unit.second();
	}
	/**
	 * Returns the exponent for radians (angle).
	 */
	int radian() const {
		return unit.radian();
	}
	/**
	 * Returns the exponent for steradians (solid angle).
	 */
	int steradian() const {
		return unit.steradian();
	}
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int amp() const {
		return unit.ampere();
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int metre() const {
		return unit.meter();
	}
	/**
	 * Returns the exponent for the ampere (current) dimension.
	 */
	int A() const {
		return unit.ampere();
	}
	/**
	 * Returns the exponent for the candela (luminous intensity) dimension.
	 */
	int cd() const {
		return unit.candela();
	}
	/**
	 * Returns the exponent for the kelvin (thermodynamic temperature) dimension.
	 */
	int K() const {
		return unit.kelvin();
	}
	/**
	 * Returns the exponent for the kilogram (mass) dimension.
	 */
	int kg() const {
		return unit.kilogram();
	}
	/**
	 * Returns the exponent for the meter (distance) dimension.
	 */
	int m() const {
		return unit.meter();
	}
	/**
	 * Returns the exponent for the mole (amount of substance) dimension.
	 */
	int mol() const {
		return unit.mole();
	}
	/**
	 * Returns the exponent for the second (time) dimension.
	 */
	int s() const {
		return unit.second();
	}
	/**
	 * Returns the exponent for radians (angle).
	 */
	int rad() const {
		return unit.radian();
	}
	/**
	 * Returns the exponent for steradians (solid angle).
	 */
	int sr() const {
		return unit.steradian();
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void ampere(int e) {
		unit.ampere(e);
	}
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void candela(int e) {
		unit.candela(e);
	}
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension
	 * without range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kelvin(int e) {
		unit.kelvin(e);
	}
	/**
	 * Sets the exponent for the ampere (current)kilogram (mass) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kilogram(int e) {
		unit.kilogram(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void meter(int e) {
		unit.meter(e);
	}
	/**
	 * Sets the exponent for the mole (amount of substance) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void mole(int e) {
		unit.mole(e);
	}
	/**
	 * Sets the exponent for the second (time) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void second(int e) {
		unit.second(e);
	}
	/**
	 * Sets the exponent for radians (angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void radian(int e) {
		unit.radian(e);
	}
	/**
	 * Sets the exponent for steradians (solid angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void steradian(int e) {
		unit.steradian(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void amp(int e) {
		unit.ampere(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void metre(int e) {
		unit.meter(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void A(int e) {
		unit.ampere(e);
	}
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void cd(int e) {
		unit.candela(e);
	}
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension
	 * without range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void K(int e) {
		unit.kelvin(e);
	}
	/**
	 * Sets the exponent for the kilogram (mass) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void kg(int e) {
		unit.kilogram(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension without range
	 * checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void m(int e) {
		unit.meter(e);
	}
	/**
	 * Sets the exponent for the mole (amount of substance) dimension without
	 * range checking.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 */
	void mol(int e) {
		unit.mole(e);
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
		unit.radian(e);
	}
	/**
	 * Sets the exponent for steradians (solid angle) without range checking.
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 */
	void sr(int e) {
		unit.steradian(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setAmpere(int e) {
		unit.setAmpere(e);
	}
	/**
	 * Sets the exponent for the candela (luminous intensity) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setCandela(int e) {
		unit.setCandela(e);
	}
	/**
	 * Sets the exponent for the kelvin (thermodynamic temperature) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setKelvin(int e) {
		unit.setKelvin(e);
	}
	/**
	 * Sets the exponent for the kilogram (mass) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setKilogram(int e) {
		unit.setKilogram(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMeter(int e) {
		unit.setMeter(e);
	}
	/**
	 * Sets the exponent for the mole (amount of substance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMole(int e) {
		unit.setMole(e);
	}
	/**
	 * Sets the exponent for the second (time) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setSecond(int e) {
		unit.setSecond(e);
	}
	/**
	 * Sets the exponent for radians (angle).
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setRadian(int e) {
		unit.setRadian(e);
	}
	/**
	 * Sets the exponent for steradians (solid angle).
	 * @param e  The exponent in a range between -2 to 1, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setSteradian(int e) {
		unit.setSteradian(e);
	}
	/**
	 * Sets the exponent for the ampere (current) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setAmp(int e) {
		unit.setAmpere(e);
	}
	/**
	 * Sets the exponent for the meter (distance) dimension.
	 * @param e  The exponent in a range between -8 to 7, inclusive.
	 * @throw UnitRangeError  The exponent @a e is out of range.
	 */
	void setMetre(int e) {
		unit.setMeter(e);
	}
	/**
	 * Returns true if the object represents no units. This is normal for some
	 * values, such as ratios.
	 */
	bool unitless() const {
		return unit.unitless();
	}
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range.
	 */
	const ExtendedUnit operator * (const Unit &U) const;
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range.
	 */
	const ExtendedUnit operator / (const Unit &U) const;
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range. All
	 *                           work is done on a temporary so that if an
	 *                           exception is thrown only the temporary is
	 *                           modified.
	 */
	ExtendedUnit &operator *= (const Unit &U);
	/**
	 * Combines two units into a new unit.
	 * @throw UnitRangeError  A resulting exponent is out of range. All
	 *                           work is done on a temporary so that if an
	 *                           exception is thrown only the temporary is
	 *                           modified.
	 */
	ExtendedUnit &operator /= (const Unit &U);
	/**
	 * Makes this extended unit equivalent to the given Unit object.
	 */
	ExtendedUnit &operator = (const Unit &U);
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator < (const ExtendedUnit &U) const {
		return ((unit == U.unit) && (scaloff < U.scaloff)) || (unit < U.unit);
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator > (const ExtendedUnit &U) const {
		return ((unit == U.unit) && (scaloff > U.scaloff)) || (unit > U.unit);
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator <= (const ExtendedUnit &U) const {
		return ((unit == U.unit) && (scaloff <= U.scaloff)) || (unit <= U.unit);
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator >= (const ExtendedUnit &U) const {
		return ((unit == U.unit) && (scaloff >= U.scaloff)) || (unit >= U.unit);
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator == (const ExtendedUnit &U) const {
		return (unit == U.unit) && (scaloff == U.scaloff);
	}
	/**
	 * Unit objects are compared using the @a u member.
	 */
	bool operator != (const ExtendedUnit &U) const {
		return (unit != U.unit) || (scaloff != U.scaloff);
	}
};

/**
 * An idea that is not yet implemented; <b>DO NOT USE</b>.
 */
template <class Q = double>
struct ExtendedQuantity {
	Q value;
	ExtendedUnit unit;
private:
	// serialization support
	friend class boost::serialization::access;
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(value);
		a & BOOST_SERIALIZATION_NVP(unit);
	}
};

} }

#endif        //  #ifndef EXTENDEDUNIT_HPP

