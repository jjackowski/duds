/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef LINUXCLOCK_HPP
#define LINUXCLOCK_HPP

#include <duds/hardware/devices/clocks/Clock.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include <sys/timex.h>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <errno.h>

namespace duds { namespace hardware { namespace devices { namespace clocks {

/**
 * The UUID for the Linux clock device.
 */
constexpr boost::uuids::uuid LinuxClockDeviceId = {
	0xbf, 0x2d, 0x4a, 0x68,
	0x62, 0xda,
	0x45, 0x56,
	0x8c, 0xc6,
	0x38, 0xd1, 0xd5, 0x5b, 0x20, 0x74
};

/**
 * Uses the Linux specific adjtimex() function to query for the time and the
 * quailty of the time sample. When the clock is updated and synchronized to
 * an external source using such things as NTP, GPS, or PTP, quality
 * information is supplied through adjtimex() without the caller needing to
 * know the underlying source. Quailty values will be duds::data::unspecified
 * if the clock is not synchronized, and filled with values supplied by
 * adjtimex() otherwise.
 *
 * @bug  Jitter and stability are ignored and not reported.
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author  Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericLinuxClock :
public duds::hardware::devices::clocks::GenericClock<SVT, SQT, TVT, TQT> {
public:
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
private:
	using duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>::sens;
	using duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>::setMeasurement;
	/**
	 * Takes a partially converted time from adjtimex(), completes the
	 * conversion, and places it in the destination along with sample quality
	 * data.
	 * @tparam Sample  A GenericSample type.
	 * @param  dest    The place to store the converted time and quailty data.
	 * @param  src     The result from calling adjtimex(). Its status flags
	 *                 are required here.
	 * @param  time    The time in either microseconds or nanoseconds.
	 */
	template <class Sample>
	void setSample(Sample &dest, const timex &src,
		const duds::data::int128_t &time
	) {
		// check for an unsync'ed clock
		if (src.status & STA_UNSYNC) {
			// no clue if the time is right
			dest.accuracy = dest.estError =
				duds::data::unspecified<typename Sample::Quality>();
		} else {
			// provide info on the correctness of the time
			GenericClock<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.accuracy, src.maxerror);
			GenericClock<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.estError, src.esterror);
		}
		// additional quality data
		GenericClock<SVT, SQT, TVT, TQT>::template convert<std::micro>(
			dest.precision, src.precision);
		dest.resolution = duds::data::unspecified<typename Sample::Quality>();
		// the time
		if (src.status & STA_NANO) {
			GenericClock<SVT, SQT, TVT, TQT>::template convert<std::nano>(
				dest.value, time);
		} else {
			GenericClock<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.value, time);
		}
		// the origin
		dest.origin = sens[0]->uuid();
	}
	/**
	 * Samples the time by calling adjtimex() and computes the time in either
	 * microseconds or nanoseconds (whatever adjtimex() uses) and puts it into
	 * a single number (provided in two).
	 * @param tx    The place to put the result from adjtimex().
	 * @param time  The value that will hold the time as a single number.
	 */
	static void doSample(timex &tx, duds::data::int128_t &time) {
		// request the time
		tx.modes = 0;  // no time adjustment
		int res = adjtimex(&tx);
		// check for an error
		if (res < 0) {
			DUDS_THROW_EXCEPTION(ClockError() <<
				boost::errinfo_api_function("adjtimex") <<
				boost::errinfo_errno(errno)
			);
		}
		// have TAI offset?
		if (tx.tai) {
			// convert to TAI for a proper IST result
			tx.time.tv_sec += tx.tai;
		} else if (duds::time::planetary::earth) {
			// attempt to add leap seconds from another source
			tx.time.tv_sec += duds::time::planetary::earth->leaps.leapSeconds(
				duds::time::interstellar::Seconds(tx.time.tv_sec)
			).count();
		} // else, time is in UTC, leap seconds are unknown, so cannot make TAI
		// compute the time; make it a single value
		time = tx.time.tv_usec;
		if (tx.status & STA_NANO) {
			time += tx.time.tv_sec * std::nano::den;
		} else {
			time += tx.time.tv_sec * std::micro::den;
		}
	}
	struct Token { };
public:
	/**
	 * Constructs a new clock device with its UUID.
	 * @private
	 */
	GenericLinuxClock(Token) :
	GenericClock<SVT, SQT, TVT, TQT>(LinuxClockDeviceId) { }
	/**
	 * Makes a new clock device object.
	 */
	static std::shared_ptr< GenericLinuxClock <SVT, SQT, TVT, TQT> > make() {
		return std::make_shared< GenericLinuxClock <SVT, SQT, TVT, TQT> >(Token());
	}
	virtual void sampleTime(typename Measurement::TimeSample &time) {
		timex tx;
		duds::data::int128_t total;
		doSample(tx, total);
		setSample(time, tx, total);
	}
	virtual void sample() {
		timex tx;
		duds::data::int128_t total;
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		doSample(tx, total);
		setSample(m->measured, tx, total);
		// no timestamp
		m->timestamp.clear();
		// store the measurement
		setMeasurement(std::move(m));
	}
	virtual void sample(const ClockSptr &clock) {
		timex tx;
		duds::data::int128_t total;
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		doSample(tx, total);
		setSample(m->measured, tx, total);
		// if the supplied clock driver is this clock driver . . .
		if (this == clock.get()) {
			// use the same time
			setSample(m->timestamp, tx, total);
		} else if (clock) {
			// sample the other clock
			clock->sampleTime(m->timestamp);
		} else {
			// no timestamp
			m->timestamp.clear();
		}
		// store the measurement
		setMeasurement(std::move(m));
	}
	virtual bool unambiguous() const noexcept {
		return true;
	}
};

/**
 * General use Linux clock driver type.
 */
typedef GenericLinuxClock<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  LinuxClock;

typedef std::shared_ptr<LinuxClock>  LinuxClockSptr;

} } } }


#endif        //  #ifndef LINUXCLOCK_HPP
