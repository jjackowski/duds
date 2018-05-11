/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <duds/time/interstellar/Metricform.hpp>
#include <duds/time/interstellar/Hectoform.hpp>
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include <duds/hardware/devices/clocks/PosixClockDriver.hpp>
#include <duds/hardware/devices/clocks/CppClockDriver.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <ratio>
//#include <sys/time.h>

struct beuint32_t {
	std::uint32_t val;
};

std::istream &operator >> (std::istream &is, beuint32_t &bei) {
	std::uint8_t buf[4];
	is.read((char*)buf, 4);
	bei.val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return is;
}

std::ostream &operator << (std::ostream &os, const beuint32_t &bei) {
	os << bei.val;
	return os;
}

struct beuint64_t {
	std::uint64_t val;
};

std::istream &operator >> (std::istream &is, beuint64_t &bei) {
	std::uint8_t buf[8];
	is.read((char*)buf, 8);
	bei.val = ((std::uint64_t)buf[0] << 56) | ((std::uint64_t)buf[1] << 48) |
		((std::uint64_t)buf[2] << 40) | ((std::uint64_t)buf[3] << 32) |
		((std::uint64_t)buf[4] << 24) | ((std::uint64_t)buf[5] << 16) |
		((std::uint64_t)buf[6] << 8) | (std::uint64_t)buf[7];
	return is;
}

std::ostream &operator << (std::ostream &os, const beuint64_t &bei) {
	os << bei.val;
	return os;
}

namespace IST = duds::time::interstellar;

void timeout(const duds::time::planetary::LeapSeconds &ls, std::int64_t t,
const IST::SecondTime &ist) {
	IST::Metricform m(ist);
	IST::Hectoform e(ist);
	IST::Seconds leaps = ls.leapSeconds(ist);
	std::cout << "Time " << std::right << std::setw(16) << t << ": " << m <<
		"   " << e << "  Leap seconds: " << leaps.count() << std::endl;
}

using duds::time::planetary::earth;

// best results from  /usr/share/zoneinfo-leaps/UTC
// seems to be the same from all under  /usr/share/zoneinfo-leaps
int main(int argc, char *argv[])
try {
	const char *zfile = (argc > 1) ? argv[1] : "/usr/share/zoneinfo-leaps/UTC";
	std::ifstream zf(zfile,
		std::ios_base::in | std::ios_base::binary);
	if (!zf.is_open()) {
		std::cerr << "Failed to open " << zfile << std::endl;
		return 1;
	}
	zf.seekg(20 + 4 * 2);
	beuint32_t lsc, lstime, numls, transt, ltt, abr;
	zf >> lsc >> transt >> ltt >> abr;
	zf.seekg(transt.val * 5 + ltt.val * 6 + abr.val, std::ios_base::cur);
	std::cout << "Number of leap second entries: " << lsc.val << std::endl;
	boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
	for (; zf.good() && (lsc.val > 0); --lsc.val) {
		beuint32_t when, count;
		// when will store the time when the leap second is added
		zf >> when >> count;
		IST::Seconds secs(when.val);
		IST::SecondTime ist(secs);
		IST::Metricform m(ist);
		IST::Hectoform e(ist);

		IST::Milliseconds tm(secs);
		IST::Femtoseconds fs(secs);

		std::cout << lsc.val << ":\t" << count.val + 10 << " \t " << when.val <<
		"\t " << (epoch + boost::posix_time::seconds(when.val - count.val -1)).date()
		<< " \t  " << m << "   " << e << std::endl;
	}
	zf.close();
	std::cout << "ptime size = " << sizeof(boost::posix_time::ptime) << std::endl;
	std::cout << "Metricform size = " << sizeof(IST::Metricform) << std::endl;
	std::cout << "Hectoform size = " << sizeof(IST::Hectoform) << std::endl;
	std::cout << "Femtoseconds size = " << sizeof(IST::Femtoseconds) << std::endl;
	std::cout << "Milliseconds size = " << sizeof(IST::Milliseconds) << std::endl;
	earth = std::unique_ptr<duds::time::planetary::Earth>(
		new duds::time::planetary::Earth()
	);
	duds::time::planetary::LeapSeconds &ls = earth->leaps;
	if ((argc > 1) /* && (argv[1][0] == '/') */) {
		ls.readZoneinfo(argv[1]);
	} else {
		ls.readZoneinfo("/usr/share/zoneinfo-leaps/UTC");
	}
	for (int i = 2; i < argc; ++i) {
		if (argv[i][0] != '/') {
			std::istringstream iss(argv[i]);
			std::int64_t t;
			iss >> t;
			IST::Seconds sec(t); // cannot combine this line with the next
			IST::SecondTime ist(sec);
			timeout(ls, t, ist);
		}
	}

	//tzset();

	timeout(ls, 0, IST::SecondClock::now());
	duds::hardware::devices::clocks::LinuxClockDriver lcd;
	duds::hardware::devices::clocks::PosixClockDriver pcd(CLOCK_TAI), rtcd(CLOCK_REALTIME);
	duds::hardware::devices::clocks::CppClockDriver ccd;
	duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample lts, pts, cts, rtts;
	lcd.sampleTime(lts);
	pcd.sampleTime(pts);
	ccd.sampleTime(cts);
	rtcd.sampleTime(rtts);
	timeout(ls, 0, lts.value);
	timeout(ls, 0, pts.value);
	timeout(ls, 0, cts.value);
	timeout(ls, 0, rtts.value);
	boost::posix_time::ptime pt;
	pt = earth->posix(lts.value);  // should be in UTC
	std::cout << pt << "  <-- lts UTC" << std::endl;
	pt = duds::time::planetary::ToPosix(lts.value);
	std::cout << pt << "  <-- lts TAI" << std::endl;
	pt = earth->posix(pts.value);  // should be wrong (UTC - leaps)
	std::cout << pt << "  <-- wrong" << std::endl;
	pt = duds::time::planetary::ToPosix(pts.value);  // should be in UTC
	std::cout << pt << std::endl;
	pt = duds::time::planetary::ToPosix(cts.value);
	std::cout << pt << std::endl;
	pt = duds::time::planetary::ToPosix(rtts.value);
	std::cout << pt << std::endl;
	pt = earth->timeZero() + boost::posix_time::milliseconds(  // UTC
		IST::MilliTime(rtts.value).time_since_epoch().count());
	std::cout << pt << std::endl;

	//tzset();
	//std::cout << "timezone = " << timezone << std::endl;
	tm testm;
	std::time_t tt = (std::time_t)std::chrono::duration_cast<std::chrono::seconds>(
		lts.value.time_since_epoch()).count();
	localtime_r(&tt, &testm);
	std::cout << "localtime T " << std::setfill(' ') << std::setw(2) << std::right <<
	testm.tm_hour << ':' << std::setfill('0') << std::setw(2) << testm.tm_min <<
	':' << std::setw(2) << testm.tm_sec << " in zone " << testm.tm_zone <<
	" (TAI referenced)" << std::endl;
	tt = earth->timeUtc(lts.value);
	localtime_r(&tt, &testm);
	std::cout << "localtime L " << std::setfill(' ') << std::setw(2) <<
	testm.tm_hour << ':' << std::setfill('0') << std::setw(2) << testm.tm_min <<
	':' << std::setw(2) << testm.tm_sec << " in zone " << testm.tm_zone <<
	std::endl;
	std::cout << "tm_gmtoff = " << testm.tm_gmtoff << std::endl;

	return 0;
}
catch (...) {
	std::cerr << "ERROR: " << boost::current_exception_diagnostic_information()
	<< std::endl;
	return __LINE__;
}

