/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/time/planetary/LeapSeconds.hpp>
#include <duds/time/TimeErrors.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <fstream>
#include <utility>
#include <mutex>

namespace duds { namespace time { namespace planetary {

/**
 * @internal
 * Used to allow a stream extraction operator to read binary big endian
 * numbers.
 */
struct beuint32_t {
	std::uint32_t val;
};

/**
 * @internal
 * Reads a big endian unsigned 32-bit integer as binary data and converts it
 * to host byte order.
 */
std::istream &operator >> (std::istream &is, beuint32_t &bei) {
	std::uint8_t buf[4];
	is.read((char*)buf, 4);
	bei.val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return is;
}

LeapSeconds::LeapSeconds() : leaps(std::make_shared<LeapMap>()),
currUntil(0 /* minimum value, 1 << 63 */) { }

LeapSeconds::LeapSeconds(const std::string &zonefile) : currUntil(0) {
	readZoneinfo(zonefile);
}

int LeapSeconds::readZoneinfo(const std::string &zonefile) {
	std::ifstream zf(zonefile, std::ios_base::in | std::ios_base::binary);
	if (!zf.is_open()) {
		DUDS_THROW_EXCEPTION(ZoneIoError() <<
			boost::errinfo_file_name(zonefile));
	}
	// seek to the leap second records
	zf.seekg(20 + 4 * 2);
	// lsc=leap second count  lstime=leap second time  ltt=leap time total
	beuint32_t lsc, lstime, numls, transt, ltt, abr;
	zf >> lsc >> transt >> ltt >> abr;
	if (!zf.good()) {
		DUDS_THROW_EXCEPTION(ZoneIoError() <<
			boost::errinfo_file_name(zonefile));
	}
	zf.seekg(transt.val * 5 + ltt.val * 6 + abr.val, std::ios_base::cur);
	shared_ptr<LeapMap> ls(std::make_shared<LeapMap>());
	// loop through all the leap seconds
	for (; zf.good() && (lsc.val > 0); --lsc.val) {
		beuint32_t when, count;
		// when will store the time when the leap second is added
		zf >> when >> count;
		// store the leap second; account for ten prior leap seconds
		std::pair<LeapMap::iterator, bool> res = ls->emplace(std::make_pair(
			duds::time::interstellar::SecondTime(
				duds::time::interstellar::Seconds(when.val + 10)
			), duds::time::interstellar::Seconds(count.val + 10)));
		// check for failure to add
		if (!res.second) {
			DUDS_THROW_EXCEPTION(ZoneDuplicateLeap() <<
				boost::errinfo_file_name(zonefile));
		}
	}
	// too few leap seconds read from file?
	if (lsc.val) {
		DUDS_THROW_EXCEPTION(ZoneTruncated() <<
			boost::errinfo_file_name(zonefile));
	}
	// keep parsed leap seconds
	std::lock_guard<duds::general::Spinlock> lock(block);
	leaps.swap(ls);
	return leaps->size();
}

void LeapSeconds::setCurrent(const duds::time::interstellar::Seconds when) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	currUntil = when;
}

duds::time::interstellar::Seconds LeapSeconds::currentUntil() const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	return currUntil;
}

void LeapSeconds::add(
	const duds::time::interstellar::SecondTime leapOn,
	const duds::time::interstellar::Seconds additional
) {
	// may run a bit long for a spinlock, but add() shouldn't be called often
	std::lock_guard<duds::general::Spinlock> lock(block);
	// no leap seconds yet?
	if (leaps->empty()) {
		// add the first
		leaps->emplace(std::make_pair(leapOn, additional));
	} else {
		// find the value that is >= leapOn
		LeapMap::iterator iter = leaps->lower_bound(leapOn);
		// add to the end?
		if (iter == leaps->end()) {
			--iter;
			leaps->emplace(std::make_pair(leapOn, iter->second + additional));
		// duplicate?
		} else if (iter->first == leapOn) {
			DUDS_THROW_EXCEPTION(DuplicateLeapSecond());
		// add to somewhere in the middle?
		} else {
			std::pair<LeapMap::iterator, bool> res =
				leaps->insert(LeapMap::value_type(leapOn, iter->second));
			// increment leap count on all entries that follow the insertion
			for (; iter != leaps->end(); ++iter) {
				iter->second += additional;
			}
		}
	}
}

void LeapSeconds::set(
	const duds::time::interstellar::SecondTime leapOn,
	const duds::time::interstellar::Seconds total
) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	// add the leap second record without modifying existing records
	leaps->emplace(std::make_pair(leapOn, total));
}

duds::time::interstellar::Seconds LeapSeconds::leapSeconds(
	const duds::time::interstellar::SecondTime &when
) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	LeapMap::const_iterator iter = leaps->lower_bound(when);
	if (iter == leaps->begin()) {
		return duds::time::interstellar::Seconds(0);
	}
	--iter;
	return iter->second;
}

LeapBounds<> LeapSeconds::getLeapBounds(
	const duds::time::interstellar::SecondTime time
) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	LeapMap::const_iterator iter = leaps->lower_bound(time);
	if (iter == leaps->begin()) {
		return LeapBounds<>(duds::time::interstellar::SecondTime::min(),
			iter->first, duds::time::interstellar::Seconds(0)
		);
	}
	if (iter == leaps->end()) {
		--iter;
		return LeapBounds<>(iter->first,
			duds::time::interstellar::SecondTime::max(),
			iter->second
		);
	}
	duds::time::interstellar::SecondTime max = iter->first;
	--iter;
	return LeapBounds<>(iter->first, max, iter->second);
}

shared_ptr<const LeapSeconds::LeapMap> LeapSeconds::leapMap() const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	return shared_ptr<LeapMap>(leaps);
}

LeapSeconds::LeapMap LeapSeconds::leapMapCopy() const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	return *leaps;
}

} } }
