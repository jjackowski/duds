/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <duds/hardware/devices/Device.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <boost/uuid/name_generator.hpp>

namespace duds { namespace hardware { namespace devices {

/**
 * The place for clock drivers; a way to use time keeping devices.
 * Usually there is a way to query the operating system for the time, so many
 * classes will not require hardware support.
 */
namespace clocks {

// ___________________________________________________________________________
// Are these items needed?

/**
 * A compact time sample that works well for most purposes.
 * NanoTime is used to minimize memory use. Floats for the quality type
 * complements this, while providing adequate range to properly represent the
 * best atomic clocks.
 */
typedef duds::data::GenericSample<duds::time::interstellar::NanoTime, float>
	NanoTimeSample;

/**
 * A time sample fit for applications requiring very long-term time samples,
 * or time samples with high resolution.
 * FemtoTime should be able to hold any point in time from the begining of the
 * universe until the last star goes dark.
 * Doubles for the quality data should be able to represent the best clocks
 * for at least a few centuries and possibly much longer.
 */
typedef duds::data::GenericSample<duds::time::interstellar::FemtoTime, double>
	FemtoTimeSample;

/**
 * The regular time sample is currently NanoTimeSample for practicality.
 */
typedef NanoTimeSample   TimeSample;

// ___________________________________________________________________________


/**
 * The base type for errors from clocks. Some clocks will only be able to
 * provide a general error that may be platform specific; they can use this
 * error rather than a derivarive. In such cases, add some relavent
 * attributes to the error to provide a better clue, such as
 * boost::errinfo_api_function and boost::errinfo_errno.
 */
struct ClockError : DeviceError { };

/**
 * The foundation to a clock driver.
 * @todo  Need to include a way to get a UUID for the clock from a config file.
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericClock :
public duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT> {
public:
	// copied from base class; cannot use from derived classes without scope
	/** @copydoc GenericDevice::Measurement */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT>  Measurement;
	typedef std::shared_ptr<GenericClock < SVT, SQT, TVT, TQT > >  ClockSptr;
protected:
	/**
	 * General template to convert time in one format to another.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam Src    The source type.
	 * @tparam Dest   The destination type. This implementation is for a
	 *                floating point type.
	 * @param  dest   Where to put the converted time value. Not a return value
	 *                because some destination types are large.
	 * @param  src    The source time.
	 */
	template<class Ratio, class Src, class Dest>
	static void convert(Dest &dest, const Src &src) {
		dest = (Dest)src * ((Dest)(Ratio::num) / (Dest)(Ratio::den));
	}
	/**
	 * Template to convert time in one format to one of the types defined in
	 * duds::time::interstellar.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam IST    The destination time type. It should be a time point type
	 *                from duds::time::interstellar.
	 * @tparam Src    The source type.
	 * @param  dest   Where to put the converted time value.
	 * @param  src    The source time.
	 */
	template<class Ratio, class IST, class Src>
	static void convertIST(IST &dest, const Src &src) {
		// src * Ratio / IST::period
		typedef std::ratio_divide<Ratio, typename IST::period> r;
		dest = IST(typename IST::duration((src * r::num) / r::den));
	}
	#if !defined(HAVE_INT128) | defined(DOXYGEN)
	/**
	 * More specialized template to convert time stored in a 128-bit integer
	 * to one of the types defined in duds::time::interstellar. The
	 * specialization is needed to deal with a type conversion of the 128-bit
	 * integer from Boost's multiprecision library. It is not needed if the
	 * compiler provides a 128-bit integer type.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam IST    The destination time type. It should be a time point type
	 *                from duds::time::interstellar.
	 * @param  dest   Where to put the converted time value.
	 * @param  src    The source time.
	 */
	template<class Ratio, class IST>
	static void convertIST(IST &dest, const duds::data::int128_t &src) {
		// src * Ratio / IST::period
		typedef std::ratio_divide<Ratio, typename IST::period> r;
		dest = IST(typename IST::duration(
			((src * r::num) / r::den).template convert_to<typename IST::rep>()
		));
	}
	#endif
	/**
	 * Template specialization to convert time in one format to FemtoTime.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam Src    The source type.
	 * @param  dest   Where to put the converted time value.
	 * @param  src    The source time.
	 */
	template<class Ratio, class Src>
	static void convert(
		duds::time::interstellar::FemtoTime &dest,
		const Src &src)
	{
		convertIST<Ratio, duds::time::interstellar::FemtoTime>(dest, src);
	}
	/**
	 * Template specialization to convert time in one format to NanoTime.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam Src    The source type.
	 * @param  dest   Where to put the converted time value.
	 * @param  src    The source time.
	 */
	template<class Ratio, class Src>
	static void convert(duds::time::interstellar::NanoTime &dest, const Src &src) {
		convertIST<Ratio, duds::time::interstellar::NanoTime>(dest, src);
	}
	/**
	 * Template specialization to convert time in one format to FemtoTime in
	 * a GenericValue.
	 * @tparam Ratio  The ratio of one second to one unit of @a Src.
	 * @tparam Src    The source type.
	 * @param  dest   Where to put the converted time value.
	 * @param  src    The source time.
	 */
	template<class Ratio, class Src>
	static void convert(duds::data::GenericValue &dest, const Src &src) {
		// same as Femtoseconds
		typedef std::ratio_divide<Ratio, std::femto> r;
		duds::time::interstellar::Femtoseconds::rep bigsrc = src;
		dest = duds::time::interstellar::Femtoseconds(
			(src * r::num) / r::den
		);
	}
	GenericClock() = delete;
	using duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>::sens;
	/**
	 * @todo  Allow naming the clock sensor object.
	 */
	GenericClock(const boost::uuids::uuid &id) :
	duds::hardware::devices::GenericDevice<SVT, SQT, TVT, TQT>(id) {
		boost::uuids::name_generator_latest g(id);
		/** @todo  Provide the name of the clock to the Something base class. */
		sens.push_back(
			Sensor::make(
				//std::static_pointer_cast< GenericClock< SVT, SQT, TVT, TQT > >(
				//	Something::shared_from_this()
				//),
				this,
				g("clock"),
				0
			)
		);
	}
public:
	//using Something::sharedPtr; // doesn't help; no effect
	operator ClockSptr () const {
		return Something::sharedPtr< GenericClock< SVT, SQT, TVT, TQT > >();
	}
	/**
	 * Samples the time from the clock device without storing the result in
	 * the clock's sensor object.
	 * @param time  The place to put the sampled time.
	 */
	virtual void sampleTime(typename Measurement::TimeSample &time) = 0;
	/**
	 * Returns true if the clock properly reports the time during a leap
	 * second. Should the clock replay the second 59 for the leap second,
	 * then two seconds appear the same making the time ambiguous. If the
	 * clock reports 60 for the second field during a leap second, increments
	 * normally, or otherwise makes the leap second distinct, then it is
	 * unambiguous.
	 *
	 * Ideally, ambiguous clocks should also indicate the problem in the
	 * reported quality values. For example, if a clock replays the previous
	 * second during the leap second, the accracy value should increase
	 * by a second starting one second before the leap second and continuing
	 * until after the leap second.
	 */
	virtual bool unambiguous() const noexcept = 0;
	/**
	 * Samples the time from the clock device without storing the result in
	 * the clock's sensor object.
	 * @return   The sampled time.
	 */
	typename Measurement::TimeSample sampleTime() {
		typename Measurement::TimeSample ts;
		sampleTime(ts);
		return ts;
	}
	#ifdef DOXYGEN
	// Additional documentation for clock driver requirements. The functions
	// are defined in GenericDevice.
	/**
	 * Samples the time from this clock stores it in the @a measured field of
	 * the @a Measurement object of the device's first sensor. The @a timestamp
	 * field will be clear.
	 */
	virtual void sample() = 0;
	/**
	 * Samples the time from this clock and the given clock, then stores the
	 * measurement. The sample from this clock will be in the @a measured
	 * field of the @a Measurement object, while the sample from the given
	 * @a clock will be in the @a timestamp field.
	 * @param clock  The clock that will be sampled for the timestamp in the
	 *               resulting measurement. If it is this clock, the clock
	 *               @b must only be sampled once and the same time must be
	 *               in both the @a measured and @a timestamp fields of the
	 *               @a Measurement object. Different types may be used to
	 *               hold the time in those fields so they might not evaluate
	 *               as equal.
	 */
	virtual void sample(const ClockSptr &clock) = 0;
	#endif
};

/**
 * General use clock driver type.
 */
typedef GenericClock<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  Clock;

typedef std::shared_ptr<Clock>  ClockSptr;

/*
template<class SVT, class SQT, class TVT, class TQT>
class GenericClockSensor : public GenericSensor<SVT, SQT, TVT, TQT> {
public:
	
};
*/

} } } }

#endif        //  #ifndef CLOCK_HPP
