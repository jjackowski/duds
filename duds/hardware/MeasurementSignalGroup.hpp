/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/MeasurementSignalSink.hpp>

namespace duds { namespace hardware {

/**
 * Distributes measurement signals from one or more instruments to one or more
 * listeners. Listeners connect to signals in this class the same way they
 * connect to signals from an Insturment. Objects of this type connect to
 * Intruements to receive their signals. When the signal group receives a
 * signal from an Instrument, it resends the signal to all of its listeners.
 *
 * @warning     This class and its functions are not thread-safe because
 *              I suspect that modifying these objects will typically not
 *              be done across threads.
 *
 * Without using MeasurementSignalGroup, listeners must be connected to each
 * Instrument of interest:
 * @dot
 * digraph NoGroup {
 * 	graph [ bgcolor="transparent" ];
 * 	node [ fontname="Helvetica", fontsize=10, height=0.2,
 * 		fillcolor="white", style="filled" ];
 * 	Instrument0 [label = "Instrument 0", shape=record];
 * 	Instrument1 [label = "Instrument 1", shape=record];
 * 	ListenerA [label = "Listener A"];
 * 	ListenerB [label = "Listener B"];
 * 	ListenerC [label = "Listener C"];
 * 	ListenerD [label = "Listener D"];
 * 	ListenerA -> Instrument0;
 * 	ListenerB -> Instrument0;
 * 	ListenerC -> Instrument0;
 * 	ListenerA -> Instrument1;
 * 	ListenerB -> Instrument1;
 * 	ListenerC -> Instrument1;
 * 	ListenerD -> Instrument1;
 * }
 * @enddot
 * Using MeasurementSignalGroup allows for the same functional result while
 * grouping connections together in a way that can make the connections easier
 * to manage and may reduce the number of connections:
 * @dot
 * digraph WithGroup {
 * 	graph [ bgcolor="transparent" ];
 * 	node [ fontname="Helvetica", fontsize=10, height=0.2,
 * 		fillcolor="white", style="filled" ];
 * 	Instrument0 [label = "Instrument 0", shape=record];
 * 	Instrument1 [label = "Instrument 1", shape=record];
 * 	Group [label = "Group", shape=record];
 * 	ListenerA [label = "Listener A"];
 * 	ListenerB [label = "Listener B"];
 * 	ListenerC [label = "Listener C"];
 * 	ListenerD [label = "Listener D"];
 * 	ListenerA -> Group;
 * 	ListenerB -> Group;
 * 	ListenerC -> Group;
 * 	ListenerD -> Instrument1;
 * 	Group -> Instrument0;
 * 	Group -> Instrument1;
 * }
 * @enddot
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template <class SVT, class SQT, class TVT, class TQT>
class GenericMeasurementSignalGroup :
	public GenericMeasurementSignalSource<SVT, SQT, TVT, TQT>,
	public GenericMeasurementSignalSink<SVT, SQT, TVT, TQT>
{
protected:
	/**
	 * Receives a new measurement signal and resends it on this object's
	 * corresponding signal.
	 */
	void handleNewMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) {
		newMeasure(i, m);
	}
	/**
	 * Receives an old measurement signal and resends it on this object's
	 * corresponding signal.
	 */
	void handleOldMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) {
		oldMeasure(i, m);
	}
public:
	/**
	 * Disconnects from all signals.
	 */
	virtual ~GenericMeasurementSignalGroup() = default;
};

typedef GenericMeasurementSignalGroup<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> MeasurementSignalGroup;

} }
