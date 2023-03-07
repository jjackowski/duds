/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP

#include <duds/Something.hpp>
#include <duds/data/Measurement.hpp>
#include <duds/hardware/MeasurementSignalSource.hpp>
//#include <boost/signals2/signal.hpp>
//#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace duds { namespace hardware {

struct InstrumentDriverAlreadySet : virtual std::exception, virtual boost::exception { };

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentDriver;

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentAdapter;

// maybe send sample events, know nothing of the set, and allow use of multiple
// sets with the same object?
// *MUST* derive from std::enable_shared_from_this ?

/**
 * Represents a specific instrument on a specific device.
 *
 * Instruments are handled in three parts: the instrument, driver, and adapter.
 * The GenericInstrument class represnts the hardware that can measure
 * something. Some devices have hardware to measure multiple things, like
 * temperature and pressure, and require multiple GenericInstrument objects
 * to be fully and properly represnted. The GenericInstrumentDriver class is
 * an interface to the code that communicates with the hardware or otherwise
 * obtains data from the instrument. The GenericInstrumentAdapter class allows
 * a single GenericInstrumentDriver object to update a single GenericInstrument
 * object, however a driver could gather multiple adapters to update multiple
 * instruments. This separation allows multiple driver implementations for
 * the same kind of hardware, and for an abstraction that prevents the code
 * using the input from being tied to the code that provides the input.
 * The plan is to use this feature to allow updates for a GenericInstrument
 * to either come from the local hardware or to travel over a network, and
 * allow code that uses the input from the instrument to be unaware of the
 * difference and able to function without a hardware driver for the
 * instrument.
 *
 * @sa Instrument
 * @sa InstrumentDriver
 * @sa InstrumentAdapter
 *
 * @todo derive from std::enable_shared_from_this ???
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * Have a reference clock. Maybe pass it in to functions that produce a
 * measurement. Maybe make it a static. Maybe do both.
 *
 * @author Jeff Jackowski
 */
