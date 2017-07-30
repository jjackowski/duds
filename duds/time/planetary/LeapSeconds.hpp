/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/time/planetary/LeapBounds.hpp>
#include <duds/general/Spinlock.hpp>
#include <memory>
#include <map>

namespace duds { namespace time { namespace planetary {

using std::shared_ptr;

/**
 * Stores when leap seconds occur to allow conversions between Interstellar
 * Time or TAI and planetary time systems like UTC and POSIX. A time indicating
 * when the data will be outdated is also stored; no additional leap seconds
 * should be added until after this time.
 * @note  This class assumes the leap seconds before the first record is 0.
 *        This may not work well for times well in the past.
 * @todo  Allow for leap seconds that occur regularly in a more efficent manner.
 *        This will be useful to handle all possible cases of days or Sols or
 *        whatnot with lengths including fractional seconds when there is an
 *        insistance on using a whole number of seconds to describe the period.
 * @author  Jeff Jackowski
 */
class LeapSeconds {
public:
	/**
	 * The data structure used to hold leap seconds. The key is the time, and
	 * the value is the sum of all leap seconds in use @b after the time in the
	 * key.
	 */
	typedef std::map<
		duds::time::interstellar::SecondTime,
		duds::time::interstellar::Seconds
	>  LeapMap;
private:
	/**
	 * The leap seconds.
	 */
	shared_ptr<LeapMap> leaps;
	/**
	 * A time stamp indicating when the stored information may be outdated.
	 */
	duds::time::interstellar::Seconds currUntil;
	/**
	 * Used to make access and changes thread-safe.
	 * @todo  Consider having functions first get a shared pointrt to @a leaps
	 *        and then use that shared pointer rather than the member.
	 */
	mutable duds::general::Spinlock block;
public:
	/**
	 * Makes a new LeapSeconds object with no leap seconds and a current time
	 * as far in the past as possible.
	 */
	LeapSeconds();
	/**
	 * Makes a new LeapSeconds object and fills it with the leap seconds from
	 * the indicated zoneinfo database file. readZoneinfo() is called, and any
	 * exception it throws is @b not caught.
	 * @param zoneinfo   The zoneinfo database file.
	 * @sa readZoneinfo()
	 */
	LeapSeconds(const std::string &zoneinfo);
	/**
	 * Reads the indicated zoneinfo database file. If successful, the leap
	 * second data in this object is replaced with the information from the
	 * file. Otherwise, the object's data will not change.
	 *
	 * It is assumed that the file is intended for use with timezones on Earth.
	 * The files include leap seconds from 1972 onward. However, TAI and UTC
	 * had diverged by about 10 seconds before the leap second system was
	 * instituted. Prior to that, UTC was either adjusted daily by thosandths
	 * of a second, or used a varying definition for a second; I'm not sure
	 * which.
	 *
	 * To address this situation, the first leap second record is the start of
	 * an 11 second difference between TAI and UTC. As a result, times before
	 * the very end of 30 June 1972 will be given 0 leap seconds after this
	 * function is successful.
	 *
	 * @param zoneinfo  The zoneinfo database file path. This function works on
	 *                  the binary zoneinfo files that are typically found on
	 *                  Linux systems. The most common zoneinfo files lack leap
	 *                  second information, but those that do contain it
	 *                  typically (always?) have the same leap second data
	 *                  regardless of the described time zone.
	 *                  On Linux, "/usr/share/zoneinfo/right/UTC" (older),
	 *                  "/usr/share/zoneinfo-leaps/UTC", or wherever the
	 *                  distribution puts them, is a good choice.
	 * @return     The number of leap second records read.
	 * @throw ZoneIoError        Failed to open the file.
	 * @throw ZoneDuplicateLeap  Two leap second records have the same time of
	 *                           occurance.
	 * @throw ZoneTruncated      Fewer leap second records were read than the
	 *                           file claims to have.
	 * @bug   The current until time (currUntil) is not changed. Is an
	 *        acceptable time available?
	 */
	int readZoneinfo(const std::string &zoneinfo);
	/**
	 * Sets a timestamp for when the leap second information becomes outdated.
	 * @param when  The time when the data may be outdated. The most correct
	 *              value to use is one that will assure more leap seconds
	 *              will not be added until after @a when.
	 */
	void setCurrent(const duds::time::interstellar::Seconds when);
	/**
	 * Returns the time when the leap second data may no longer be up to date.
	 * This is intended to allow software to tell if the leap second data is
	 * current or outdated. Such a check cannot be done using the leap second
	 * records since they can occur irregularly.
	 */
	duds::time::interstellar::Seconds currentUntil() const;
	/**
	 * Adds new leap second(s) at the given time and adjusts existing records
	 * to match.
	 * @param leapOn      The time when the change of leap seconds will take
	 *                    take place.
	 * @param additional  The number of leap seconds to add.
	 */
	void add(const duds::time::interstellar::SecondTime leapOn,
		const duds::time::interstellar::Seconds additional =
		duds::time::interstellar::Seconds(1)
	);
	/**
	 * Makes a new leap second entry.
	 * @post  The new leap second entry is added, and the existing entries
	 *        remain unchanged. This means the number of leap seconds applied
	 *        for all existing time periods will remain the same.
	 * @param leapOn  The time when the leap second is added.
	 * @param total   The total number of leap seconds up to the point in time
	 *                in @a leapOn.
	 */
	void set(const duds::time::interstellar::SecondTime leapOn,
		const duds::time::interstellar::Seconds total
	);
	/**
	 * Returns the sum of all leap seconds in use at the given time.
	 * @param when  The time to inspect for leap seconds.
	 */
	duds::time::interstellar::Seconds leapSeconds(
		const duds::time::interstellar::SecondTime &when
	) const;
	/*
	 * Returns the sum of all leap seconds in use at the given time.
	 * @param when  The time to inspect for leap seconds.
	 * /
	template <class ISTime>
	typename ISTime::duration leapSeconds(const ISTime &when) const {
		return std::chrono::duration_cast<typename ISTime::duration>(leapSeconds(
			duds::time::interstellar::SecondTime(
				std::chrono::duration_cast<duds::time::interstellar::Seconds>(
					when.time_since_epoch()
				)
			)
		));
	} */
	/*
	template <class ISTime>
	LeapBounds getLeapBounds(const ISTime &time) const {
		return getLeapBounds(
	}
	*/
	/**
	 * Create a LeapBounds object with the data for the time period covered
	 * for the given time
	 * @param time  The time that will be within the bounds of the resulting
	 *              LeapBounds.
	 * @return      A new LeapBounds object.
	 */
	LeapBounds<> getLeapBounds(
		const duds::time::interstellar::SecondTime time
	) const;
	/**
	 * Returns a new shared pointer to the current map of leap seconds. This
	 * allows inspection of all the leap seconds in a mostly thread-safe manner.
	 * To keep it thread-safe, the const modifier must not be violated, and
	 * the map object should not be used during a call to add() or set().
	 * readZoneinfo() may be called without breaking thread-safety, but if it
	 * succeeds, this object will hold a different LeapMap.
	 * @todo  Justify the need or remove this function. Maybe quit using a
	 *        shared pointer, too. Or make a copy in add() and set().
	 */
	shared_ptr<const LeapMap> leapMap() const;
	/**
	 * Returns a copy of the current map of leap seconds. This allows
	 * inspection of all leap second records in a thread-safe manner.
	 */
	LeapMap leapMapCopy() const;
};

} } }

