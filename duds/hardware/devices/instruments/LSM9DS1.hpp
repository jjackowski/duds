/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/I2c.hpp>
#include <duds/hardware/interface/Conversation.hpp>
#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/data/QuantityArray.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Base class for all LSM9DS1 specific errors.
 */
struct LSM9DS1Error : DeviceError { };
/**
 * The requested data rate is unsupported.
 */
struct LSM9DS1BadDataRate : LSM9DS1Error { };
/**
 * The requested maximum magnitude is either unsupported or an invalid value.
 */
struct LSM9DS1BadMagnitude : LSM9DS1Error { };
/**
 * The configuration requested that neither the accelerometer or magnetometer
 * be used.
 */
struct LSM9DS1NoInsturment : LSM9DS1Error { };

typedef boost::error_info<struct Info_UpdateRate, float>  RequestedUpdateRate;

/**
 * Initial support of the accelerometer and gyroscope on the LSM9DS1.
 *
 * The magnetometer is supported by LSM9DS1Mag because it acts like an
 * independent device. The documentation makes it seem as though the LSM9DS1
 * is really two devices stuck together.
 *
 * The Sparkfun board I've got uses I2C address 0x6B for the accelerometer
 * and gyroscope.
 *
 * @author  Jeff Jackowski
 */
class LSM9DS1AccelGyro : boost::noncopyable {
public:
	/**
	 * Stores the sample data as reported by the device. The accelerometer data
	 * will be shifted to place the LSb into the LSb of the integer here.
	 */
	union RawSample {
		std::int16_t vals[3];
		struct {
			std::int16_t x;
			std::int16_t y;
			std::int16_t z;
		};
	};
	/**
	 * The sample data converted to known units. The accelerometer data is a
	 * bit of a guess since its units are vaugely defined and callibration
	 * is done based on the local pull of Earth's gravity.
	 */
	typedef duds::data::QuantityXyz  ConvertedQuantity;
	/**
	 * The maximum magnitude options for the accelerometer.
	 * The ordering is strange, but that is how the device does it.
	 */
	enum AccelRange {
		/**
		 * Selects the +/-2g accelerometer range.
		 */
		AccelRange2g,
		/**
		 * Selects the +/-16g accelerometer range.
		 */
		AccelRange16g,
		/**
		 * Selects the +/-4g accelerometer range.
		 */
		AccelRange4g,
		/**
		 * Selects the +/-8g accelerometer range.
		 */
		AccelRange8g,
		AccelRange19m61ps2 = 0,  /**< 19.6133m/(s*s) */
		AccelRange156m9ps2,      /**< 156.9064m/(s*s) */
		AccelRange39m23ps2,      /**< 39.2266m/(s*s) */
		AccelRange78m45ps2,      /**< 78.4532m/(s*s) */
	};
	/**
	 * The maximum magnitude options for the gyroscope.
	 */
	enum GyroRange {
		GyroRange245dps,       /**< 245 degrees per second */
		GyroRange500dps,       /**< 500 degrees per second */
		GyroRange2000dps = 3,  /**< 2000 degrees per second */
		GyroRange4r276ps = 0,  /**< 4.276 radians per second */
		GyroRange8r727ps,      /**< 8.727 radians per second */
		GyroRange34r91ps = 3,  /**< 34.91 radians per second */
	};
	/**
	 * The various settings for the device packed into an integer to avoid
	 * passing a lot of parameters.
	 */
	struct Settings {
		/**
		 * Flag to use the accelerometer. At least the accelerometer or
		 * magnetometer must be selected for use.
		 */
		unsigned int accelerometer         : 1;
		/**
		 * Flag to use the gyroscope. Enabling the gyroscope requires enabling
		 * the accelerometer.
		 */
		unsigned int gyroscope             : 1;
		/**
		 * The maximum magnetude setting for the accelerometer.
		 */
		AccelRange accelRange              : 2;
		/**
		 * The maximum magnetude setting for the gyroscope.
		 */
		GyroRange gyroRange               : 2;
		/**
		 * Enable the low-power mode of the gyroscope. Not sure how this affects
		 * operation or how much of a difference it makes.
		 */
		unsigned int gyroLowPower         : 1;
		unsigned int gyroHighPass         : 1;
	};
private:
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> agcom;
	/**
	 * The conversation used to initialize the device. It is created in init()
	 * and held for later use by resume().
	 */
	duds::hardware::interface::Conversation initialize;
	/**
	 * The conversation used to read in samples from the device.
	 */
	duds::hardware::interface::Conversation agsample, statq;
	/**
	 * The values supplied by the device.
	 */
	RawSample accl, gyro;
	/**
	 * The currently configured sample rate.
	 */
	float agdatarate;
	/**
	 * The current various configuration options.
	 */
	Settings cfg;
	/**
	 * The data rate value given to the device.
	 */
	std::int8_t agdrval;
public:
	/**
	 * Attempts to identify the device, then suspends the device's operation.
	 * This resets the device, which requires 2ms to complete.
	 *
	 * Sparkfun's board uses device address 0x6B.
	 *
	 * @post         The device is suspended.
	 * @param i2c    The I2C object to use to communicate with the
	 *               accelerometer/gyroscope part of the device.
	 *               This LSM9DS1 object will take ownership over the
	 *               communicator.
	 * @post         The provided @a i2c unique_ptr no longer contains an
	 *               object upon success. If any exception is thrown, the
	 *               unique_ptr will retain the object.
	 * @throw DeviceMisidentified  The reported device ID is incorrect.
	 */
	LSM9DS1AccelGyro(
		std::unique_ptr<duds::hardware::interface::I2c> &i2c
	);
	/**
	 * Calls suspend().
	 */
	~LSM9DS1AccelGyro();
	/**
	 * Configures the device.
	 * @post  The device will be suspended.
	 * @param freq      The minimum sample frequency. If the rate is too high,
	 *                  LSM9DS1BadDataRate will be thrown.
	 * @param settings  The requested device configuration.
	 * @throw LSM9DS1NoInsturment  The configuration does not select either
	 *                                the accelerometer or magnetometer for
	 *                                use.
	 * @throw LSM9DS1BadMagnitude  The requested maximum magnetude for the
	 *                                accelerometer is either not supported or
	 *                                an invalid value.
	 * @throw LSM9DS1BadDataRate
	 */
	void configure(float freq, Settings settings);
	/**
	 * Tells the device to start sampling.
	 * @pre  configure() has been called and succeeded.
	 */
	void start() {
		// oops.
	}
	/**
	 * Suspends operation by putting the device into a low-power mode that
	 * discontinues sampling. This may require as much as 300ms to complete if
	 * the magnetometer is in use.
	 */
	void suspend();
	/**
	 * Resumes operation after a call to suspend().
	 * Seems to be a nice name that I used with other device drivers, so I
	 * put it here until I do more to figure out a consistent interface that
	 * I like.
	 * @throw DeviceUninitalized  init() hasn't been called.
	 */
	void resume() {
		start();
	}
	/**
	 * Reads sampled data from the device.
	 * @return  True if the device had new data.
	 */
	bool sample();
	/**
	 * Returns the confiured sampling rate. The value will be zero if
	 * configure() has not yet been called, or if it failed.
	 */
	float sampleRate() const {
		return agdatarate;
	}
	/**
	 * Returns the accelerometer data as read from the device. The data will
	 * be shifted, if needed, so that the LSb of the sample data will be in
	 * the LSb of the integers. Otherwise, the data is unchanged from what the
	 * device reported.
	 * @note  The values will change when the device is sampled and the
	 *        accelerometer is in use.
	 */
	const RawSample &rawAccelerometer() const {
		return accl;
	}
	const RawSample &rawGyroscope() const {
		return gyro;
	}
	/**
	 * Provides the accelerometer data in meters per second squared.
	 */
	void accelerometerQuantity(ConvertedQuantity &ps) const;
	/**
	 * Provides the gyroscope data in radians per second.
	 */
	void gyroscopeQuantity(ConvertedQuantity &ps) const;
};