template <class SVT, class SQT, class TVT, class TQT>
class GenericInstrument :
	public Something,
	public GenericMeasurementSignalSource<SVT, SQT, TVT, TQT>
{
public:
	/**
	 * The measurement type used by this instrument.
	 */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT>  Measurement;
	/**
	 * The base driver class used for this instrument.
	 */
	typedef GenericInstrumentDriver<SVT, SQT, TVT, TQT>  Driver;
	/**
	 * The base adapter class used for this instrument.
	 */
	typedef GenericInstrumentAdapter<SVT, SQT, TVT, TQT>  Adapter;
private:
	friend Adapter;
	/**
	 * The UUID for the part, the specific component that contains the
	 * instrument. The only link to the part is an UUID rather than some
	 * pointer so that the part object need not exist. Not all programs will
	 * have need of the part object.
	 */
	boost::uuids::uuid pid;
	/**
	 * The most current measurement from the insturment. It will be empty
	 * until the first measurement is receieved.
	 * @note  Declared as mutable to allow changes to this member when the
	 *        object is stored in containers like std::set that keep const
	 *        elements.
	 */
	mutable std::shared_ptr<const Measurement> currmeas;
	mutable Adapter *adp;  // like an access object
	/**
	 * The units of the instrument's samples.
	 */
	duds::data::Unit u;
	/**
	 * Removes the adapter object when that object is destroyed.
	 */
	void retireAdapter(Adapter *ia) noexcept{
		// these really should match
		if (ia == adp) {
			adp = NULL;
		}
	}
	/**
	 * Provide a measurement to the signal listeners. The measurement does not
	 * have to be the most recent.
	 * @pre   Neither signalMeasurement() nor signalSample() are called for the
	 *        same instrument object on another thread. The operation is @b not
	 *        thread-safe.
	 * @param measure  The measurement object to be recorded.
	 */
	void signalMeasurement(
		const std::shared_ptr<const Measurement> &measure
	) {
		/** @todo  Check units - assure match */
		// time check -- newer measurement?
		if (measure->timestamp.value > currmeas->timestamp.value) {
			// assign current measurement
			currmeas = measure;
			// send new measurement signal
			this->newMeasure(sharedPtr(), measure);
		} else {
			// send old measurement signal
			this->oldMeasure(sharedPtr(), measure);
		}
	}
	/**
	 * Sends a signal with the given sample along with the current time using
	 * the default clock (?!?).
	 * @pre   Neither signalMeasurement() nor signalSample() are called for the
	 *        same instrument object on another thread. The operation is @b not
	 *        thread-safe.
	 * @param samp     The sample value to record.
	 * @post  The data in @a samp will be swapped into a Measurement object
	 *        created by this function. This is done to avoid a potentially
	 *        expensive copy operation, but means the value of @a samp will
	 *        change and should be considered uninitalized after the call.
	 */
	void signalSample(
		typename Measurement::Sample &samp
	) {

		/** @todo   Record the time.  */

		typename Measurement::TimeSample t;
		// make a new measurement
		std::shared_ptr<Measurement> m = std::make_shared<Measurement>(
			std::move(t), std::move(samp));

		/*  Or maybe record time here directly to m->timestamp.  */

		// swap the samples
		//std::swap(m->measured, samp);
		// record the measurement
		signalMeasurement(m);
	}
public:
	/**
	 * @post  The units will be reported as unitless. The part ID will be zero.
	 * @param uid     The UUID for this specific instrument.
	 */
	GenericInstrument(const boost::uuids::uuid &uid) : Something(uid),
		pid(boost::uuids::nil_uuid()), adp(nullptr), u(0) { }
	/**
	 * @post  The units will be reported as unitless.
	 * @param uid     The UUID for this specific instrument.
	 * @param partId  The UUID identifying the type of gizmo that has the
	 *                instrument.
	 */
	GenericInstrument(const boost::uuids::uuid &uid,
		const boost::uuids::uuid &partId) : Something(uid),
		pid(partId), adp(nullptr), u(0) { }
	/**
	 * @post  The units will be reported as unitless unless the driver sets
	 *        the units. The part ID will be zero.
	 * @param uid     The UUID for this specific instrument.
	 * @param driver  The instrument driver object.
	 */
	GenericInstrument(const boost::uuids::uuid &uid,
		const std::shared_ptr<Driver> &driver) : Something(uid),
		pid(boost::uuids::nil_uuid()), adp(nullptr), u(0)
	{
		setDriver(driver);
	}
	/**
	 * @post  The units will be reported as unitless unless the driver sets
	 *        the units.
	 * @param uid     The UUID for this specific instrument.
	 * @param partId  The UUID identifying the type of gizmo that has the
	 *                instrument.
	 * @param driver  The instrument driver object.
	 */
	GenericInstrument(const boost::uuids::uuid &uid,
		const boost::uuids::uuid &partId,
		const std::shared_ptr<Driver> &driver) : Something(uid), pid(partId),
		adp(nullptr), u(0)
	{
		setDriver(driver);
	}
	/**
	 * Severs the link from the adapter object to this object should an adapter
	 * still exist. Ideally, an adapter should not exist or should no longer
	 * be in active use.
	 */
	~GenericInstrument() {
		// if an adpater still exists . . .
		if (adp) {
			assert(adp->inst.get() == this);
			// . . . destroy its link back to this object
			adp->inst.reset();
		}
	}
	
	std::shared_ptr<GenericInstrument> sharedPtr() {
		return std::static_pointer_cast<GenericInstrument>(shared_from_this());
	}
	
	/**
	 * Sets the driver object for this instrument.
	 * @pre   A driver has not been set for this instrument, or it has lost
	 *        its reference to the InstrumentAdapter.
	 * @param driver   The driver object that will communicate with the
	 *                 instrument. It must @b not be an empty std::shared_ptr.
	 * @throw InstrumentDriverAlreadySet   Destroy all shared pointer
	 *                                     references to the Adapter object
	 *                                     before setting a new driver.
	 * @throw std::exception        An error thrown from
	 *                              Driver::setAdapter(). The driver
	 *                              will not be used.
	 */
	void setDriver(const std::shared_ptr<Driver> &driver) const {
		// there must not already be an adapter object
		if (adp) {
			/** @todo  Add an attribute, either UUID or std::shared_ptr.  */
			DUDS_THROW_EXCEPTION(InstrumentDriverAlreadySet());
		}
		std::shared_ptr<Adapter> iadapt = std::make_shared<Adapter>(driver);
		// give the driver its adapter object; may throw
		driver->setAdapter(iadapt);
		// record the adapter's address
		adp = iadapt.get();
	}
	/**
	 * Makes an adapter object without an associated driver object.
	 * This may make it a little easier to use, but will block any requests
	 * made of the instrument through this class.
	 * @pre   A driver has not been set for this instrument, or it has lost
	 *        its reference to the InstrumentAdapter.
	 * @throw InstrumentDriverAlreadySet   Destroy all shared pointer
	 *                                     references to the Adapter object
	 *                                     before setting a new driver.
	 */
	std::shared_ptr<Adapter> makeAdapter() const {
		// there must not already be an adapter object
		if (adp) {
			/** @todo  Add an attribute, either UUID or std::shared_ptr.  */
			DUDS_THROW_EXCEPTION(InstrumentDriverAlreadySet());
		}
		std::shared_ptr<Adapter> iadapt =
			std::make_shared<Adapter>(std::shared_ptr<Driver>());
		// record the adapter's address
		adp = iadapt.get();
		return iadapt;
	}
	/*  Needs some global store of parts to work, so maybe not a good idea.
	std::shared_ptr<Part> part() const {
		return adp.part(); // or find from a UUID
	}
	*/
	/**
	 * Returns the UUID of the part, the component that provides the
	 * instrument. If the UUID has not been provided, a nil UUID (all zeros)
	 * will be returned.
	 */
	const boost::uuids::uuid &partId() const noexcept {
		return pid;
	}
	// status  --  see Instance.hpp from old duds
	/*  incomplete
	status() const {
		return adp->status();
	}
	changeStatus() const {
		if (adp) {
			adp->changeStatus();
		} else {
			throw 1;
		}
	}
	*/
	/**
	 * Returns the units of the samples provided by this instrument.
	 * The units should be set by the driver through the adapter. Any units
	 * other than unitless should have a non-zero numeric value. Other types
	 * and more complex values must be reported as unitless.
	 */
	duds::data::Unit unit() const noexcept {
		return u;
	}
	// measured property?  relative humidity won't show up in units
	/**
	 * Returns the most current measurement.
	 * @note   This function returns a copy of the std::shared_ptr managing the
	 *         measurement object. This is required to avoid further
	 *         synchronization while working properly should another thread
	 *         update the current measurement. Hold and use a local copy of
	 *         the std::shared_ptr for the best performance.
	 * @warning  Successive calls to this function may return a std::shared_ptr
	 *           with a different measurement object. If the measurement
	 *           object needs to be accessed multiple times without changing,
	 *           a copy of the shared pointer must be retained and used to
	 *           access the measurement.
	 */
	std::shared_ptr<const Measurement> currentMeasurement() const noexcept {
		return currmeas;
	}
	//currentSample() const;  // ???

	// future?: optional data on accuracy, precision, resolution


	// get & set status
	//status() const;
	//changeStatus(); // requests change of status

};

/**
 * An easy and shorter way to use GenericInstrument with the best generally
 * applicable template arguments.
 * @bug  Doxygen is not automatically making links to typedefs of templates.
 */
typedef GenericInstrument<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  Instrument;

typedef std::shared_ptr<Instrument>  InstrumentSptr;
typedef std::weak_ptr<Instrument>  InstrumentWptr;

} }

#endif        //  #ifndef INSTRUMENT_HPP
