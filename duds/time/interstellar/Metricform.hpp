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
 * of 1000, three decimal digits each, up to a gigaseconds field. The size of
 * these fields jumps by large bounds compared to common systems of time and
 * date used on Earth, which may make this form less desirable for use than
 * Hectoform, but its fields all use proper metric prefixes.
 * @author  Jeff Jackowski
 */
 struct Metricform {
	/**
	 * Gigaseconds field.
	 * One unit in regular old Earth time is:
	 * 31 years, 251 days, 13 hours, 21 minutes, and 28 seconds.
	 */
	unsigned int G : 32;
	/**
	 * Megaseconds field.
	 * One unit in regular old Earth time is:
	 * 11 days, 13 hours, 46 minutes, and 40 seconds.
	 */
	unsigned int M : 10;
	/**
	 * Kiloseconds field.
	 * One unit in regular old Earth time is:
	 * 16 minutes and 40 seconds.
	 */
	unsigned int k : 10;
	/**
	 * Seconds field.
	 */
	unsigned int s : 10;
	/**
	 * The negative flag. It doesn't make sense to have every number in this
	 * struct be negative for a negative time, and integers cannot be negative
	 * zero, so this flag is used instead.
	 */
	unsigned int neg : 1;
	/**
	 * Default constructor.
	 */
	Metricform() = default;
	/**
	 * Constructs a Metricform with the duration contained in @a d truncated to
	 * seconds.
	 * @tparam Duration  A time duration class related to std::chrono::duration.
	 * @param  d         The duration.
	 */
	template <class Rep, class Period>
	Metricform(const std::chrono::duration<Rep, Period> &d) {
		setDuration(d);
	}
	/**
	 * Constructs a Metricform with the time contained in @a t truncated to
	 * seconds.
	 * @tparam Clock   A clock class, such as FemtoClock.
	 * @param  t       The time.
	 */
	template <class Clock, class Duration>
	Metricform(const std::chrono::time_point<Clock, Duration> &t) {
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
		s = (unsigned int)(sec % 1000);
		sec /= 1000;
		k = (unsigned int)(sec % 1000);
		sec /= 1000;
		M = (unsigned int)(sec % 1000);
		G = (unsigned int)(sec / 1000);
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
 * Writes the Metricform time in its Human readable format in plain text.
 * @author  Jeff Jackowski
 */
std::ostream &operator << (std::ostream &os, const Metricform &m);

} } }

