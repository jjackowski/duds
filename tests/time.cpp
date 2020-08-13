/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * Tests of various classes in duds::time.
 */

#define BOOST_TEST_DYN_LINK
// must be in one file only; creates int main()
#define BOOST_TEST_MODULE DUDS library tests
#include <boost/test/unit_test.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include <duds/time/TimeErrors.hpp>

std::ostream &operator << (
	std::ostream &os,
	const duds::time::planetary::LeapBounds<> &lb
) {
	os << "Bound " << lb.minimum().time_since_epoch().count() << ", " <<
	lb.maximum().time_since_epoch().count() << ",  leaps = " << lb.leaps().count();
	return os;
}

// needed for output by some Boost tests
std::ostream &operator << (
	std::ostream &os,
	const duds::time::interstellar::Seconds &sec
) {
	return os << sec.count() << 's';
}

BOOST_AUTO_TEST_SUITE(LeapSeconds)

BOOST_AUTO_TEST_CASE(ReadLeapsFromZonefile /*,    Boost 1.59 feature?  untested
	* boost::unit_test::::precondition(
		boost::unit_test::framework::master_test_suite().argc > 1
	) */
) {
	// check for a zone file specified as an argument
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		// only test if given; disabling the test needs Boost 1.59
		duds::time::planetary::LeapSeconds ls(boost::unit_test::framework::master_test_suite().argv[1]);
		std::shared_ptr<const duds::time::planetary::LeapSeconds::LeapMap> lm = ls.leapMap();
		BOOST_WARN_GE(lm->size(), 26);
		BOOST_CHECK((lm->size() == 0) || (lm->size() >= 26));
		if (!lm->empty()) {
			duds::time::interstellar::Seconds leaps = ls.leapSeconds(
				duds::time::interstellar::SecondClock::now()
			);
			BOOST_CHECK(leaps >= duds::time::interstellar::Seconds(36));
		} else {
			std::cout << "Zero leap seconds in supplied zoneinfo file." <<
			std::endl;
		}
	} else {
		std::cout << "No zoneinfo file path supplied in arguments; "
		"skipping test of reading a zoneinfo file." << std::endl;
	}
}

BOOST_AUTO_TEST_CASE(ReadLeapsFromNonexistentFile) {
	duds::time::planetary::LeapSeconds ls;
	BOOST_CHECK_THROW(ls.readZoneinfo("hikeeba"),
		duds::time::planetary::ZoneIoError
	);
}

