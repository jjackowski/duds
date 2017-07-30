/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <iostream>
#include <chrono>

namespace duds { namespace time { namespace interstellar {

/**
 * Holds Interstellar Time down to seconds in fields that increase by a power
 * of 100, two decimal digits each, up to a 1e10 seconds field. The size of
 * these fields seem to be in a better range for human use than the fields of
 * Metricform, but there are jumps that people of Earth are not acustomed to
 * starting with megaseconds. The field names try to use metric prefixes, but
 * prefixes do not exist for all field sizes, and some uncommon prefixes are
 * put to use.
 * @author  Jeff Jackowski
 */
struct Hectoform {
	/**
	 * 1e10 seconds field.
	 * One unit in regular old Earth time is:
	 * 316 years, 324 days, 2 hours, 39 minutes, and 28 seconds.
	 */
	unsigned int e10 : 25;
	/**
	 * 1e8 seconds field.
	 * One unit in regular old Earth time is:
	 * 3 years, 61 days, 16 hours, 19 minutes, and 4 seconds.
	 */
	unsigned int e8  : 7;
	/**
	 * Megaseconds field.
	 * One unit in regular old Earth time is:
	 * 11 days, 13 hours, 46 minutes, and 40 seconds.
	 */
	unsigned int M   : 7;
	/**
	 * Myriaseconds field.
	 * One unit in regular old Earth time is:
	 * 2 hours, 46 minutes, and 40 seconds.
	 * @note  The metric prefix myria has been deprecated since 1960.
	 */
	unsigned int ma  : 7;
	/**
	 * Hectoseconds field.
	 * One unit in regular old Earth time is:
	 * 1 minute and 40 seconds.
	 */
	unsigned int h   : 7;
	/**
	 * Seconds field.
	 */
	unsigned int s   : 7;
	/**
	 * The negative flag. It doesn't make sense to have every number in this
	 * struct be negative for a negative time, and integers cannot be negative
	 * zero, so this flag is used instead.
	 */
	unsigned int neg : 1;
	/**
	 * Default constructor.
	 */
	Hectoform() = default;
	/**
	 * Constructs a Hectoform with the duration contained in @a d truncated to
	 * seconds.
	 * @tparam Duration  A time duration class related to std::chrono::duration.
	 * @param  d         The duration.
	 */
	template <class Rep, class Period>
	Hectoform(const std::chrono::duration<Rep, Period> &d) {
		setDuration(d);
	}
	/**
	 * Constructs a Hectoform with the time contained in @a t truncated to
	 * seconds.
	 * @tparam Clock   A clock class, such as FemtoClock.
	 * @param  t       The time.
	 */
	template <class Clock, class Duration>
	Hectoform(const std::chrono::time_point<Clock, Duration> &t) {
		setTime(t);
	}
	/**
	 * Sets the stored time to be the same as the time given.
	 * @tparam Int  An integer type or something that acts like an integer.
	 * @param  t    The time in seconds.
	 */
	template <class Int>
	void setToSeconds(Int t) {
		Int sec;
		if (t < 0) {
			sec = -t; // std::abs() is not defined for __int128
			neg = 1;
		} else {
			sec = t;
			neg = 0;
		}
		// typecasting is required to work reliably with all valid types for
		// Int, especially boost::multiprecision::int128_t
		s = (unsigned int)(sec % 100);
		sec /= 100;
		h = (unsigned int)(sec % 100);
		sec /= 100;
		ma = (unsigned int)(sec % 100);
		sec /= 100;
		M = (unsigned int)(sec % 100);
		sec /= 100;
		e8 = (unsigned int)(sec % 100);
		e10 = (unsigned int)(sec / 100);
	}
	/**
	 * Sets the stored time to be the same as the time given truncated to
	 * seconds.
	 * @tparam Duration  A time duration class related to std::chrono::duration.
	 * @param  d         The duration.
	 */
	template <class Duration>
	void setDuration(const Duration &d) {
		std::chrono::duration<typename Duration::rep> sec =
		std::chrono::duration_cast<std::chrono::seconds>(d);
		setToSeconds(sec.count());
	}
	template <class Time>
	void setTime(const Time &d) {
		std::chrono::duration<typename Time::rep> sec =
		std::chrono::duration_cast<std::chrono::seconds>(d.time_since_epoch());
		setToSeconds(sec.count());
	}
};

/**
 * Writes the Hectoform time in its Human readable format in plain text.
 * @author  Jeff Jackowski
 */
std::ostream &operator << (std::ostream &os, const Hectoform &h);

} } }
