/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/time/interstellar/Interstellar.hpp>

namespace duds { namespace time { namespace planetary {

/**
 * A container holding the number of leap seconds and the time bounds over
 * which the leap seconds are applied. Useful in cases when leap seconds will
 * be regularly queried, but the queries will normally be for similar times.
 * The template parameters allow for either smaller storage, or storage of the
 * specific types that will be used (compared, added) to avoid run-time
 * conversions.
 *
 * @tparam Clock     The clock type used with
 *                   duds::time::interstellar::TimePoint to define the
 *                   time bounds.
 * @tparam Duration  The duration type used with
 *                   duds::time::interstellar::TimePoint to define the
 *                   time bounds.
 * @tparam Count     The duration type used to store the number of leap
 *                   seconds.
 *
 * @author  Jeff Jackowski
 */
template <
	class Clock = duds::time::interstellar::SecondClock,
	class Duration = duds::time::interstellar::Seconds,
	class Count = duds::time::interstellar::Seconds
>
class LeapBounds {
public:
	/**
	 * The time point type used in this class.
	 */
	typedef duds::time::interstellar::TimePoint<Clock, Duration>  TimePoint;
private:
	/**
	 * The minimum bound time; times within bounds do not include this time.
	 */
	TimePoint min;
	/**
	 * The maximum bound time.
	 */
	TimePoint max;
	/**
	 * The total leap seconds to apply during this period.
	 */
	Count total;
public:
	/**
	 * A default constructor that sets invalid bounds and no leap seconds.
	 */
	constexpr LeapBounds() :
		min(TimePoint::max()),
		max(TimePoint::min()),
		total(Count(0)) { }
	/**
	 * Constructs with specific values.
	 * @param minimum  The minimum bound for the period.
	 * @param maximum  The maximum bound for the period.
	 * @param leaps    The number of leap seconds to apply during the period.
	 */
	constexpr LeapBounds(
		const TimePoint &minimum,
		const TimePoint &maximum,
		const Count &leaps
	) : total(leaps), min(minimum), max(maximum) { }
	LeapBounds(const LeapBounds &) = default;
	LeapBounds &operator = (const LeapBounds &) = default;
	/**
	 * Assigns from a different LeapBounds template.
	 * @param lb  A LeapBounds object. Its values will be converted to the types
	 *            used by this object and the stored.
	 */
	template <class OtherClock, class OtherDuration, class OtherCount>
	LeapBounds &operator = (
		const LeapBounds<OtherClock, OtherDuration, OtherCount> &lb
	) {
		min = lb.minimum();
		max = lb.maximum();
		total = std::chrono::duration_cast<Count>(lb.leaps());
		return *this;
	}
	/**
	 * Constructs from a different LeapBounds template.
	 * @param lb  A LeapBounds object. Its values will be converted to the types
	 *            used by this object and the stored.
	 */
	template <class OtherClock, class OtherDuration, class OtherCount>
	LeapBounds(
		const LeapBounds<OtherClock, OtherDuration, OtherCount> &lb
	) {
		this = lb;
	}
	/**
	 * Returns the minimum bound time; the minimum is exclusive.
	 */
	const TimePoint &minimum() const {
		return min;
	}
	/**
	 * Returns the maximum bound time; the maximum is inclusive.
	 */
	const TimePoint &maximum() const {
		return max;
	}
	/**
	 * Returns the leap seconds in this period.
	 */
	const Count &leaps() const {
		return total;
	}
	/**
	 * Returns true if the given provided time is within bounds.
	 * @param time  The time to check.
	 * @return      True if @a time is within the bounds of this period.
	 */
	template <class OtherClock, class OtherDuration>
	bool within(
		const duds::time::interstellar::TimePoint<OtherClock, OtherDuration> &time
	) const {
		TimePoint comp = time;  // convert
		return (comp > min) && (comp <= max);
	}
	/**
	 * Returns true if the given provided time is within bounds.
	 * @param time  The time to check.
	 * @return      True if @a time is within the bounds of this period.
	 */
	bool within(const TimePoint &time) const {
		return (time > min) && (time <= max);
	}
	/**
	 * Checks for validity; false if the maximum bound is under the minimum
	 * bound.
	 */
	bool valid() const {
		return min < max;
	}
};

} } }
