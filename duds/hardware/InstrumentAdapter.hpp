/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/Unit.hpp>
#include <memory>

namespace duds { namespace data {

template <class VT, class QT>
class GenericSample;

template <class SVT, class SQT, class TVT, class TQT>
class GenericMeasurement;

} }

namespace duds { namespace hardware {

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrument;

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentDriver;

// allows a driver to modify an Instrument
// may hold the modifible data so that Instrument can reference it while const
/**
 * A connector between an instrument and its driver.
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentAdapter : boost::noncopyable {
public:
	typedef GenericInstrument<SVT, SQT, TVT, TQT>  Instrument;
	typedef GenericInstrumentDriver<SVT, SQT, TVT, TQT>  Driver;
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
private:

	// a const ref of this can be sent to all event listerners / signal slots to
	// avoid making a new shared pointer and assure good behavior in all cases.
	// But this has implications for destruction.
	std::shared_ptr<Instrument> inst;
	std::weak_ptr<Driver> drv;
	// event listeners - all measurements and just newer most current measurement
	// current measurement

	// needed?
	//duds::general::Spinlock lock;

	/**
	 * Instrument::setDriver() calls this class's constructor.
	 */
	friend void Instrument::setDriver(const std::shared_ptr<Driver> &) const;
	/**
	 * Used for safe destruction of the insturment before destruction of the
	 * adapter.
	 */
	friend Instrument::~Instrument();
	GenericInstrumentAdapter(const std::shared_ptr<Instrument> &instrument,
		const std::shared_ptr<Driver> &driver) : inst(instrument), drv(driver) { }
public:
	~GenericInstrumentAdapter() {
		inst->retireAdapter(this);
	}
	bool haveDriver() const noexcept {
		return !drv.expired();
	}
	/*
	std::shared_ptr<Driver> getDriver() const {
		std::shared_ptr<Driver> sdrv = drv.lock();
		if (!sdrv) {
			throw NoDriver;
		}
		return sdrv;
	}
	*/
	std::shared_ptr<Driver> getDriver() const {
		return drv.lock();
	}
	const std::shared_ptr<Instrument> &instrument() const noexcept {
		return inst;
	}
	/**
	 * Provide a measurement from the instrument. The measurement does not
	 * have to be the most recent.
	 * @pre   Neither signalMeasurement() nor signalSample() are called on the
	 *        same adapter object on another thread. The operation is @b not
	 *        thread-safe.
	 * @param measure  The measurement object to be recorded.
	 */
	void signalMeasurement(
		const std::shared_ptr<const Measurement> &measure
	) const {
		inst->signalMeasurement(measure);
	}
	/**
	 * Sends a signal with the given sample along with the current time using
	 * the default clock (?!?).
	 * @pre   Neither signalMeasurement() nor signalSample() are called on the
	 *        same adapter object on another thread. The operation is @b not
	 *        thread-safe.
	 * @param samp  The sample value to record.
	 * @post  The data in @a samp will be swapped into a Measurement object
	 *        created by this function. This is done to avoid a potentially
	 *        expensive copy operation, but means the value of @a samp will
	 *        change and should be considered uninitalized after the call.
	 */
	void signalSample(typename Measurement::Sample &samp) const {
		inst->signalSample(samp);
	}
	//currentMeasurement() const;
	void setUnit(duds::data::Unit u) const {
		inst->u = u;
	}
	// measured property?  relative humidity won't show up in units
	void setPartId(const boost::uuids::uuid &pid) const {
		inst->pid = pid;
	}
};

/**
 * An easy and shorter way to use GenericInstrumentAdapter with the default
 * template arguments.
 */
typedef GenericInstrumentAdapter<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
>  InstrumentAdapter;

} }
