/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CPPCLOCKDRIVER_HPP
#define CPPCLOCKDRIVER_HPP

#include <duds/hardware/devices/clocks/Clock.hpp>

namespace duds { namespace hardware { namespace devices { namespace clocks {

/**
 * The clock driver for C++ clocks that meet the requirements of the
 * [TrivialClock](http://en.cppreference.com/w/cpp/concept/TrivialClock)
 * concept. This concept only provides the time. It does not provide any
 * information on the quality of the time, so this driver also only provides
 * the time. The resulting time should be considered no better than a wild
 * guess.
 *
 * @tparam CLK  The clock class; it must be a TrivialClock.
 * @tparam SVT  Sample value type.
 * @tparam SQT  Sample quality type.
 * @tparam TVT  Time value type.
 * @tparam TQT  Time quality type.
 *
 * @author Jeff Jackowski
 */
template<class CLK, class SVT, class SQT, class TVT, class TQT>
class GenericCppClockDriver :
public duds::hardware::devices::clocks::GenericClockDriver<SVT, SQT, TVT, TQT> {
public:
	// copied from base class; cannot use from derived classes
	/** @copydoc GenericInstrumentDriver::Adapter */
	typedef duds::hardware::GenericInstrumentAdapter<SVT, SQT, TVT, TQT>
		Adapter;
	/** @copydoc GenericInstrumentDriver::Measurement */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	/**
	 * The clock class.
	 */
	typedef CLK Clock;
protected:
	/*
	 * The clock's accuracy; this value is not supplied by the C++ interface.
	 * @todo  Assure accuracy is what is ment; clocks are a bit different.
	 */
	//SQT sampleAccuracy;
	//TQT timeAccuracy;
	/* The following seemed like a more elegant solution, but I can't get it
	   to work.
	template <class Value>
	void setValue(Value &value, const typename CLK::time_point &time) {
		value = typename Value(
			std::chrono::duration_cast<typename Value::duration>(
				time.time_since_epoch())
		);
	}
	//template <>
	void setValue(duds::data::GenericValue &value,
		const typename CLK::time_point &time
	) {
		value = duds::time::interstellar::FemtoTime(
			std::chrono::time_point_cast<
				duds::time::interstellar::FemtoTime,
				duds::time::interstellar::FemtoClock,
				typename CLK::duration
			>(
				time
			)
		);
	}
	template <class Sample>
	void setSample(Sample &samp, const typename CLK::time_point &time) {
		samp.resolution = (typename Sample::Quality)Clock::period::num /
			(typename Sample::Quality)Clock::period::den;
		samp.accuracy = samp.precision = samp.estError =
			duds::data::unspecified<typename Sample::Quality>();
		setValue(samp.value, time);
	}
	*/
public:
	//GenericCppClockDriver()  { }
	/**
	 * Samples the time from the clock device without triggering a new
	 * measurement event.
	 * @param time  The place to put the sampled time.
	 */
	virtual void sampleTime(typename Measurement::TimeSample &time) {
		/* part of the more elegant way that doesn't compile.
		typename CLK::time_point now = Clock::now();
		setSample(time, now);
		*/
		time.accuracy = time.precision = time.estError =
			duds::data::unspecified<TQT>();
		time.resolution = (TQT)Clock::period::num / (TQT)Clock::period::den;
		time.value = Clock::now();
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
		/* part of the more elegant way that doesn't compile.
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		typename CLK::time_point now = Clock::now();
		setSample(m->measured, now);
		// if the supplied clock driver is this clock driver . . .
		if (this == &clock) {
			// sample the clock just once
			setSample(m->timestamp, now);
		} else {
			// sample the other clock
			clock.sampleTime(m->timestamp);
		}
		// send out the measurement
		GenericClockDriver<SVT, SQT, TVT, TQT>::adp->recordMeasurement(m);
		*/
		// make a new Measurement, then fill it
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		m->measured.accuracy = m->measured.precision = m->measured.estError =
			duds::data::unspecified<SQT>();
		m->measured.resolution = (SQT)Clock::period::num / (SQT)Clock::period::den;
		typename Clock::time_point ctp = Clock::now();
		m->measured.value = ctp;
		// if the supplied clock driver is this clock driver . . .
		if (this == &clock) {
			// sample the clock just once
			m->timestamp.value = ctp;
				//std::chrono::time_point_cast
				//<typename TVT::duration>(ctp);
			m->timestamp.accuracy = m->timestamp.precision =
				m->timestamp.estError = duds::data::unspecified<TQT>();
			m->timestamp.resolution = (TQT)Clock::period::num /
				(TQT)Clock::period::den;
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
 * General use C++ clock driver type.
 */
typedef GenericCppClockDriver<
	duds::time::interstellar::NanoClock,  // could be FemtoClock
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  CppClockDriver;

} } } }


#endif        //  #ifndef CPPCLOCKDRIVER_HPP
