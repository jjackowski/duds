/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/Sample.hpp>

namespace duds { namespace data {

/**
 * Stores a sample of something along with a timestamp stored as a sample from
 * a clock.
 * @tparam SVT  The sample value type. Used for @a VT in a GenericSample.
 * @tparam SQT  The sample quality type. Used for @a QT in a GenericSample.
 * @tparam TVT  The time value type. Used for @a VT in a GenericSample.
 * @tparam TQT  The time quality type. Used for @a QT in a GenericSample.
 * @author  Jeff Jackowski
 */
template <class SVT, class SQT, class TVT, class TQT>
struct GenericMeasurement {
	/**
	 * The type used to store a sample from some instrument.
	 */
	typedef GenericSample<SVT, SQT>  Sample;
	/**
	 * The type used to store the timestamp.
	 */
	typedef GenericSample<TVT, TQT>  TimeSample;
	/**
	 * The time when @a measured was recorded.
	 */
	TimeSample timestamp;
	/**
	 * A sample from an instrument.
	 */
	Sample measured;
	GenericMeasurement() = default;
	GenericMeasurement(const GenericMeasurement&) = default;
	GenericMeasurement(GenericMeasurement&&) = default;
	GenericMeasurement(const TimeSample &t, const Sample &s) :
	timestamp(t), measured(s) { }
	GenericMeasurement(TimeSample &&t, Sample &&s) :
	timestamp(std::move(t)), measured(std::move(s)) { }
};

/**
 * A measurement type capable of holding a wide range of types for the sample
 * value. It is suitable for storing measurements when it is not known at
 * compile-time what sample types are needed.
 */
typedef GenericMeasurement<
	GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  Measurement;

template <
	class VT = GenericValue,
	class QT = double,
	class TT = duds::time::interstellar::NanoTime
>
struct CompactMeasurement {
	CompactSample<TT, float> timestamp;
	CompactSample<VT, QT> measured;
};

} }
