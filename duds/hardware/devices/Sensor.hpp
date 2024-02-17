/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2024  Jeff Jackowski
 */
#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <duds/Something.hpp>
#include <duds/data/Measurement.hpp>

namespace duds { namespace hardware { namespace devices {

template<class SVT, class SQT, class TVT, class TQT>
class GenericDevice;

/*
Consider conversion of voltage to another unit.
Make function that can return Unit used in a GenericValue, or a Measurement
	containing a GenericValue.
*/

/**
 * Represents a sensor on a specific Device; allows access to measurements
 * without needing to use a Device object.
 *
 * @tparam SVT  Sample value type
 * @tparam SQT  Sample quality type
 * @tparam TVT  Time value type
 * @tparam TQT  Time quality type
 *
 * @author  Jeff Jackowski
 */
template<class SVT, class SQT, class TVT, class TQT>
class GenericSensor : public Something {
	friend class GenericDevice<SVT, SQT, TVT, TQT>;
public:
	/**
	 * The device type used by this sensor.
	 */
	typedef GenericDevice<SVT, SQT, TVT, TQT>  Device;
	/**
	 * A shared pointer type to the device type used by this sensor.
	 */
	typedef std::shared_ptr<Device>  DeviceSptr;
	/**
	 * A weak pointer type to the device type used by this sensor.
	 */
	typedef std::weak_ptr<Device>  DeviceWptr;
	/**
	 * The measurement type provided by the sensors of the parent device.
	 */
	typedef duds::data::GenericMeasurement<SVT, SQT, TVT, TQT>  Measurement;
	/**
	 * A shared pointer type to the measurement type used by this sensor.
	 */
	typedef std::shared_ptr<Measurement>  MeasurementSptr;
	/**
	 * A shared pointer type to the const measurement type used by this sensor.
	 */
	typedef std::shared_ptr<const Measurement>  ConstMeasurementSptr;
private:
	/**
	 * The owning device object. When the device is destructed, it will set
	 * this to nullptr.
	 */
	Device *dev;
	/**
	 * The current measurement object.
	 */
	ConstMeasurementSptr meas;
	/**
	 * The sensor index within the owning Device. Should this object be used
	 * without a Device, set this to -1.
	 * @note  This value must not change after this object is placed within a
	 *        Device.
	 * @todo  Remove above note if this value will remain private.
	 */
	unsigned int idx;
protected:
	struct Token { };
public:
	/**
	 * Make a sensor without setting the UUID.
	 * @protected @internal
	 * @param pdev  The owning device.
	 */
	GenericSensor(Device *pdev, Token) : dev(pdev) { }
	/**
	 * Make a new sensor.
	 * @protected @internal
	 * @param pdev  The owning device.
	 * @param id    The UUID for this sensor.
	 * @param i     The sensor's device specific index.
	 */
	GenericSensor(
		Device *pdev,
		const boost::uuids::uuid &id,
		unsigned int i,
		Token
	) : Something(id), dev(pdev), idx(i) { }
	/**
	 * Make a sensor without setting the UUID.
	 * @param pdev  The owning device.
	 */
	static std::shared_ptr< GenericSensor <SVT, SQT, TVT, TQT> > make(
		Device *pdev
	) {
		return std::make_shared< GenericSensor <SVT, SQT, TVT, TQT> >(
			pdev, Token()
		);
	}
	/**
	 * Make a new sensor.
	 * @param pdev  The owning device.
	 * @param id    The UUID for this sensor.
	 * @param i     The sensor's device specific index.
	 */
	static std::shared_ptr< GenericSensor <SVT, SQT, TVT, TQT> > make(
		Device *pdev,
		const boost::uuids::uuid &id,
		unsigned int i
	) {
		return std::make_shared< GenericSensor <SVT, SQT, TVT, TQT> >(
			pdev, id, i, Token()
		);
	}
	/**
	 * Returns the sensor's device specific index.
	 */
	unsigned int index() const {
		return idx;
	}
	/**
	 * Returns a shared pointer to the owning device.
	 * @return  A shared pointer to the device. If the device object has been
	 *          destroyed, this will be an empty shared pointer. This object
	 *          does @b not contain a shared pointer to the device.
	 */
	DeviceSptr device() const {
		Device *d = dev;
		if (d) {
			return d->sharedPtr();
		}
		return DeviceSptr();
	}
	/**
	 * Returns the current measurement object. The measurement object will be
	 * replaced when the device is sampled, so repeated calls to this function
	 * can return different objects.
	 */
	ConstMeasurementSptr &measurement() const {
		return meas;
	}
};

typedef GenericSensor<
	duds::data::GenericValue,
	double,
	duds::time::interstellar::NanoTime,
	float
> Sensor;

typedef std::shared_ptr<Sensor>  SensorSptr;
typedef std::weak_ptr<Sensor>  SensorWptr;

} } }

#endif        //  #ifndef SENSOR_HPP
