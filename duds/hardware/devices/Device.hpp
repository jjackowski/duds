/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/Sensor.hpp>
#include <atomic>
#include <thread>

namespace duds { namespace hardware { namespace devices {

namespace clocks {
	template<class SVT, class SQT, class TVT, class TQT>
	class GenericClock;
}

/**
 * Represents something with one or more sensors that are sampled through the
 * same hardware. This is intended to support sampling sensors and finding
 * sensors.
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
	/**
	 * Used to assure the destructor function does not return before Sensor
	 * objects quit using member functions on this object on other threads.
	 */
	std::atomic<unsigned int> destructWait;
	/**
	 * Used to assure the destructor function does not return before Sensor
	 * objects quit using member functions on this object on other threads.
	 * @todo  Would this be useful in a generic form outside this class?
	 */
	class DestructCounter {
		std::atomic<unsigned int> &count;
	public:
		DestructCounter(std::atomic<unsigned int> &c) : count(c) {
			++count;
		}
		~DestructCounter() {
			--count;
		}
	};
public:
	/**
	 * The sensor type used by this device.
	 */
	typedef GenericSensor<SVT, SQT, TVT, TQT>  Sensor;
	/**
	 * A shared pointer type to the sensor type used by this device.
	 */
	typedef std::shared_ptr<Sensor>  SensorSptr;
	/**
	 * A shared pointer type to the const sensor type used by this device.
	 */
	typedef std::shared_ptr<const Sensor>  ConstSensorSptr;
	/**
	 * A weak pointer type to the sensor type used by this device.
	 */
	typedef std::weak_ptr<Sensor>  SensorWptr;
	/**
	 * The measurement type provided by the instruments of this device.
	 */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT>  Measurement;
	/**
	 * A shared pointer type to the measurement type used by this device.
	 */
	typedef std::shared_ptr<Measurement>  MeasurementSptr;
	/**
	 * A shared pointer type to the const measurement type used by this device.
	 */
	typedef std::shared_ptr<const Measurement>  ConstMeasurementSptr;
	/**
	 * The clock driver type used by the instruments of this device for
	 * time stamps.
	 */
	typedef clocks::GenericClock<SVT, SQT, TVT, TQT>  Clock;
	/**
	 * A shared pointer type to the clock type used by this device.
	 */
	typedef std::shared_ptr<Clock>  ClockSptr;
protected:
	/**
	 * Contains Sensor objects to represent all the kinds of data this device
	 * can collect.
	 */
	std::vector<SensorSptr> sens;
	/**
	 * Updates the current measurement of a member sensor.
	 * @param store  The new measurement object.
	 * @param sIdx   The index of the sensor to update.
	 */
	void setMeasurement(const ConstMeasurementSptr &store, int sIdx = 0) const {
		sens[sIdx]->meas = store;
	}
	/**
	 * Updates the current measurement of a member sensor.
	 * @param store  The new measurement object.
	 * @param sIdx   The index of the sensor to update.
	 */
	void setMeasurement(ConstMeasurementSptr &&store, int sIdx = 0) const {
		sens[sIdx]->meas = std::move(store);
	}
	/**
	 * Derived classes should create Sensor objects and place them in
	 * @a sens either in the constructor or an initialization function.
	 */
	GenericDevice() = default;
	/**
	 * Derived classes should create Sensor objects and place them in
	 * @a sens either in the constructor or an initialization function.
	 * @param id  The UUID for this device.
	 */
	GenericDevice(const boost::uuids::uuid &id) : Something(id) { }
public:
	/**
	 * Modifies member Sensor objects so they no longer have a pointer back to
	 * this object, and waits for calls those objects may be making to
	 * sharedPtr() to complete.
	 */
	~GenericDevice() {
		// prevent sensors from getting their Device
		for (auto s : sens) {
			s->dev = nullptr;
		}
		// wait for any stragglers
		while (destructWait > 0) {
			std::this_thread::yield();
		}
	}
	/**
	 * Returns a shared pointer to this device object; will be empty if this
	 * object's destructor is running.
	 */
	std::shared_ptr< GenericDevice< SVT, SQT, TVT, TQT > > sharedPtr() {
		// use this counter object to ensure this function returns before
		// this object's destructor concludes
		DestructCounter(destructWait);
		// should be empty if the destructor is running on another thread
		return std::static_pointer_cast< GenericDevice< SVT, SQT, TVT, TQT > >(
			Something::weak_from_this().lock()
		);
	}
	/**
	 * Samples the device and updates its sensor objects, but does not provide
	 * timestamps in the sensors' measurement objects.
	 * @todo  Is this needed, or should sample(const ClockSptr &) be called
	 *        with an empty shared pointer?
	 */
	virtual void sample() = 0;
	/**
	 * Samples the device and updates its sensor objects, and provides a
	 * timestamp from the given @a clock device.
	 * @param clock  The clock that will provide a timestamp for the
	 *               measurements taken by this device.
	 */
	virtual void sample(const ClockSptr &clock) = 0;
	/**
	 * Returns the number of sensors provided by this device.
	 */
	unsigned int numSensors() const {
		return sens.size();
	}
	/**
	 * Returns the sensor object at the given device specific index. The
	 * retuned object may outlive this device.
	 * @param sIdx  The sensor index.
	 * @return      The sensor object.
	 */
	const SensorSptr &sensor(unsigned int sIdx = 0) const {
		/** @todo  Throw exception if index is bad. */
		return sens.at(sIdx);
	}
	//Unit sensorUnits(int sIdx) const;
	/**
	 * Returns the measurement object for the sensor at the given device
	 * specific index.
	 * @param sIdx  The sensor index.
	 * @return      The current measurement object from the sensor.
	 */
	ConstMeasurementSptr currentMeasurement(unsigned int sIdx = 0) const {
		/** @todo  Throw exception if index is bad. */
		return sens.at(sIdx)->measurement();
	}
};

typedef GenericDevice<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> Device;

typedef std::shared_ptr<Device>  DeviceSptr;
typedef std::weak_ptr<Device>  DeviceWptr;


} } }
