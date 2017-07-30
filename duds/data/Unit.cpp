/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/Unit.hpp>

namespace duds { namespace data {

void Unit::setAmpere(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Ampere"));
	}
	u = (u & 0xFFFFFFF0) | v;
}

void Unit::setCandela(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Candela"));
	}
	u = (u & 0xFFFFFF0F) | (v << 4);
}

void Unit::setKelvin(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Kelvin"));
	}
	u = (u & 0xFFFFF0FF) | (v << 8);
}

void Unit::setKilogram(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Kilogram"));
	}
	u = (u & 0xFFFF0FFF) | (v << 12);
}

void Unit::setMeter(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Meter"));
	}
	u = (u & 0xFFF0FFFF) | (v << 16);
}

void Unit::setMole(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Mole"));
	}
	u = (u & 0xFF0FFFFF) | (v << 20);
}

void Unit::setSecond(int e) {
	int v = general::SignExtend<4>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Second"));
	}
	u = (u & 0xF0FFFFFF) | (v << 24);
}

void Unit::setRadian(int e) {
	int v = general::SignExtend<2>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Radian"));
	}
	u = (u & 0xCFFFFFFF) | (v << 28);
}

void Unit::setSteradian(int e) {
	int v = general::SignExtend<2>(e);
	if (v != e) {
		BOOST_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(e) <<
			BadUnit("Steradian"));
	}
	u = (u & 0x3FFFFFFF) | (v << 30);
}

Unit::Unit(int A, int cd, int K, int kg, int m, int mol, int s,
int rad, int sr) {
	setAmpere(A);
	setCandela(cd);
	setKelvin(K);
	setKilogram(kg);
	setMeter(m);
	setMole(mol);
	setSecond(s);
	setRadian(rad);
	setSteradian(sr);
}

const Unit Unit::operator * (const Unit &U) const {
	// the result
	Unit r;
	// set the result; may fail with exception
	r.setAmpere(ampere() + U.ampere());
	r.setCandela(candela() + U.candela());
	r.setKelvin(kelvin() + U.kelvin());
	r.setKilogram(kilogram() + U.kilogram());
	r.setMeter(meter() + U.meter());
	r.setMole(mole() + U.mole());
	r.setSecond(second() + U.second());
	r.setRadian(radian() + U.radian());
	r.setSteradian(steradian() + U.steradian());
	// return the result
	return r;
}

const Unit Unit::operator / (const Unit &U) const {
	// the result
	Unit r;
	// set the result; may fail with exception
	r.setAmpere(ampere() - U.ampere());
	r.setCandela(candela() - U.candela());
	r.setKelvin(kelvin() - U.kelvin());
	r.setKilogram(kilogram() - U.kilogram());
	r.setMeter(meter() - U.meter());
	r.setMole(mole() - U.mole());
	r.setSecond(second() - U.second());
	r.setRadian(radian() - U.radian());
	r.setSteradian(steradian() - U.steradian());
	// return the result
	return r;
}

Unit &Unit::operator *= (const Unit &U) {
	// a temporary
	Unit n(*this * U);  // may throw
	// keep the new value
	u = n.value();
	return *this;
}

Unit &Unit::operator /= (const Unit &U) {
	// a temporary
	Unit n(*this / U);  // may throw
	// keep the new value
	u = n.value();
	return *this;
}

} }
