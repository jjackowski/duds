/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/ExtendedUnit.hpp>

namespace duds { namespace data {

void ExtendedUnit::offset(float o) {
	const std::uint32_t &oi = *(std::uint32_t*)(&o);
	int temp = ((oi >> 23) & 0xFF) - 127;
	if ((temp > 62) || (temp < -63)) {
		DUDS_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(temp));
	}
	exp = temp;
	sign = oi >= (1 << 31);
	mant = (oi & 0x7FFFFF) >> 3;
}

float ExtendedUnit::offsetf() const {
	std::uint32_t oo = (mant << 3) | ((exp + 127) << 23) | (sign << 31);
	return *((float*)(&oo));
}

void ExtendedUnit::offset(double o) {
	const std::uint64_t &oi = *(std::uint64_t*)(&o);
	int temp = ((oi >> 52) & 0x7FF) - 1023;
	if ((temp > 62) || (temp < -63)) {
		DUDS_THROW_EXCEPTION(UnitRangeError() << BadUnitExponent(temp));
	}
	exp = temp;
	sign = oi >= ((std::uint64_t)1 << 63);
	mant = (oi & (std::uint64_t)0xFFFFFFFFFFFFF) >> 32;
}

double ExtendedUnit::offset() const {
	std::uint64_t oo = ((std::uint64_t)mant << 32) |
		((std::uint64_t)(exp + 1023) << 52) | ((std::uint64_t)sign << 63);
	return *((double*)(&oo));
}

const ExtendedUnit ExtendedUnit::operator * (const Unit &U) const {
	// return the result; may fail with exception
	return ExtendedUnit(unit * U, scaloff);
}

const ExtendedUnit ExtendedUnit::operator / (const Unit &U) const {
	// return the result; may fail with exception
	return ExtendedUnit(unit / U, scaloff);
}

ExtendedUnit &ExtendedUnit::operator *= (const Unit &U) {
	// a temporary
	Unit n(unit * U);  // may throw
	// keep the new value
	unit = n.value();
	return *this;
}

ExtendedUnit &ExtendedUnit::operator /= (const Unit &U) {
	// a temporary
	Unit n(unit / U);  // may throw
	// keep the new value
	unit = n.value();
	return *this;
}

ExtendedUnit &ExtendedUnit::operator = (const Unit &U) {
	unit = U;
	scaloff = 0;
	return *this;
}

} }

