/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef PLANETARY_HPP
#define PLANETARY_HPP

#include <duds/time/planetary/LeapSeconds.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <duds/data/GenericValue.hpp>

namespace duds {

/**
 * Items that deal with time, a surprisingly difficult and annoying subject.
 * So difficult and annoying, I opted against an implementation close to
 * exitsing ones because those implementations and their conventions seem
 * to be part of the problem. Seconds will not be redefined, solar days
 * do not have a constant length, and a one second point in time will not
 * occur twice. The system here lives within that reality.
 * @author  Jeff Jackowski
 */
namespace time {

namespace planetary {

using std::unique_ptr;

class PlanetaryTime {
	LeapSeconds leaps;
	int days;
};


// move to earth.hpp
/**
 * Terran time stuffs.
 */
class Earth {
	LeapBounds<> bound;
public:
	Earth();
	LeapSeconds leaps;
	static const boost::gregorian::date &dateZero();
	static const boost::posix_time::ptime &timeZero();
	static void make(const std::string &path = "/usr/share/zoneinfo-leaps/UTC");
	typedef duds::time::interstellar::Femtoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef std::chrono::time_point<Earth, duration> time_point;
	static constexpr bool is_steady = false;
	time_point now() noexcept {
		// wrong
		return time_point(std::chrono::duration_cast<duration, rep, period>(
			std::chrono::system_clock::now().time_since_epoch()));
	}
	/**
	 * Converts from IST to UTC and provides a date result.
	 */
	boost::gregorian::date date(
		const duds::time::interstellar::SecondTime &t
	) const;
	template <class ISTime>
	/**
	 * Converts from IST to UTC and provides a date result.
	 */
	boost::gregorian::date date(const ISTime &t) const {
		return date(interstellar::SecondTime(
			std::chrono::duration_cast<interstellar::Seconds>(
				t.time_since_epoch()
			)
		));
	}
	/**
	 * Converts from UTC to TAI and provides a date result.
	 */
	boost::gregorian::date dateUtcToTai(
		const duds::time::interstellar::SecondTime &t
		) const;
	template <class ISTime>
	/**
	 * Converts from IST to UTC and provides a date result.
	 */
	boost::gregorian::date dateUtcToTai(const ISTime &t) const {
		return dateUtcToTai(interstellar::SecondTime(
			std::chrono::duration_cast<interstellar::Seconds>(
				t.time_since_epoch()
			)
		));
	}
	/**
	 * Converts from IST to UTC and provides a ptime result.
	 */
	boost::posix_time::ptime posix(
		const duds::time::interstellar::MilliTime &t
	) const;
	template <class ISTime>
	/**
	 * Converts from IST to UTC and provides a ptime result.
	 */
	boost::posix_time::ptime posix(const ISTime &t) const {
		return posix(duds::time::interstellar::MilliTime(
			std::chrono::duration_cast<duds::time::interstellar::Milliseconds>(
				t.time_since_epoch()
			)
		));
	}
	/**
	 * Converts from UTC to TAI and provides a ptime result.
	 */
	boost::posix_time::ptime posixUtcToTai(
		const duds::time::interstellar::MilliTime &t
	) const;
	template <class ISTime>
	/**
	 * Converts from UTC to TAI and provides a ptime result.
	 */
	boost::posix_time::ptime posixUtcToTai(const ISTime &t) const {
		return posixUtcToTai(duds::time::interstellar::MilliTime(
			std::chrono::duration_cast<duds::time::interstellar::Milliseconds>(
				t.time_since_epoch()
			)
		));
	}
	// UTC
	
	/**
	 * Adds leap seconds to the given time in UTC, resulting in TAI.
	 * @param time  The time that needs leap seconds.
	 */
	template <class Clock, class Duration>
	void addLeapSeconds(
		duds::time::interstellar::TimePoint<Clock, Duration> &time
	) {
		//duds::time::interstellar::SecondTime comp = time;
		if (!bound.within(time)) {
			bound = leaps.getLeapBounds(time);
		}
		assert(bound.within(time));
		time += bound.leaps;
	}
	
	/**
	 * Converts a Gregorian calendar date into one of the time formats defined
	 * inside the duds::time::interstellar namespace.
	 */
	template <class ISTime>
	void date(ISTime &dest, const boost::gregorian::date &src) {
		dest = duds::time::interstellar::SecondTime(
			duds::time::interstellar::Seconds((src - dateZero()).days() * 86400)
		);
		duds::time::interstellar::Seconds ls = leaps.leapSeconds(dest);
		dest += ls;
	}
	/**
	 * Converts a POSIX time into one of the time formats defined
	 * inside the duds::time::interstellar namespace.
	 */
	template <class ISTime>
	void time(ISTime &dest, const boost::posix_time::ptime &src) {
		dest = duds::time::interstellar::MilliTime(
			duds::time::interstellar::Milliseconds(
				(src - timeZero()).total_milliseconds()
			)
		);
		duds::time::interstellar::Seconds ls = leaps.leapSeconds(dest);
		dest += ls;
	}
};

extern unique_ptr<Earth> earth;

/**
 * Converts without applying any modification to the time, so if the input
 * is in TAI, the result will be, too. Not a member of Earth to emphasize that
 * this is just a type conversion that does no adjustment of the time.
 */
template <class ISTime>
inline boost::gregorian::date ToDate(const ISTime &t) {
	return Earth::dateZero() + boost::gregorian::days(
		std::chrono::duration_cast<duds::time::interstellar::Seconds>(
			t.time_since_epoch()
		).count()
	);
}

/**
 * Converts without applying any modification to the time, so if the input
 * is in TAI, the result will be, too. Not a member of Earth to emphasize that
 * this is just a type conversion that does no adjustment of the time.
 */
template <class ISTime>
inline boost::posix_time::ptime ToPosix(const ISTime &t) {
	return Earth::timeZero() + boost::posix_time::milliseconds(
		std::chrono::duration_cast<duds::time::interstellar::Milliseconds>(
			t.time_since_epoch()
		).count()
	);
}

} } }

#endif        //  #ifndef PLANETARY_HPP
