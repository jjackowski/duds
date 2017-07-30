/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/time/interstellar/Metricform.hpp>
#include <duds/time/interstellar/Hectoform.hpp>
#include <duds/data/Int128.hpp>
#include <iomanip>

namespace duds { namespace time { namespace interstellar {

std::ostream &operator << (std::ostream &os, const Metricform &m) {
	char f = os.fill();
	if (m.neg) {
		os << '-';
	}
	os << m.G << std::setfill('0') << std::right << "G " << std::setw(3) << m.M
	<< "M " << std::setw(3) << m.k << "k " << std::setw(3) << m.s << 's' <<
	std::setfill(f) << std::left;
	return os;
}

std::ostream &operator << (std::ostream &os, const Hectoform &h) {
	char f = os.fill();
	if (h.neg) {
		os << '-';
	}
	os << h.e10 << std::setfill('0') << std::right << ':' << std::setw(2) <<
	h.e8 << ':' << std::setw(2) << h.M << '-' << std::setw(2) << h.ma << ':' <<
	std::setw(2) << h.h << ':' << std::setw(2) << h.s << std::setfill(f) <<
	std::left;
	return os;
}

const duds::data::int128_t OneE6(1000000L);
const duds::data::int128_t OneE12(1000000000000L);
const duds::data::int128_t OneE15(1000000000000000L);

} } }
