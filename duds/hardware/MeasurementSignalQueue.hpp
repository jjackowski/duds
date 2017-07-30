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
#include <duds/general/Spinlock.hpp>
#include <utility>
#include <mutex>

namespace duds { namespace hardware {

/**
 * Queues mesurement signals for later processing. The queue is thread-safe to
 * allow queueing and dequeueing from multiple threads. It can be used to
 * store signals from many threads and later process the signals on one thread.
 * The advantages of such a setup are less thread synchronization and avoiding
 * taking up time on the thread that sent the signal. The disadvantage is a
 * greater latency to responding to the signal when ignoring the time taken to
 * handle a signal.
 *
 * @todo           Could make a derived class that can resend the signals
 *                 using GenericMeasurementSignalSource.
 *
 * @tparam SVT     Sample value type
 * @tparam SQT     Sample quality type
 * @tparam TVT     Time value type
 * @tparam TQT     Time quality type
 * @tparam IS      The Instrument storage type. Should be either
 *                 std::shared_ptr or std::weak_ptr. This will affect the
 *                 lifespan of the Instrument objects.
 * @tparam ISArgs  Additional template parameters to @a IS other than the
 *                 data type being stored. In most cases, this can be ommitted.
 *
 * @author Jeff Jackowski
 */
template <
	class SVT,
	class SQT,
	class TVT,
	class TQT,
	template <typename, typename ...> class IS = std::shared_ptr,
	typename... ISArgs
>
class GenericMeasurementSignalQueue :
	public GenericMeasurementSignalSink<SVT, SQT, TVT, TQT>
{
public:
	typedef GenericInstrument<SVT, SQT, TVT, TQT>  Instrument;
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT> Measurement;
	enum EventType {
		NewMeasurement,
		OldMeasurement
	};
	/**
	 * Stores the information from a new or old measurement signal.
	 */
	struct SignalData {
		/**
		 * The originating instrument.
		 */
		IS<Instrument, ISArgs ...> insturment;
		/**
		 * The measurement taken by the instrument.
		 */
		std::shared_ptr<const Measurement> measurement;
		/**
		 * Denotes either a new or old measurement.
		 */
		EventType type;
		SignalData() = default;
		SignalData(
			const IS<Instrument> &i,
			const std::shared_ptr<const Measurement> &m,
			EventType e
		) : insturment(i), measurement(m), type(e) { }
	};
	/**
	 * The list type used to store information from incoming signals.
	 */
	typedef std::list<SignalData>  EventList;
private:
	/**
	 * Storage of signal data.
	 */
	EventList events;
	/**
	 * Used to allow only one thread access to @a events. The accesses are
	 * likely quick in most cases, so a spinlock is used instead of a mutex.
	 */
	mutable duds::general::Spinlock block;
protected:
	/**
	 * Receives a new measurement signal and queues its information.
	 */
	void handleNewMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		events.emplace_back(i, m, NewMeasurement);
	}
	/**
	 * Receives an old measurement signal and queues its information.
	 */
	void handleOldMeasure(
		const std::shared_ptr<Instrument> &i,
		const std::shared_ptr<const Measurement> &m
	) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		events.emplace_back(i, m, OldMeasurement);
	}
public:
	GenericMeasurementSignalQueue() = default;
	GenericMeasurementSignalQueue(const GenericMeasurementSignalQueue &sq) {
		std::lock_guard<duds::general::Spinlock> lock(sq.block);
		events = sq.events;
	}
	GenericMeasurementSignalQueue(GenericMeasurementSignalQueue &&sq) {
		std::lock_guard<duds::general::Spinlock> lock(sq.block);
		events = std::move(sq.events);
	}
	GenericMeasurementSignalQueue &operator = (
		const GenericMeasurementSignalQueue &sq
	) {
		std::unique_lock<duds::general::Spinlock> lock0(block, std::defer_lock);
		std::unique_lock<duds::general::Spinlock> lock1(sq.block, std::defer_lock);
		std::lock(lock0, lock1);
		events = sq.events;
	}
	GenericMeasurementSignalQueue &operator = (
		GenericMeasurementSignalQueue &&sq
	) {
		std::unique_lock<duds::general::Spinlock> lock0(block, std::defer_lock);
		std::unique_lock<duds::general::Spinlock> lock1(sq.block, std::defer_lock);
		std::lock(lock0, lock1);
		events = std::move(sq.events);
	}
	/**
	 * Swaps the internal signal data list with another
	 * GenericMeasurementSignalQueue object.
	 */
	void swap(GenericMeasurementSignalQueue &sq) {
		std::unique_lock<duds::general::Spinlock> lock0(block, std::defer_lock);
		std::unique_lock<duds::general::Spinlock> lock1(sq.block, std::defer_lock);
		std::lock(lock0, lock1);
		events.swap(sq.events);
	}
	/**
	 * Returns a copy of the signal events stored internally.
	 */
	EventList copy() const {
		EventList copy;
		{
			std::lock_guard<duds::general::Spinlock> lock(block);
			copy = events;
		}
		return copy;
	}
	/**
	 * Creates a copy of the ignal events stored internally.
	 * @param copy  The list that will hold a copy of the internal list.
	 */
	void copy(EventList &copy) const {
		std::lock_guard<duds::general::Spinlock> lock(block);
		copy = events;
	}
	/**
	 * Returns a move-constructed list of the signal events stored internally.
	 * @post  No signal events are stored; @a events is empty.
	 */
	EventList move() {
		EventList copy;
		{
			std::lock_guard<duds::general::Spinlock> lock(block);
			copy = std::move(events);
		}
		return copy;
	}
	/**
	 * Move-assigns to a given list the signal events stored internally.
	 * @post  No signal events are stored; @a events is empty.
	 */
	void move(EventList &copy) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		copy = std::move(events);
	}
	/**
	 * Push signal data onto the end (newest side) of the internal list.
	 * @param sd  The information to push.
	 */
	void pushBack(const SignalData &sd) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		events.push_back(sd);
	}
	/**
	 * Push signal data onto the front (oldest side) of the internal list.
	 * @param sd  The information to push.
	 */
	void pushFront(const SignalData &sd) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		events.push_front(sd);
	}
	/**
	 * Pop signal data from the end (newest side) of the internal list and
	 * return that data.
	 */
	SignalData popBack() {
		SignalData sd;
		{
			std::lock_guard<duds::general::Spinlock> lock(block);
			sd = std::move(events.back());
			events.pop_back();
		}
		return sd;
	}
	/**
	 * Pop signal data from the front (oldest side) of the internal list and
	 * return that data.
	 */
	SignalData popFront() {
		SignalData sd;
		{
			std::lock_guard<duds::general::Spinlock> lock(block);
			sd = std::move(events.front());
			events.pop_front();
		}
		return sd;
	}
	/**
	 * Clear the signal data stored internally.
	 */
	void clear() {
		std::lock_guard<duds::general::Spinlock> lock(block);
		events.clear();
	}
};

/**
 * Swap support for GenericMeasurementSignalQueue.
 * This allows std::swap to work with GenericMeasurementSignalQueue objects.
 *
 * @param  sq0     The first signal queue.
 * @param  sq1     The second signal queue. It must use the same template
 *                 parameters as the first.
 *
 * @tparam SVT     Sample value type
 * @tparam SQT     Sample quality type
 * @tparam TVT     Time value type
 * @tparam TQT     Time quality type
 * @tparam IS      The Instrument storage type. Should be either
 *                 std::shared_ptr or std::weak_ptr.
 * @tparam ISArgs  Additional template parameters to @a IS other than the
 *                 data type being stored. In most cases, this can be ommitted.
 *
 * @author Jeff Jackowski
 */
template <
	class SVT,
	class SQT,
	class TVT,
	class TQT,
	template <typename> class IS,
	typename ... ISArgs
>
void swap(
	GenericMeasurementSignalQueue<SVT, SQT, TVT, TQT, IS, ISArgs ...> &sq0,
	GenericMeasurementSignalQueue<SVT, SQT, TVT, TQT, IS, ISArgs ...> &sq1
) {
	sq0.swap(sq1);
}

typedef GenericMeasurementSignalQueue<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> MeasurementSignalQueue;

} }
