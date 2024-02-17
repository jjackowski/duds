/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CPPCLOCK_HPP
#define CPPCLOCK_HPP

#include <duds/hardware/devices/clocks/Clock.hpp>

namespace duds { namespace hardware { namespace devices { namespace clocks {

/**
 * The UUID for the C++ clock device.
 */
constexpr boost::uuids::uuid CppClockDeviceId = {
	0x7f, 0x3a, 0x9f, 0x9a,
	0x82, 0x59,
	0x43, 0xde,
	0x97, 0xac,
	0xd2, 0xea, 0x2a, 0x48, 0x51, 0xb2
};

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
class GenericCppClock :
public duds::hardware::devices::clocks::GenericClock<SVT, SQT, TVT, TQT> {
public:
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	/**
	 * The clock class.
	 */
	typedef CLK Clock;
protected:
	using duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>::sens;
	using duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>::setMeasurement;
	struct Token { };
public:
	/**
	 * Constructs a new clock device with its UUID.
	 * @private
	 */
	GenericCppClock(Token) :
	GenericClock<SVT, SQT, TVT, TQT>(CppClockDeviceId) { }
	/**
	 * Makes a new clock device object.
	 */
	static std::shared_ptr< GenericCppClock <CLK, SVT, SQT, TVT, TQT> > make() {
		return std::make_shared< GenericCppClock <CLK, SVT, SQT, TVT, TQT> >(
			Token()
		);
	}
	virtual void sampleTime(typename Measurement::TimeSample &time) {
		time.accuracy = time.precision = time.estError =
			duds::data::unspecified<TQT>();
		time.resolution = (TQT)Clock::period::num / (TQT)Clock::period::den;
		time.value = Clock::now();
	}
	virtual void sample() {
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		m->measured.accuracy = m->measured.precision = m->measured.estError =
			duds::data::unspecified<SQT>();
		m->measured.resolution = (SQT)Clock::period::num / (SQT)Clock::period::den;
		typename Clock::time_point ctp = Clock::now();
		m->measured.value = ctp;
		// no timestamp
		m->timestamp.clear();
		// store the measurement
		setMeasurement(std::move(m));
	}
	virtual void sample(const ClockSptr &clock) {
		std::shared_ptr<Measurement> m =
			std::make_shared<Measurement>();
		m->measured.accuracy = m->measured.precision = m->measured.estError =
			duds::data::unspecified<SQT>();
		m->measured.resolution = (SQT)Clock::period::num / (SQT)Clock::period::den;
		typename Clock::time_point ctp = Clock::now();
		m->measured.value = ctp;
		// if the supplied clock driver is this clock driver . . .
		if (this == clock.get()) {
			// sample the clock just once
			m->timestamp.value = ctp;
				//std::chrono::time_point_cast
				//<typename TVT::duration>(ctp);
			m->timestamp.accuracy = m->timestamp.precision =
				m->timestamp.estError = duds::data::unspecified<TQT>();
			m->timestamp.resolution = (TQT)Clock::period::num /
				(TQT)Clock::period::den;
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
		return false;
	}
};

/**
 * General use C++ clock driver type.
 */
typedef GenericCppClock<
	duds::time::interstellar::NanoClock,  // could be FemtoClock
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  CppClock;

typedef std::shared_ptr<CppClock>  CppClockSptr;

} } } }


#endif        //  #ifndef CPPCLOCK_HPP