// refactor with test datasets when Boost 1.59 can be used
BOOST_AUTO_TEST_CASE(LeapSecondApplication) {
	duds::time::planetary::LeapSeconds ls;
	// fill in some leap second times -- avoid need for zoneinfo file
	const duds::time::interstellar::SecondTime
		// first four leap seconds
		Jun1972(duds::time::interstellar::Seconds(78796810)),
		Dec1972(duds::time::interstellar::Seconds(94694411)),
		Dec1973(duds::time::interstellar::Seconds(126230412)),
		Dec1974(duds::time::interstellar::Seconds(157766413)),
		// additional times for tesing
		testEarly(duds::time::interstellar::Seconds(42)),
		test1972(duds::time::interstellar::Seconds(88796810)),
		test1974(duds::time::interstellar::Seconds(126231412)),
		testLate(duds::time::interstellar::Seconds(157799913));
	ls.set(Jun1972, duds::time::interstellar::Seconds(11));
	ls.add(Dec1972);
	ls.add(Dec1974);
	ls.add(Dec1973);
	//BOOST_TEST_MESSAGE("Leap map contents check");
	const duds::time::planetary::LeapSeconds::LeapMap lm = ls.leapMapCopy();
	/*
	duds::time::planetary::LeapSeconds::LeapMap::const_iterator iter = lm.begin();
	for (; iter != lm.end(); ++iter) {
		BOOST_TEST_MESSAGE(iter->first.time_since_epoch().count());
		BOOST_TEST_MESSAGE(iter->second.count());
	}
	*/
	BOOST_CHECK_EQUAL(lm.size(), 4);
	//BOOST_TEST_MESSAGE("Bounds check");
	// check leap seconds prior to first leap
	duds::time::planetary::LeapBounds<> lb(ls.getLeapBounds(testEarly));
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == duds::time::planetary::LeapBounds<>::TimePoint::min());
	BOOST_CHECK(lb.maximum() == Jun1972);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(ls.leapSeconds(testEarly) == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(lb.within(testEarly));
	BOOST_CHECK(lb.within(Jun1972));
	BOOST_CHECK(!lb.within(Dec1972));
	// check leap seconds one second before first leap
	lb = ls.getLeapBounds(Jun1972 - duds::time::interstellar::Seconds(1));
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == duds::time::planetary::LeapBounds<>::TimePoint::min());
	BOOST_CHECK(lb.maximum() == Jun1972);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(ls.leapSeconds(Jun1972 - duds::time::interstellar::Seconds(1)) == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(lb.within(testEarly));
	BOOST_CHECK(lb.within(Jun1972));
	BOOST_CHECK(!lb.within(Dec1972));
	// check leap seconds at first leap; leap second not yet applied
	lb = ls.getLeapBounds(Jun1972);
	//std::cout << lb << std::endl;
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == duds::time::planetary::LeapBounds<>::TimePoint::min());
	BOOST_CHECK(lb.maximum() == Jun1972);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(ls.leapSeconds(Jun1972) == duds::time::interstellar::Seconds(0));
	BOOST_CHECK(lb.within(testEarly));
	BOOST_CHECK(lb.within(Jun1972));
	BOOST_CHECK(!lb.within(test1972));
	BOOST_CHECK(!lb.within(Dec1972));
	// check leap seconds one second after first leap
	lb = ls.getLeapBounds(Jun1972 + duds::time::interstellar::Seconds(1));
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == Jun1972);
	BOOST_CHECK(lb.maximum() == Dec1972);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(11));
	BOOST_CHECK(ls.leapSeconds(Jun1972 + duds::time::interstellar::Seconds(1)) == duds::time::interstellar::Seconds(11));
	BOOST_CHECK(!lb.within(testEarly));
	BOOST_CHECK(lb.within(Jun1972 + duds::time::interstellar::Seconds(1)));
	BOOST_CHECK(lb.within(Dec1972));
	BOOST_CHECK(!lb.within(Dec1972 + duds::time::interstellar::Seconds(1)));
	// check leap seconds between first two leap seconds
	lb = ls.getLeapBounds(test1972);
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == Jun1972);
	BOOST_CHECK(lb.maximum() == Dec1972);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(11));
	BOOST_CHECK(ls.leapSeconds(test1972) == duds::time::interstellar::Seconds(11));
	BOOST_CHECK(!lb.within(Jun1972));
	BOOST_CHECK(lb.within(test1972));
	BOOST_CHECK(!lb.within(testEarly));
	BOOST_CHECK(lb.within(Dec1972));
	// check leap seconds on third leap
	lb = ls.getLeapBounds(Dec1973);
	//std::cout << lb << std::endl;
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == Dec1972);
	BOOST_CHECK(lb.maximum() == Dec1973);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(12));
	BOOST_CHECK(ls.leapSeconds(Dec1973) == duds::time::interstellar::Seconds(12));
	BOOST_CHECK(!lb.within(Dec1972));
	BOOST_CHECK(lb.within(Dec1973));
	BOOST_CHECK(!lb.within(test1974));
	BOOST_CHECK(!lb.within(Dec1974));
	// check leap seconds after third leap
	lb = ls.getLeapBounds(test1974);
	//std::cout << lb << std::endl;
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == Dec1973);
	BOOST_CHECK(lb.maximum() == Dec1974);
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(13));
	BOOST_CHECK(ls.leapSeconds(test1974) == duds::time::interstellar::Seconds(13));
	BOOST_CHECK(!lb.within(Dec1973));
	BOOST_CHECK(lb.within(test1974));
	BOOST_CHECK(!lb.within(Dec1972));
	BOOST_CHECK(lb.within(Dec1974));
	// check leap seconds after last leap
	lb = ls.getLeapBounds(testLate);
	//std::cout << lb << std::endl;
	BOOST_CHECK(lb.valid());
	BOOST_CHECK(lb.minimum() == Dec1974);
	BOOST_CHECK(lb.maximum() == duds::time::planetary::LeapBounds<>::TimePoint::max());
	BOOST_CHECK(lb.leaps() == duds::time::interstellar::Seconds(14));
	BOOST_CHECK(ls.leapSeconds(testLate) == duds::time::interstellar::Seconds(14));
	BOOST_CHECK(lb.within(testLate));
	BOOST_CHECK(!lb.within(Dec1974));
	BOOST_CHECK(lb.within(Dec1974 + duds::time::interstellar::Seconds(11)));
	BOOST_CHECK(!lb.within(testEarly));
	BOOST_CHECK(!lb.within(test1972));
}

BOOST_AUTO_TEST_SUITE_END()
