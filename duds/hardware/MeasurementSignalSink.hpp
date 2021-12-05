/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/Instrument.hpp>

namespace duds { namespace hardware {

/**
 * A base class for receiving measurement signals from multiple
 * @ref GenericInstrument "Instruments".
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author Jeff Jackowski
 */
template <class SVT, class SQT, class TVT, class TQT>
class GenericMeasurementSignalSink {
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
	 * Stores connections to a specific Insturment.
	 */
	struct MeasurementConnections {
		boost::signals2::connection newCon;
		boost::signals2::connection oldCon;
	};
	/**
	 * Reduce typing and limit line length; no other reason.
	 */
	typedef std::map<Instrument*, MeasurementConnections> ConnectionMap;
	typedef ConnectionMap  ConnectionMapIterator;
	/**
	 * Stores connections keyed by Instrument pointer. The Instrument pointer
	 * must never be dereferenced and cannot be assumed to point to an existing
	 * object. It is also possible that the same address may be used for a
	 * different Instrument over time.
	 */
	ConnectionMap conns;
	/**
	 * Handles an incoming new measurement signal.
	 * @param i  The originating Instrument.
	 * @param m  The measurement.
	 */
	virtual void handleNewMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) = 0;
	/**
	 * Handles an incoming old measurement signal.
	 * @param i  The originating Instrument.
	 * @param m  The measurement.
	 */
	virtual void handleOldMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) = 0;	
public:
	/**
	 * Disconnects from all signals.
	 */
	virtual ~GenericMeasurementSignalSink() {
		disconnectAll();
	}
	/**
	 * Removes any disconnected connection objects held by this object.
	 * The connections will no longer be connected if the source of the signal
	 * is destroyed.
	 */
	void purgeDisconnections() {
		for (ConnectionMapIterator iter = conns.begin(); iter != conns.end();) {
			if (!iter->newCon.connected() && !iter->oldCon.connected()) {
				iter = conns.erase(iter);
			} else {
				++iter;
			}
		}
	}
	/**
	 * Connect this object to the new measurement signal of the given
	 * instrument, or return the existing connection. Only one connection
	 * per signal per insturment is allowed. Attempts to make additional
	 * identical connections will result in the existing connection being
	 * returned.
	 * @param inst  The Instrument that will be sending the new measurement
	 *              signals.
	 * @param at    The insertion position.
	 */
	boost::signals2::connection newMeasurementSource(
		const InstrumentSptr &inst,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		// look for already existing connections to the instrument
		ConnectionMapIterator iter = conns[inst.get()];
		// is there a connection to this signal already?
		if (iter->newCon.connected()) {
			// already done; do not repeat
			return iter->newCon;
		}
		// make a new connection
		return iter->newCon = inst->newMeasurementConnect(
			std::bind(
				&handleNewMeasure,
				this,
				std::placeholders::_1,
				std::placeholders::_2
			),
			at
		);
	}
	boost::signals2::connection newMeasurementSource(
		const typename MesurementSignal::group_type &group,
		const Instrument &inst,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		// look for already existing connections to the instrument
		ConnectionMapIterator iter = conns[inst.get()];
		// is there a connection to this signal already?
		if (iter->newCon.connected()) {
			// already done; do not repeat
			return iter->newCon;
		}
		// make a new connection
		return iter->newCon = inst->newMeasurementConnect(
			group, std::bind(&handleNewMeasure,
				this,
				std::placeholders::_1,
				std::placeholders::_2
			),
			at
		);
	}
	/**
	 * Disconnects from the new measurement signal of the given instrument.
	 * @param inst  The instrument to disconnect from.
	 * @return      True if a disconnection occured; false if there was no
	 *              connection.
	 */
	bool disconnectNewMeasurement(const InstrumentSptr &inst) {
		// look for the instrument
		ConnectionMapIterator iter = conns.find(inst.get());
		if (iter != conns.end()) {
			// disconnect it
			iter->newCon.disconnect();
			// are both signals disconnected?
			if (!iter->oldCon.connected()) {
				// remove the record for this instrument
				conns.erase(iter);
				return true;
			}
		}
		return false;
	}
	boost::signals2::connection oldMeasurementSource(
		const InstrumentSptr &inst,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		// look for already existing connections to the instrument
		ConnectionMapIterator iter = conns[inst.get()];
		// is there a connection to this signal already?
		if (iter->oldCon.connected()) {
			// already done; do not repeat
			return iter->oldCon;
		}
		// make a new connection
		return iter->oldCon = inst->newMeasurementConnect(
			std::bind(&handleOldMeasure, this, _1, _2), at
		);
	}
	boost::signals2::connection oldMeasurementSource(
		const typename MesurementSignal::group_type &group,
		const Instrument &inst,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		// look for already existing connections to the instrument
		ConnectionMapIterator iter = conns[inst.get()];
		// is there a connection to this signal already?
		if (iter->oldCon.connected()) {
			// already done; do not repeat
			return iter->oldCon;
		}
		// make a new connection
		return iter->oldCon = inst->newMeasurementConnect(
			group, std::bind(&handleOldMeasure, this, _1, _2), at
		);
	}
	/**
	 * Disconnects from the old measurement signal of the given instrument.
	 * @param inst  The instrument to disconnect from.
	 * @return      True if a disconnection occured; false if there was no
	 *              connection.
	 */
	bool disconnectOldMeasurement(const InstrumentSptr &inst) {
		// look for the instrument
		ConnectionMapIterator iter = conns.find(inst.get());
		if (iter != conns.end()) {
			// disconnect it
			iter->oldCon.disconnect();
			// are both signals disconnected?
			if (!iter->newCon.connected()) {
				// remove the record for this instrument
				conns.erase(iter);
				return true;
			}
		}
		return false;
	}
	/**
	 * Disconnects the group from both the new and old measurement signals from
	 * the given instrument.
	 * @param inst  The instrument to disconnect from.
	 * @return      True if a disconnection occured; false if there were no
	 *              connections.
	 */
	bool disconnectAll(const InstrumentSptr &inst) {
		// look for the instrument
		ConnectionMapIterator iter = conns.find(inst.get());
		if (iter != conns.end()) {
			// is either signal connected?
			if (iter->newCon.connected() || iter->oldCon.connected()) {
				// disconnect
				iter->newCon.disconnect();
				iter->oldCon.disconnect();
				// remove the record for this instrument
				conns.erase(iter);
				return true;
			}
		} return false;
	}
	/**
	 * Disconnects the group from all signals.
	 */
	void disconnectAll() {
		ConnectionMapIterator iter;
		for (iter = conns.begin(); iter != conns.end(); iter = conns.erase(iter)) {
			iter->newCon.disconnect();
			iter->oldCon.disconnect();
		}
	}
};

} }
