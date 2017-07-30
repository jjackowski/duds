/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef QUANTITY_HPP
#define QUANTITY_HPP

#include <duds/data/Units.hpp>
#include <duds/time/interstellar/Interstellar.hpp>

namespace duds { namespace data {

/**
 * A container for a value and a unit to better describe the value.
 *
 * This struct has a full set of arithmetic operators with Quantity objects
 * as both operands. These operators enforce proper use of units by throwing
 * exceptions when the operation is invalid. For multiplication and division,
 * the new unit is also computed for the result.
 *
 * The @a unit member is a Unit, which is simpiler to work with than an
 * ExtendedUnit. Because of the limitations of Unit, the @a value member must
 * be floating point. Bewteen this and the decision to use doubles unless
 * integers are a better representation, I decided not to make this class
 * a template and force the use of a double.
 *
 * @todo  Consider allowing user-defined literals with common units.
 *
 * @author  Jeff Jackowski
 */
struct Quantity {
	/**
	 * Some value; probably something measured.
	 */
	double value;
	/**
	 * The units describing the value.
	 */
	Unit unit;
	/**
	 * The default constructor; expect @a value and @a unit to be uninitialized.
	 */
	Quantity() = default;
	/**
	 * A default copy constrcutor.
	 */
	Quantity(const Quantity &) = default;
	/**
	 * Constructs a new Quantity with the given values.
	 * @param v  The initial value.
	 * @param u  The initlal unit.
	 */
	constexpr Quantity(const double &v, const Unit &u) : value(v), unit(u) { }
	/**
	 * Constructs a new Quantity holding the number of seconds stored in the
	 * given duration.
	 * @tparam Duration  A time duration class related to std::chrono::duration.
	 * @param  d         The duration.
	 */
	template <class Duration>
	Quantity(const Duration &d) : unit(duds::data::units::Second) {
		std::chrono::duration<double> sec =
			std::chrono::duration_cast<std::chrono::seconds>(d);
		value = sec.count();
	}
	/**
	 * Adds two quantities; they must use the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	Quantity operator + (const Quantity &q) const;
	/**
	 * Subtracts a quantity from another; they must use the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	Quantity operator - (const Quantity &q) const;
	/**
	 * Multiplies two quantities; the units are also multiplied.
	 * @throw BadUnitExponent  The resulting units cannot be represented.
	 */
	Quantity operator * (const Quantity &q) const {
		return Quantity(value * q.value, unit * q.unit);
	}
	/**
	 * Multiplies a Quantity by a scalar.
	 */
	Quantity operator * (double s) const {
		return Quantity(value * s, unit);
	}
	/**
	 * Divides a quantity by another; the units are also divided.
	 * @throw BadUnitExponent  The resulting units cannot be represented.
	 */
	Quantity operator / (const Quantity &q) const {
		return Quantity(value / q.value, unit / q.unit);
	}
	/**
	 * Divides a Quantity by a scalar.
	 */
	Quantity operator / (double s) const {
		return Quantity(value / s, unit);
	}
	/**
	 * Adds two quantities; they must use the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	Quantity &operator += (const Quantity &q);
	/**
	 * Subtracts a quantity from another; they must use the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	Quantity &operator -= (const Quantity &q);
	/**
	 * Multiplies two quantities; the units are also multiplied.
	 * @throw BadUnitExponent  The resulting units cannot be represented.
	 */
	Quantity &operator *= (const Quantity &q);
	/**
	 * Multiplies a Quantity by a scalar.
	 */
	Quantity &operator *= (double s) {
		value *= s;
		return *this;
	}
	/**
	 * Divides a quantity by another; the units are also divided.
	 * @throw BadUnitExponent  The resulting units cannot be represented.
	 */
	Quantity &operator /= (const Quantity &q);
	/**
	 * Divides a Quantity by a scalar.
	 */
	Quantity &operator /= (double s) {
		value /= s;
		return *this;
	}
	/**
	 * Compares two quantities of the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	bool operator < (const Quantity &q) const;
	/**
	 * Compares two quantities of the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	bool operator > (const Quantity &q) const;
	/**
	 * Compares two quantities of the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	bool operator <= (const Quantity &q) const;
	/**
	 * Compares two quantities of the same units.
	 * @throw UnitMismatch  The quantities are for different units.
	 */
	bool operator >= (const Quantity &q) const;
	/**
	 * Compares two quantities for equality.
	 */
	bool operator == (const Quantity &q) const {
		return (unit == q.unit) && (value == q.value);
	}
	/**
	 * Compares two quantities for inequality.
	 */
	bool operator != (const Quantity &q) const {
		return (unit != q.unit) || (value != q.value);
	}
	/**
	 * Sets a duration to the seconds stored in this quantity.
	 * @pre      The units of this quantity are seconds.
	 * @tparam   Duration  The duration type. It must be usable in the same
	 *           manner as std::chrono::duration.
	 * @param d  The duration object to set. The value will be converted to
	 *           the units used by the duration type.
	 * @throw UnitBadConversion  The quantity value is not in seconds.
	 */
	template <class Duration>
	void toDuration(Duration &d) const {
		if (unit != duds::data::units::Second) {
			BOOST_THROW_EXCEPTION(UnitBadConversion() << BadUnit(unit));
		}
		d = Duration(typename Duration::rep(value *
			(double)(Duration::period::num) /
			(double)(Duration::period::den)
		) );
	}
	/**
	 * Returns a duration with the seconds stored in this quantity.
	 * @pre      The units of this quantity are seconds.
	 * @tparam   Duration  The duration type. It must be usable in the same
	 *           manner as std::chrono::duration.
	 * @return   The duration object. The value will be converted to
	 *           the units used by the duration type.
	 * @throw UnitBadConversion  The quantity value is not in seconds.
	 */
	template <class Duration>
	Duration toDuration() const {
		if (unit != duds::data::units::Second) {
			BOOST_THROW_EXCEPTION(UnitBadConversion() << BadUnit(unit));
		}
		return Duration(typename Duration::rep(value *
			(double)(Duration::period::num) /
			(double)(Duration::period::den)
		) );
	}
private:
	// serialization support
	friend class boost::serialization::access;
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(value);
		a & BOOST_SERIALIZATION_NVP(unit);
	}
};

/**
 * Multiplies a Quantity by a scalar.
 */
inline Quantity operator * (double s, const Quantity &q) {
	return q * s;
}

/**
 * Divides a Quantity by a scalar.
 */
inline Quantity operator / (double s, const Quantity &q) {
	return Quantity(s / q.value, q.unit);
}

/*
 * Assigns a duration in femtoseconds.
 * @param q  The quantity to convert; must be in seconds.
 * @throw UnitBadConversion  The quantity is not in seconds.
 * /
::duds::time::interstellar::Femtoseconds &operator = (
	::duds::time::interstellar::Femtoseconds &fs, const Quantity &q
);

/ **
 * Assigns a duration in nanoseconds.
 * @param q  The quantity to convert; must be in seconds.
 * @throw UnitBadConversion  The quantity is not in seconds.
 * /
::duds::time::interstellar::Nanoseconds &operator = (
	::duds::time::interstellar::Nanoseconds &ns, const Quantity &q
);
*/
} }

#endif        //  #ifndef QUANTITY_HPP