/**
 * Initial support of the magnetometer of the LSM9DS1.
 *
 * The accelerometer and gyroscope are supported by LSM9DS1AccelGyro because
 * it acts like an independent device. The documentation makes it seem as
 * though the LSM9DS1 is really two devices stuck together.
 *
 * The axes for the magnetometer are not the same as the accelerometer and
 * gyroscope. The magnetometer's axes are the accelerometer's axes rotated
 * about the Z-axis 90 degrees such that the +X axis is rotated to the
 * accelerometer's -Y axis. This class will modify the magnetometer's data
 * to have the same axes because having different axes is bizarre.
 *
 * The Sparkfun board I've got uses I2C address 0x1E for the magnetometer.
 *
 * @author  Jeff Jackowski
 */
class LSM9DS1Mag : boost::noncopyable {
public:
	/**
	 * Stores the sample data as reported by the device. The accelerometer data
	 * will be shifted to place the LSb into the LSb of the integer here.
	 */
	union RawSample {
		std::int16_t vals[3];
		struct {
			std::int16_t x;
			std::int16_t y;
			std::int16_t z;
		};
	};
	/**
	 * The sample data converted to Tesla.
	 */
	typedef duds::data::QuantityXyz  ConvertedQuantity;
	/**
	 * The maximum magnitude options for the magnetometer.
	 */
	enum MagRange {
		MagRange400uT,   /**< 400 microtesla */
		MagRange800uT,   /**< 800 microtesla */
		MagRange1200uT,  /**< 1200 microtesla */
		MagRange1600uT   /**< 1600 microtesla */
	};
	/**
	 * Selects an operation mode for the axes. The documentation isn't clear
	 * on anything here, so take a guess as to what these options do.
	 */
	enum MagAxesMode {
		/**
		 * Use a low power mode. The relation with the low-power mode for the
		 * whole magnetometer isn't specified in the documentation.
		 */
		AxesLowPower,
		/**
		 * Called medium performance in the doc; renamed to avoid buzzwords.
		 */
		AxesLowPerformance,
		/**
		 * Called high performance in the doc; renamed to avoid buzzwords.
		 */
		AxesMediumPerformance,
		/**
		 * Buzzword high performance in the doc; renamed to avoid stupid.
		 */
		AxesHighPerformance
	};
	/**
	 * The various settings for the device packed into an integer to avoid
	 * passing a lot of parameters.
	 */
	struct Settings {
		/**
		 * Flag to use the magnetometer. At least the accelerometer or
		 * magnetometer must be selected for use.
		 */
		unsigned int magnetometer          : 1;
		/**
		 * The maximum magnetude setting.
		 */
		MagRange magRange             : 2;
		/**
		 * A low-power mode that forces the 0.625Hz sample rate. It is separate
		 * from the low-power mode on the axes, but the documentation makes it
		 * unclear if the axes mode makes any difference if this device low-power
		 * mode is used.
		 */
		unsigned int magLowPower          : 1;
		/**
		 * Operating mode for the X and Y axes.
		 */
		MagAxesMode xyMagMode             : 2;
		/**
		 * Operating mode for the Z axis.
		 */
		MagAxesMode zMagMode              : 2;
		/**
		 * Enables temperature compensation using a temperature sensor inside
		 * the device.
		 */
		unsigned int magTempComp          : 1;
	};
private:
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> magcom;
	/**
	 * The conversation used to initialize the device. It is created in init()
	 * and held for later use by resume().
	 */
	duds::hardware::interface::Conversation initialize;
	/**
	 * The conversation used to read in samples from the device.
	 */
	duds::hardware::interface::Conversation magsample, statq;
	/**
	 * The values supplied by the device.
	 */
	RawSample magn;
	/**
	 * The currently configured sample rate.
	 */
	float mdatarate;
	/**
	 * The current various configuration options.
	 */
	Settings cfg;
	/**
	 * The data rate value given to the device.
	 */
	std::int8_t mdrval;
public:
	/**
	 * Attempts to identify the device, then suspends the device's operation.
	 * This resets the device, which requires 2ms to complete.
	 *
	 * Sparkfun's board uses device address 0x1E.
	 *
	 * @post         The device is suspended.
	 * @param i2c    The I2C object to use to communicate with the magnetometer
	 *               part of the device. This LSM9DS1 object will take ownership
	 *               over the communicator.
	 * @post         The provided @a i2c unique_ptr no longer contains an
	 *               object upon success. If any exception is thrown, the
	 *               unique_ptr will retain the object.
	 * @throw DeviceMisidentified  The reported device ID is incorrect.
	 */
	LSM9DS1Mag(
		std::unique_ptr<duds::hardware::interface::I2c> &i2c
	);
	/**
	 * Calls suspend().
	 */
	~LSM9DS1Mag();
	/**
	 * Configures the device.
	 * @post  The device will be suspended.
	 * @param freq      The minimum sample frequency. If the rate is too high,
	 *                  LSM9DS1BadDataRate will be thrown.
	 * @param settings  The requested device configuration.
	 * @throw LSM9DS1NoInsturment  The configuration does not select either
	 *                                the accelerometer or magnetometer for
	 *                                use.
	 * @throw LSM9DS1BadMagnitude  The requested maximum magnetude for the
	 *                                accelerometer is either not supported or
	 *                                an invalid value.
	 * @throw LSM9DS1BadDataRate
	 */
	void configure(float freq, Settings settings);
	/**
	 * Tells the device to start sampling.
	 * @pre  configure() has been called and succeeded.
	 */
	void start() {
		// oops.
	}
	/**
	 * Suspends operation by putting the device into a low-power mode that
	 * discontinues sampling. This may require as much as 300ms to complete if
	 * the magnetometer is in use.
	 */
	void suspend();
	/**
	 * Resumes operation after a call to suspend().
	 * Seems to be a nice name that I used with other device drivers, so I
	 * put it here until I do more to figure out a consistent interface that
	 * I like.
	 * @throw DeviceUninitalized  init() hasn't been called.
	 */
	void resume() {
		start();
	}
	/**
	 * Reads sampled data from the device.
	 * @return  True if the device had new data.
	 */
	bool sample();
	/**
	 * Returns the confiured sampling rate. The value will be zero if
	 * configure() has not yet been called, or if it failed.
	 */
	float sampleRate() const {
		return mdatarate;
	}
	/**
	 * Returns the magnetometer data as read from the device.
	 * @note  The values will change when the device is sampled and the
	 *        magnetometer is in use.
	 */
	const RawSample &rawSample() const {
		return magn;
	}
	void quantity(ConvertedQuantity &ps) const;
};

} } } }
