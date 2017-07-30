/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef INTERSTELLAR_HPP
#define INTERSTELLAR_HPP

#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <duds/data/Int128.hpp>

namespace duds { namespace time { namespace interstellar {

using duds::data::int128_t;

/**
 * 1e6 in a 128-bit integer.
 */
extern const int128_t OneE6;
/**
 * 1e12 in a 128-bit integer.
 */
extern const int128_t OneE12;
/**
 * 1e15 in a 128-bit integer.
 */
extern const int128_t OneE15;

/**
 * An extention to the C++ std::chrono::time_point template to make time
 * points easier to work with. I found the inclusion of the clock type to
 * be rather bothersome since the DUDS library doesn't use types to track
 * the origin of information. This class avoids that issue, and allows
 * easier type conversions and cross-type assignments with a lot less typing.
 * I find it is also eaiser to write code that doesn't result in compile-time
 * errors using this template rather than using the C++ one directly.
 *
 * @warning   This template allows implicit conversions that result in a loss
 *            of precision. For example, a time in nanoseconds can be converted
 *            to seconds. This is not the case with std::chrono::time_point.
 *
 * @tparam Clock     A clock class.
 * @tparam Duration  A duration class.
 *
 * @author  Jeff Jackowski
 */
template <class Clock, class Duration>
class TimePoint : public std::chrono::time_point<Clock, Duration> {
public:
	constexpr TimePoint() = default;
	TimePoint(const TimePoint &) = default;
	/**
	 * Constructs a TimePoint from a duration.
	 * @param od  A duration from time-zero, epoch, whatever you want to call it.
	 */
	template <class OtherRep, class OtherPeriod>
	constexpr TimePoint(const std::chrono::duration<OtherRep, OtherPeriod> &od) :
	std::chrono::time_point<Clock, Duration>(
		std::chrono::duration_cast<Duration, typename Duration::rep, typename Duration::period>(od)
	) { }
	/*
	 * Constructs a TimePoint from another TimePoint.
	 * @param otp  The source TimePoint. It may use a different period and a
	 *             a different clock type.
	 */
	 /*
	template <class OtherClock, class OtherDuration>
	constexpr TimePoint(const TimePoint<OtherClock, OtherDuration> &otp) :
	std::chrono::time_point<Clock, Duration>(
		std::chrono::duration_cast<Duration>(otp.time_since_epoch())
	) { }
	*/
	/**
	 * Constructs a TimePoint from a std::chrono::time_point.
	 * @param otp  The source time point. It may use a different period and a
	 *             a different clock type.
	 */
	template <class OtherClock, class OtherDuration>
	constexpr TimePoint(const std::chrono::time_point<OtherClock, OtherDuration> &otp) :
	std::chrono::time_point<Clock, Duration>(
		std::chrono::duration_cast<Duration>(otp.time_since_epoch())
	) { }
	TimePoint &operator=(const TimePoint &) = default;
	/**
	 * Assigns a new time from another TimePoint using different template
	 * parameters.
	 * @param otp  The source TimePoint. It may use a different period and a
	 *             a different clock type.
	 */
	template <class OtherClock, class OtherDuration>
	TimePoint &operator=(const TimePoint<OtherClock, OtherDuration> &otp) {
		*this = TimePoint(otp);
		return *this;
	}
	/**
	 * Returns the earlist time that can be represented.
	 */
	static constexpr TimePoint min() {
		return TimePoint(Duration(
			std::numeric_limits<typename Duration::rep>::lowest()
		) );
	}
	/**
	 * Returns the latest time that can be represented.
	 */
	static constexpr TimePoint max() {
		return TimePoint(Duration(
			std::numeric_limits<typename Duration::rep>::max()
		) );
	}
};

/**
 * Stores a duration in femtoseconds. The range of the 128-bit number
 * is about 1.701e23 seconds, or about 5.39 quadrillion Earth years, both
 * positive and negative.
 * @note    Seralization support is in Serialize.hpp. If needed, Serialize.hpp
 *          should be included instead of Interstellar.hpp.
 * @author  Jeff Jackowski
 */
typedef std::chrono::duration<int128_t, std::femto> Femtoseconds;

/**
 * Provides Interstellar Time in Femtoseconds. The range of the 128-bit number
 * is about 1.701e23 seconds, or about 5.39 quadrillion Earth years, before
 * and after time zero. This should continue to count past the time when the
 * last stars in the universe go dark, except for stars formed from collisions
 * of the remains of stars. Maybe enough time to do something about that
 * outcome?
 * @author  Jeff Jackowski
 */
struct FemtoClock {
	typedef Femtoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef TimePoint<FemtoClock, duration> time_point;
	static constexpr bool is_steady = false;
	static time_point now() noexcept {
		return time_point(std::chrono::duration_cast<duration, rep, period>(
			std::chrono::system_clock::now().time_since_epoch()));
	}
};

/**
 * A point in time in Interstellar Time stored in Femtoseconds. Time zero is
 * defined as the very begining of the Earth year 1972, Gregorian calendar,
 * according to TAI, minus 31536000 seconds (two non-leap years without leap
 * seconds). The range of 5.39 quadrillion Earth years before and after time
 * zero should allow this type to represent any time during the
 * [Stelliferous Era](http://en.wikipedia.org/wiki/Stelliferous_era#Stelliferous_Era).
 * This is a requirement for using the name Interstellar Time.
 * @note    Seralization support is in Serialize.hpp. If needed, Serialize.hpp
 *          should be included instead of Interstellar.hpp.
 */
typedef FemtoClock::time_point  FemtoTime;

/**
 * Stores a duration in milliseconds. The range of the 64-bit number
 * is about 9.223e15 seconds, or about 292 million Earth years, both positive
 * and negative. This should be useful for many long-running applications
 * where a 128-bit integer is considered burdensome.
 * @author  Jeff Jackowski
 */
typedef std::chrono::duration<std::int64_t, std::milli> Milliseconds;

/**
 * Provides Interstellar Time in Milliseconds. The range of the 64-bit number
 * is about 9.223e15 seconds, or about 292 million Earth years, before and
 * after time zero. This should be useful for many long-running applications
 * where a 128-bit integer is considered burdensome.
 * @author  Jeff Jackowski
 */
struct MilliClock {
	typedef Milliseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef TimePoint<MilliClock, duration> time_point;
	static constexpr bool is_steady = false;
	static time_point now() noexcept {
		return time_point(std::chrono::duration_cast<duration>(
			std::chrono::system_clock::now().time_since_epoch()));
	}
};

/**
 * A point in time in Interstellar Time stored in Milliseconds.
 * @note    Seralization support is in Serialize.hpp. If needed, Serialize.hpp
 *          should be included instead of Interstellar.hpp.
 */
typedef MilliClock::time_point  MilliTime;

/**
 * Stores a duration in nanoseconds. The range of the 64-bit number
 * is about 1.844e10 seconds, or about 584.5 Earth years, positive only.
 * This should be useful for applications needing better than millisecond
 * resolution where a 128-bit integer is considered burdensome.
 *
 * @todo  Maybe this should be signed.
 *
 * @author  Jeff Jackowski
 */
typedef std::chrono::duration<std::uint64_t, std::nano> Nanoseconds;

/**
 * Provides Interstellar Time in Nanoseconds. The range of the 64-bit number
 * is about 1.844e10 seconds, or about 584.5 Earth years, after time zero.
 * This should be useful for applications needing better than millisecond
 * resolution where a 128-bit integer is considered burdensome.
 * @author  Jeff Jackowski
 */
struct NanoClock {
	typedef Nanoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef TimePoint<NanoClock, duration> time_point;
	static constexpr bool is_steady = false;
	static time_point now() noexcept {
		return time_point(std::chrono::duration_cast<duration>(
			std::chrono::system_clock::now().time_since_epoch()));
	}
};

/**
 * A point in time in Interstellar Time stored in Nanoseconds.
 * @note    Seralization support is in Serialize.hpp. If needed, Serialize.hpp
 *          should be included instead of Interstellar.hpp.
 */
typedef NanoClock::time_point  NanoTime;

/**
 * Stores a duration in seconds. The range of the 64-bit number is about
 * 9.223e18 seconds, or about 292 billion Earth years, both positive and
 * negative. This should be useful for many long-running applications
 * where a 128-bit integer is considered burdensome and high resolution is
 * not required.
 * @author  Jeff Jackowski
 */
typedef std::chrono::duration<std::int64_t> Seconds;

/**
 * Provides Interstellar Time in seconds. The range of the 64-bit number
 * is about 9.223e18 seconds, or about 292 billion Earth years, before and
 * after time zero. This should be useful for many long-running applications
 * where a 128-bit integer is considered burdensome and high resolution is
 * not required.
 * @author  Jeff Jackowski
 */
struct SecondClock {
	typedef Seconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef TimePoint<SecondClock, duration> time_point;
	static constexpr bool is_steady = false;
	static time_point now() noexcept {
		return time_point(std::chrono::duration_cast<duration>(
			std::chrono::system_clock::now().time_since_epoch()));
	}
};

/**
 * A point in time in Interstellar Time stored in Seconds.
 * @note    Seralization support is in Serialize.hpp. If needed, Serialize.hpp
 *          should be included instead of Interstellar.hpp.
 */
typedef SecondClock::time_point  SecondTime;

#ifdef BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
#warning POSIX nanosecond
#else
//#warning POSIX microsecond
#endif

template <class ISTime>
boost::posix_time::ptime EarthTimeTAI(const ISTime &t) {  // ???
	return boost::posix_time::ptime((std::time_t)t.seconds());
}

} } }

#endif        //  #ifndef INTERSTELLAR_HPP
