/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <memory>
#include <duds/general/Errors.hpp>

namespace duds { namespace data {

template<class SVT, class SQT, class TVT, class TQT>
class GenericMeasurement;

} }

namespace duds { namespace hardware {

template<class SVT, class SQT, class TVT, class TQT>
class GenericClockDriver;

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentAdapter;

//class Device : public Something {
	// hold GenericInstrument objects
	// hold/own GenericInstrumentDriver objects
		// maybe make GenericInstrumentDriver as sub-classes of classes derived from this class
			// no; must be separate to support multiple implementations
		// do not require linking to hardware support
	// make drivers for local or remote
		// some kind of discovery and configuration method
			// could use this to construct driver objects
	// find remote by UUID
//};

// maybe allow use of objects from std::bind instead of or in addition to a class?

// Change to an interface for requests made through InstrumentDriver, and make
// sure that the calls can be delivered over a network in a future implementation.
// Can derive from this as private.
/**
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentDriver {  // not a Something
public:
	/**
	 * The instrument type used as a facade to this driver.
	 */
	typedef GenericInstrument<SVT, SQT, TVT, TQT>  Instrument;
	/**
	 * The base adapter class used for this instrument driver.
	 */
	typedef GenericInstrumentAdapter<SVT, SQT, TVT, TQT>  Adapter;
	/**
	 * The measurement type provided by this instrument driver.
	 */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	/**
	 * The clock driver type required by this instrument for time stamps.
	 */
	typedef GenericClockDriver<SVT, SQT, TVT, TQT> ClockDriver;
	/**
	 * Drivers may supply thier own destructor.
	 */
	virtual ~GenericInstrumentDriver() { }
	//virtual void init(Instrument &i) = 0;
	//virtual size_t insturmentCount() = 0;
	// how to get insturments?
	//virtual void addInsturments(InstrumentSet &is) = 0;

	/**
	 * Called by Instrument::setDriver() with the InstrumentAdapter object
	 * for the Instrument. The shared pointer @a adp is the only shared pointer
	 * that references the InstrumentAdapter object; a copy must be retained
	 * to maintain the InstrumentAdapter object.
	 * @param adp  A shared pointer to the InstrumentAdapter object used to
	 *             update the corresponding Instrument object.
	 * @throw boost::exception  An error that will prevent the driver from
	 *                          being used.
	 */
	virtual void setAdapter(const std::shared_ptr<Adapter> &adp) = 0;
	/**
	 * Sample the instrument a send a new measurement event. The @a measured
	 * field in the generated Measurement object(s) must contain the results
	 * from sampling this instrument. Calling this function is not required to
	 * generate measurement events; the driver may produce them as it sees
	 * fit. However, in the case when not producing a new sample is the
	 * correct and normal behavior, an exception must not be thrown, unless
	 * this function should never be called.
	 *
	 * This function provides a way to offer
	 * a generic interface to poll for new samples.
	 *
	 * @note         This function should work in a synchronous manner.
	 * @pre          The function must never be called from more than one
	 *               thread simultaneously.
	 * @param clock  The clock driver to query for a time stamp.
	 * @throw duds::general::Unimplemented  The implementing class does not
	 *                                     allow sampling through this function.
	 */
	virtual void sample(ClockDriver &clock) {
		BOOST_THROW_EXCEPTION(duds::general::Unimplemented());
	}
	//virtual void changeStatus(/* Instrument or InstrumentAdapter ? */) = 0;

	// request measurement function ?
};

/**
 * Blarg.
 * @bug  Doxygen is not automatically making links to typedefs of templates.
 */
typedef GenericInstrumentDriver<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  InstrumentDriver;

} }
