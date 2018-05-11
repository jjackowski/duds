/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/time/planetary/Planetary.hpp>

namespace duds { namespace time { namespace planetary {

// boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
const boost::gregorian::date EarthDateZero(boost::gregorian::date(1970,1,1));
const boost::posix_time::ptime EarthTimeZero(EarthDateZero);

unique_ptr<Earth> earth;

Earth::Earth() { }

const boost::gregorian::date &Earth::dateZero() {
	return EarthDateZero;
}

const boost::posix_time::ptime &Earth::timeZero() {
	return EarthTimeZero;
}

void Earth::make(const std::string &path) {
	earth = std::unique_ptr<Earth>(new Earth());
	earth->leaps.readZoneinfo(path);
}

std::time_t Earth::timeUtc(
	const duds::time::interstellar::SecondTime &t
) const {
	duds::time::interstellar::Seconds ls = leaps.leapSeconds(t);
	return (std::time_t)((t - ls).time_since_epoch().count());
}

boost::gregorian::date Earth::date(
	const duds::time::interstellar::SecondTime &t
) const {
	duds::time::interstellar::Seconds ls = leaps.leapSeconds(t);
	// TAI to UTC
	return EarthDateZero + boost::gregorian::days(
		(t - ls).time_since_epoch().count() / 86400
	);
}

boost::gregorian::date Earth::dateUtcToTai(
	const duds::time::interstellar::SecondTime &t
) const {
	duds::time::interstellar::Seconds ls = leaps.leapSeconds(t);
	// UTC to TAI
	return EarthDateZero + boost::gregorian::days(
		(t + ls).time_since_epoch().count() / 86400
	);
}

boost::posix_time::ptime Earth::posix(
	const duds::time::interstellar::MilliTime &t
) const {
	duds::time::interstellar::Milliseconds ls = leaps.leapSeconds(t);
	// TAI to UTC
	return EarthTimeZero + boost::posix_time::milliseconds(
		(t - ls).time_since_epoch().count()
	);
}

boost::posix_time::ptime Earth::posixUtcToTai(
	const duds::time::interstellar::MilliTime &t
) const {
	duds::time::interstellar::Milliseconds ls = leaps.leapSeconds(t);
	// UTC to TAI
	return EarthTimeZero + boost::posix_time::milliseconds(
		(t + ls).time_since_epoch().count()
	);
}

} } }
