/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/Quantity.hpp>

namespace duds { namespace data {

Quantity Quantity::operator + (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return Quantity(value + q.value, unit);
}

Quantity Quantity::operator - (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return Quantity(value - q.value, unit);
}

Quantity &Quantity::operator += (const Quantity &q) {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	value += q.value;
	return *this;
}

Quantity &Quantity::operator -= (const Quantity &q) {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	value -= q.value;
	return *this;
}

Quantity &Quantity::operator *= (const Quantity &q) {
	unit *= q.unit;  // do first; may throw
	value *= q.value;
	return *this;
}

Quantity &Quantity::operator /= (const Quantity &q) {
	unit *= q.unit;  // do first; may throw
	value *= q.value;
	return *this;
}

bool Quantity::operator < (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return value < q.value;
}

bool Quantity::operator > (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return value > q.value;
}

bool Quantity::operator <= (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return value <= q.value;
}

bool Quantity::operator >= (const Quantity &q) const {
	if (unit != q.unit) {
		BOOST_THROW_EXCEPTION(UnitMismatch());
	}
	return value >= q.value;
}
/*
::duds::time::interstellar::Femtoseconds &operator = (
	::duds::time::interstellar::Femtoseconds &fs, const Quantity &q
) {
	if (q.unit != units::Seconds) {
		BOOST_THROW_EXCEPTION(UnitBadConversion() << BadUnit(q.unit));
	}
	fs = duds::time::interstellar::Femtoseconds(
		duds::time::interstellar::Femtoseconds::rep(
			q.value * (double)std::femto
		)
	);
	return fs;
}

::duds::time::interstellar::Nanoseconds &operator = (
	::duds::time::interstellar::Nanoseconds &ns, const Quantity &q
) {
	if (q.unit != units::Seconds) {
		BOOST_THROW_EXCEPTION(UnitBadConversion() << BadUnit(q.unit));
	}
	ns = duds::time::interstellar::Nanoseconds(
		duds::time::interstellar::Nanoseconds::rep(
			q.value * (double)std::nano
		)
	);
	return ns;
}
*/
} }
