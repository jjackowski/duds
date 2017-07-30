/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/GenericValue.hpp>
#include <limits>

namespace duds { namespace data {

/**
 * A namespace to hold some data structures where they won't cause trouble.
 */
namespace _it_needs_to_go_somewhere_ {
	/**
	 * Needed by unspecified(). Functions cannot have local structs with static
	 * data members. The function also needs a template to provide a double,
	 * but doubles cannot be used as template parameters. Structs can be used,
	 * but they need a static data member for the double.
	 */
	template <class T>
	struct infinity {
		static constexpr T value = std::numeric_limits<T>::infinity();
	};
	/**
	 * Needed by unspecified(). Functions cannot have local structs with static
	 * data members. The function also needs a template to provide a double,
	 * but doubles cannot be used as template parameters. Structs can be used,
	 * but they need a static data member for the double.
	 */
	template <class T>
	struct lowest {
		static constexpr T value = std::numeric_limits<T>::lowest();
	};
}

/**
 * Returns the value used to represent an unspecified or unknown accuracy,
 * precision, resolution, or error. It will either be the type's representation
 * of infinity, or if it lacks that, the lowest possible value. This works
 * for floating point, fix point, and signed integer types. It does not work
 * well for unsigned integers.
 * @author  Jeff Jackowski
 */
template <typename QT>
static constexpr QT unspecified() {
	return std::conditional<std::numeric_limits<QT>::has_infinity,
		_it_needs_to_go_somewhere_::infinity<QT>,
		_it_needs_to_go_somewhere_::lowest<QT>
	>::type::value;
}

template <class VT = GenericValue, class QT = double>
struct CompactSample;

/**
 * A template for a sample from an insturment.
 * @tparam VT  The type used to store the value meausred by the insturment.
 *             In most cases, this should be either @a GenericValue for
 *             flexibility, or @a double for less memory usage. In the case
 *             of a time stamp rather than an interval, the value should be
 *             an integer to avoid loss of resolution the further the time is
 *             from time zero. GenericValue includes time types.
 * @tparam QT  The type used to store quality information about the sampled
 *             value. In the case of a time stamp, a double or float could be
 *             used. Units are be the same as in @a VT, unless @a VT is one
 *             of the duds::time::interstellar time types and @a QT is a
 *             floating point type, in which case the units are seconds.
 *             Unsigned intergers must @b not be used.
 * @author  Jeff Jackowski
 * @todo    Should the unit field be for the quality fields, or should the
 *          quality fields be changed to Quantity objects?
 */
template <class VT, class QT>
struct GenericSample {
	typedef VT Value;
	typedef QT Quality;
	/**
	 * Returns the value used to represent an unspecified or unknown accuracy,
	 * precision, resolution, or error.
	 */
	static constexpr QT unspecified() {
		return duds::data::unspecified<QT>();
	}
	/**
	 * The UUID for the source instrument of this sample.
	 */
	boost::uuids::uuid origin;
	/**
	 * The value sampled from the insturment.
	 * This should normally be a Quantity type, or a GenericValue storing a
	 * Quantity object.
	 */
	VT value;
	/**
	 * The expected accuracy of the instrument under the conditions in which the
	 * sample was taken. This is how far from correct the value could be.
	 */
	QT accuracy;
	/**
	 * The expected precision of the instrument under the conditions in which
	 * the sample was taken. This is how much the sample value may vary when
	 * the measured property is the same; think of it as consistency or
	 * repeatability.
	 */
	QT precision;
	/**
	 * The estimated error of the observation. This is primarily to support
	 * the estimated error field reported by adjtimex() on Linux, but may have
	 * application elsewhere.
	 */
	QT estError;
	/**
	 * The expected resolution of the instrument under the conditions in which
	 * the sample was taken. This is the smallest increment the insturment can
	 * represent. Since the Sample object may store the value in different units
	 * than the instrument reports, this value is not determined by the types
	 * used in the Sample object.
	 */
	QT resolution;
	/**
	 * Produce a CompactSample using the data in this Sample.
	 */
	CompactSample<VT, QT> makeCompactSample() const;
	/**
	 * Produce a CompactSample using the data in this Sample.
	 */
	operator CompactSample<VT, QT> () const {
		return makeCompactSample();
	}
};

/**
 * A sample type that is good for general purpose use. It isn't the most
 * memory efficent type, but it can work with a wide variety of data types
 * to hold the sample. This makes it a good choice to support anything when
 * it isn't known at compile-time what will be sampled.
 */
typedef GenericSample<GenericValue, double>  Sample;

/**
 * A template for a sample from an insturment that does not store units or the
 * origin. This is intended for use in data structures that store many samples
 * from the same Instrument.
 * @tparam VT  The type used to store the value meausred by the insturment.
 *             In most cases, this should be @a double. In the case of a time
 *             stamp rather than an interval, the value should be an integer
 *             to avoid loss of resolution the further the time is from time
 *             zero.
 * @tparam QT  The type used to store quality information about the sampled
 *             value. In the case of a time stamp, a double or float could be
 *             used. Units are be the same as in @a VT, unless @a VT is one
 *             of the duds::time::interstellar time types and @a QT is a
 *             floating point type, in which case the units are seconds.
 *             Unsigned intergers must @b not be used.
 * @author  Jeff Jackowski
 */
template <class VT, class QT>  // default types in forward declaration above
struct CompactSample {
	typedef VT Value;
	typedef QT Quality;
	/**
	 * The value used to represent an unspecified or unknown accuracy,
	 * precision, or resolution.
	 */
	static constexpr QT Unspecified = std::numeric_limits<QT>::infinity();
	/**
	 * The value sampled from the insturment.
	 */
	VT value;
	/**
	 * The expected accuracy of the instrument under the conditions in which the
	 * sample was taken. This is how far from correct the value could be.
	 */
	QT accuracy;
	/**
	 * The expected precision of the instrument under the conditions in which
	 * the sample was taken. This is how much the sample value may vary when
	 * the measured property is the same; think of it as consistency or
	 * repeatability.
	 */
	QT precision;
	/**
	 * The expected resolution of the instrument under the conditions in which
	 * the sample was taken. This is the smallest increment the insturment can
	 * represent. Since the Sample object may store the value in different units
	 * than the instrument reports, this value is not determined by the types
	 * used in the Sample object.
	 */
	QT resolution;
	/*
	 * Produce a Sample using the data in this CompactSample and the given units.
	 * @param unit  The units used in the CompactSample; used to fill the units
	 *              field of the procuded Sample object.
	 */
	 /*
	Sample<VT, QT> makeSample(const Unit unit) const {
		Sample<VT, QT> s = { value, accuracy, precision, resolution, unit };
		return s;
	}
	*/
};

// no units
template <class VT, class QT>  // default types in forward declaration above
struct SampleNU {
	typedef VT Value;
	typedef QT Quality;
	/**
	 * The value used to represent an unspecified or unknown accuracy,
	 * precision, or resolution.
	 */
	static constexpr QT Unspecified = std::numeric_limits<QT>::infinity();
	/**
	 * The source of this sample.
	 */
	boost::uuids::uuid origin;
	/**
	 * The value sampled from the insturment.
	 */
	VT value;
	/**
	 * The expected accuracy of the instrument under the conditions in which the
	 * sample was taken. This is how far from correct the value could be.
	 */
	QT accuracy;
	/**
	 * The expected precision of the instrument under the conditions in which
	 * the sample was taken. This is how much the sample value may vary when
	 * the measured property is the same; think of it as consistency or
	 * repeatability.
	 */
	QT precision;
	/**
	 * The expected resolution of the instrument under the conditions in which
	 * the sample was taken. This is the smallest increment the insturment can
	 * represent. Since the Sample object may store the value in different units
	 * than the instrument reports, this value is not determined by the types
	 * used in the Sample object.
	 */
	QT resolution;
	/*
	 * Produce a Sample using the data in this CompactSample and the given units.
	 * @param unit  The units used in the CompactSample; used to fill the units
	 *              field of the procuded Sample object.
	 */
	 /*
	Sample<VT, QT> makeSample(const Unit unit) const {
		Sample<VT, QT> s = { value, accuracy, precision, resolution, unit };
		return s;
	}
	*/
};


template <class VT, class QT>
CompactSample<VT, QT> GenericSample<VT, QT>::makeCompactSample() const {
	CompactSample<VT, QT> cs = { value, accuracy, precision, resolution };
	return cs;
}

template <class VT>
struct ExtraCompactSample {
	/**
	 * The value sampled from the insturment.
	 */
	VT value;
};

} }
