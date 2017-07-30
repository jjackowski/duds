/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef POSIXCLOCKDRIVER_HPP
#define POSIXCLOCKDRIVER_HPP

#include <duds/hardware/devices/clocks/Clock.hpp>
#include <time.h>

#ifndef CLOCK_TAI
/**
 * Present in Linux 3.10 kernels, but not always in the header files used
 * to build software even on systems with the 3.10 kernel. Defined here to
 * be the same as in the kernel. Attempts to use this with older kernels
 * will result in a run-time error.
 */
#define CLOCK_TAI 11
#endif

namespace duds { namespace hardware { namespace devices { namespace clocks {

/**
 * An attempt was made to use a POSIX clock that is not supported by the
 * system.
 */
struct PosixClockUnsupported : ClockError { };

/**
 * Indicates the POSIX clockid_t in an error involving a POSIX clock.
 */
typedef boost::error_info<struct tag_posixclockid, clockid_t>  PosixClockId;

/**
 * A clock driver to use clocks through the POSIX interface.
 * Not all POSIX clocks are suitable to provide the time. For example,
 * CLOCK_MONOTONIC cannot be related to the time without additional
 * information.
 * @warning  Linux uses an offset between UTC and TAI, and computes TAI from
 *           UTC. The default value of this offset is @b zero and will result
 *           in TAI being the same as UTC, which is incorrect. The Linux
 *           specific adjtimex() function queries the current offset value. On
 *           most Linux systems, unless a system administrator has taken
 *           action to assure the offset is set, then it probably is not set.
 *           It seems most distributions do nothing to set the offset.
 *
 * @tparam SVT  Sample value type.
 * @tparam SQT  Sample quality type.
 * @tparam TVT  Time value type.
 * @tparam TQT  Time quality type.
 *
 * @author Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericPosixClockDriver : public GenericClockDriver<SVT, SQT, TVT, TQT> {
public:
	// copied from base class; cannot use from derived classes without scope
	/** @copydoc GenericInstrumentDriver::Adapter */
	typedef duds::hardware::GenericInstrumentAdapter<SVT, SQT, TVT, TQT>
		Adapter;
	/** @copydoc GenericInstrumentDriver::Measurement */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
private:
	/**
	 * The clock's resolution for use with samples.
	 * Another value is used for the time stamp to avoid more than one
	 * convertion.
	 */
	SQT sampleResolution;
	/**
	 * The clock's resolution for use with time stamps.
	 * Another value is used for the sample to avoid more than one convertion.
	 */
	TQT timeResolution;
	/**
	 * An offset in seconds that is applied to the time provided by the clock.
	 */
	int offset;
	
	// are seconds ok? maybe TVT would work better. might work better for using
	// CLOCK_MONOTONIC, although it cannot be corrected over time.
	// for leap seconds, store a duds::time::interstellar::SecondTime with when
	// leap seconds should be inspected again. when now becomes that time value,
	// query for leap seconds, update the offset and the time to do this again.
	// maybe don't use duds::time::interstellar::SecondTime; use something
	// in the form of the data from the clock for a faster comparison.
	
	/**
	 * The POSIX clock id.
	 */
	clockid_t clk;
	/**
	 * Sums the second and nanosecond fields given by the POSIX functions into
	 * a large integer.
	 * @param ts  The result from one of the POSIX clock functions.
	 * @return    The combined value in nanoseconds.
	 */
	static duds::data::int128_t sum(const timespec &ts) {
		return duds::data::int128_t(
			duds::data::int128_t(ts.tv_sec) * std::nano::den +
			duds::data::int128_t(ts.tv_nsec)
		);
	}
	/**
	 * Samples the time and adds in an offset.
	 * @return  The time in nanoseconds.
	 * @throw   ClockError  The clock failed to be sampled.
	 */
	duds::data::int128_t doSample() const {
		timespec ts;
		int res = clock_gettime(clk, &ts);
		if (res) {
			BOOST_THROW_EXCEPTION(ClockError() << PosixClockId(clk));
		}
		ts.tv_sec += offset;
		return sum(ts);
	}
	/**
	 * Sets the sample values based on the time sample.
	 * @tparam A GenericSample class,
	 * @param dest  The Sample object that will be the destination for the
	 *              time.
	 * @param time  The time in nanoseconds.
	 */
	template <class Sample>
	void setSample(Sample &dest, const duds::data::int128_t &time) const {
		// two resolutions are stored; select one based on type
		if (std::is_same<SQT, typename Sample::Quality>::value) {
			dest.resolution = sampleResolution;
		} else {
			dest.resolution = timeResolution;
		}
		// not all quality values are reported
		dest.accuracy = dest.estError = dest.precision =
			duds::data::unspecified<typename Sample::Quality>();
		// set the time
		GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::nano>(
			dest.value, time);
	}
public:
	/**
	 * Construct a clock driver for the given POSIX clock.
	 * @param id  The ID number for the POSIX clock.
	 * @param os  An offset in seconds that will be applied to the time provdied
	 *            by this object.
	 * @warning   On Linux, CLOCK_TAI may provide UTC. See the warning in the
	 *            detail documentation of this class for more information.
	 * @throw PosixClockUnsupported  The specified clock is not supported by
	 *                               the system.
	 */
	GenericPosixClockDriver(clockid_t id = CLOCK_REALTIME, int os = 0) :
	clk(id), offset(os) {
		// query the clock for its resolution
		timespec ts;
		int res = clock_getres(clk, &ts);
		if (res) {
			BOOST_THROW_EXCEPTION(PosixClockUnsupported() << PosixClockId(clk));
		}
		// put the resolution into a single value
		duds::data::int128_t rez = sum(ts);
		// convert the resolution into the two quality types found
		// in a Measurement 
		GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::nano>(
			sampleResolution, rez);
		GenericClockDriver<SVT, SQT, TVT, TQT>::template convert<std::nano>(
			timeResolution, rez);
	}
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
		duds::data::int128_t sum = doSample();
		setSample(time, sum);
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
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		duds::data::int128_t sum = doSample();
		setSample(m->measured, sum);
		// if the supplied clock driver is this clock driver . . .
		if (this == &clock) {
			// sample the clock just once
			setSample(m->timestamp, sum);
		} else {
			// sample the other clock
			clock.sampleTime(m->timestamp);
		}
		// send out the measurement
		GenericClockDriver<SVT, SQT, TVT, TQT>::adp->signalMeasurement(m);
	}
	virtual bool unambiguous() const noexcept {
		return false;
	}
};

/**
 * General use POSIX clock driver type.
 */
typedef GenericPosixClockDriver<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  PosixClockDriver;

} } } }


#endif        //  #ifndef POSIXCLOCKDRIVER_HPP
