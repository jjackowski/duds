/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/signals2/signal.hpp>
#include <duds/data/GenericValue.hpp>

namespace duds { namespace data {

template <class SVT, class SQT, class TVT, class TQT>
class GenericMeasuement;

} }

namespace duds { namespace hardware {

template<class SVT, class SQT, class TVT, class TQT>
class GenericInstrument;

/**
 * Handles sending signals of measurements taken by an instrument.
 * This class is intened to be used as a base class. The functions to send
 * signals are protected to limit access. The constructors are also protected.
 * @author  Jeff Jackowski
 */
template <class SVT, class SQT, class TVT, class TQT>
class GenericMeasurementSignalSource {
public:
	typedef GenericInstrument<SVT, SQT, TVT, TQT>  Instrument;
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	/**
	 * The type used for event listeners that are told of measurments.
	 */
	typedef boost::signals2::signal<
		void (const std::shared_ptr<Instrument> &,
		const std::shared_ptr<const Measurement> &)
	> MesurementSignal;
protected:
	/**
	 * A set of signals that are invoked when a meaurement on this Instrument
	 * is recorded that is newer than any previously recorded measurement.
	 * @note  Declared as mutable to allow changes to this member when the
	 *        object is stored in containers like std::set that keep const
	 *        elements.
	 */
	mutable MesurementSignal newMeasure;
	/**
	 * A set of signals that are invoked when a meaurement on this Instrument
	 * is recorded that is older than another already recorded measurement.
	 * @note  Declared as mutable to allow changes to this member when the
	 *        object is stored in containers like std::set that keep const
	 *        elements.
	 */
	mutable MesurementSignal oldMeasure;
	/**
	 * This class is intened to be used as a base class.
	 */
	GenericMeasurementSignalSource() = default;
	/**
	 * This class is intened to be used as a base class.
	 */
	GenericMeasurementSignalSource(const GenericMeasurementSignalSource&) = default;
public:
	/**
	 * Make a connection to receive signals for new measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection newMeasurementConnect(
		const typename MesurementSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return newMeasure.connect(slot, at);
	}
	/**
	 * Make a connection to receive signals for new measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection newMeasurementConnect(
		const typename MesurementSignal::group_type &group,
		const typename MesurementSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return newMeasure.connect(group, slot, at);
	}
	/**
	 * Make a connection to receive signals for new measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp187906672-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection newMeasurementConnectExtended(
		const typename MesurementSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return newMeasure.connect_extended(slot, at);
	}
	/**
	 * Make a connection to receive signals for new measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp187906672-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection newMeasurementConnectExtended(
		const typename MesurementSignal::group_type &group,
		const typename MesurementSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return newMeasure.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect from the new measurement signal. See the
	 * [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp195142656-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void newMeasurementDisconnect(
		const typename MesurementSignal::group_type &group
	) {
		newMeasure.disconnect(group);
	}
	/**
	 * Disconnect from the new measurement signal. See the
	 * [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp195142656-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename S>
	void newMeasurementDisconnect(const S &slotFunc) {
		newMeasure.disconnect(slotFunc);
	}

	/**
	 * Make a connection to receive signals for old measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection oldMeasurementConnect(
		const typename MesurementSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return oldMeasure.connect(slot, at);
	}
	/**
	 * Make a connection to receive signals for old measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection oldMeasurementConnect(
		const typename MesurementSignal::group_type &group,
		const typename MesurementSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return oldMeasure.connect(group, slot, at);
	}
	/**
	 * Make a connection to receive signals for old measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp187906672-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection oldMeasurementConnectExtended(
		const typename MesurementSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return oldMeasure.connect_extended(slot, at);
	}
	/**
	 * Make a connection to receive signals for old measurements.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp187906672-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection oldMeasurementConnectExtended(
		const typename MesurementSignal::group_type &group,
		const typename MesurementSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return oldMeasure.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect from the old measurement signal. See the
	 * [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp195142656-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void oldMeasurementDisconnect(
		const typename MesurementSignal::group_type &group
	) {
		oldMeasure.disconnect(group);
	}
	/**
	 * Disconnect from the old measurement signal. See the
	 * [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp195142656-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename S>
	void oldMeasurementDisconnect(const S &slotFunc) {
		oldMeasure.disconnect(slotFunc);
	}
};

typedef GenericMeasurementSignalSource<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> MeasurementSignalSource;

} }
