/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/InstrumentDriver.hpp>
#include <duds/Something.hpp>
#include <vector>

namespace duds { namespace hardware {

template<class SVT, class SQT, class TVT, class TQT>
class GenericClockDriver;

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrumentAdapter;

/**
 * Represents something with one or more instruments.
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericDevice : public Something {
public:
	/**
	 * The instrument type used by this device.
	 */
	typedef GenericInstrument<SVT, SQT, TVT, TQT>  Instrument;
	typedef GenericInstrumentDriver<SVT, SQT, TVT, TQT>  InstrumentDriver;
	/**
	 * The base adapter class used for the instrument drivers for this device.
	 */
	typedef GenericInstrumentAdapter<SVT, SQT, TVT, TQT>  Adapter;
	/**
	 * The measurement type provided by the instruments of this device.
	 */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	/**
	 * The clock driver type used by the instruments of this device for
	 * time stamps.
	 */
	typedef GenericClockDriver<SVT, SQT, TVT, TQT> ClockDriver;

	struct InstrumentAndDriver {
		std::shared_ptr<Instrument> instrument;
		std::shared_ptr<InstrumentDriver> driver;
	};
protected:
	std::vector<InstrumentAndDriver> instruments;
public:
	/**
	 * Derived classes should create Instrument objects and place them in
	 * @a instruments either in the constructor or an initialization function.
	 */
	Device() = default;
	
	const std::shared_ptr<Instrument> &instrument(int idx) const {
		return instruments[idx].instrument;
	}
	std::size_t size() const {
		return instruments.size();
	}
};

typedef GenericDevice<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> Device;


} }
