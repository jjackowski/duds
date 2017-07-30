/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef LINUXCLOCKDRIVER_HPP
#define LINUXCLOCKDRIVER_HPP

#include <duds/hardware/devices/clocks/Clock.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include <sys/timex.h>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <errno.h>

namespace duds { namespace hardware { namespace devices { namespace clocks {

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
class GenericLinuxClockDriver :
public duds::hardware::devices::clocks::GenericClockDriver<SVT, SQT, TVT, TQT> {
public:
	// copied from base class; cannot use from derived classes
	/** @copydoc GenericInstrumentDriver::Adapter */
	typedef duds::hardware::GenericInstrumentAdapter<SVT, SQT, TVT, TQT>
		Adapter;
	/** @copydoc GenericInstrumentDriver::Measurement */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
private:
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
	static void setSample(Sample &dest, const timex &src,
		const duds::data::int128_t &time
	) {
		// check for an unsync'ed clock
		if (src.status & STA_UNSYNC) {
			// no clue if the time is right
			dest.accuracy = dest.estError =
				duds::data::unspecified<typename Sample::Quality>();
		} else {
			// provide info on the correctness of the time
			GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.accuracy, src.maxerror);
			GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.estError, src.esterror);
		}
		// additional quality data
		GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::micro>(
			dest.precision, src.precision);
		dest.resolution = duds::data::unspecified<typename Sample::Quality>();
		// the time
		if (src.status & STA_NANO) {
			GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::nano>(
				dest.value, time);
		} else {
			GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::micro>(
				dest.value, time);
		}
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
			BOOST_THROW_EXCEPTION(ClockError() <<
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
public:
	/*
	virtual void setAdapter(const std::shared_ptr<Adapter> &a) {
		adp = a;
		adp->setUnit(duds::data::units::Second);
		// setting part is probably best done in a derived class
		//adp->setPart(SystemClockPart);
	}
	*/
	/**
	 * Samples the time from the clock device without triggering a new
	 * measurement event.
	 * @param time  The place to put the sampled time.
	 */
	virtual void sampleTime(typename Measurement::TimeSample &time) {
		timex tx;
		duds::data::int128_t total;
		doSample(tx, total);
		setSample(time, tx, total);
	}
	/**
	 * Samples the time from this clock and the given clock, then sends the
	 * measurement event. The sample from this clock will be in the @a measured
	 * field of the @a Measurement object.
	 * @param clock  The clock that will be sampled for the timestamp in the
	 *               resulting measurement. If it is this clock, the clock will
	 *               only be sampled once and the same time will be in both the
	 *               @a measured and @a timestamp fields of the @a Measurement
	 *               object. Different types may be used to hold the time in
	 *               those fields so they might not evaluate as equal.
	 */
	virtual void sample(ClockDriver &clock) {
		timex tx;
		duds::data::int128_t total;
		doSample(tx, total);
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		setSample(m->measured, tx, total);
		// if the supplied clock driver is this clock driver . . .
		if (this == &clock) {
			setSample(m->timestamp, tx, total);
		} else {
			// sample the other clock
			clock.sampleTime(m->timestamp);
		}
		// send out the measurement
		// protected members of the parent class are not in this scope
		GenericClockDriver<SVT, SQT, TVT, TQT>::adp->signalMeasurement(m);
	}
	virtual bool unambiguous() const noexcept {
		return true;
	}
};

/**
 * General use Linux clock driver type.
 */
typedef GenericLinuxClockDriver<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  LinuxClockDriver;

} } } }


#endif        //  #ifndef LINUXCLOCKDRIVER_HPP
