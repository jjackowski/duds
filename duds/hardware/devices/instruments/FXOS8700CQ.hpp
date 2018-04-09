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
 * Base class for all FXOS8700CQ specific errors.
 */
struct FXOS8700CQError : DeviceError { };
/**
 * The requested data rate is unsupported.
 */
struct FXOS8700CQBadDataRate : FXOS8700CQError { };
/**
 * The requested maximum magnitude is either unsupported or an invalid value.
 */
struct FXOS8700CQBadMagnitude : FXOS8700CQError { };
/**
 * The configuration requested that neither the accelerometer or magnetometer
 * be used.
 */
struct FXOS8700CQNoInsturment : FXOS8700CQError { };

typedef boost::error_info<struct Info_UpdateRate, float>  RequestedUpdateRate;

/**
 * Initial support of the FXOS8700CQ; a combined triple axis accelerometer and
 * magnetometer.
 *
 * Errata document says any communication over I2C will adversely affect any
 * magnetometer sample by as much as 70uT. This likely will be triggered by
 * any use of the I2C bus rather than just communication with the FXOS8700CQ.
 *
 * Adafruit's board uses device address 0x1F. When using the board with
 * the I2C bus on HDMI from a notebook computer, I found that the reset
 * operation causes the device to ignore all communication until after a
 * power cycle. Not sure why. I also found that with that setup and certain
 * almost but not quite level orientations, the device fails to register
 * gravity. It also has issues with the magnetometer data. Using the same
 * device with a Raspberry Pi resulted in good accelerometer data, but
 * bad magnetometer data. Could just be an issue with the board I have.
 *
 * @author  Jeff Jackowski
 */
class FXOS8700CQ : boost::noncopyable {
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
	typedef duds::data::QuantityXyz  ConvertedSample;
	/**
	 * The magnitude options for the accelerometer.
	 */
	enum Magnitude {
		/**
		 * Selects the +/-2g accelerometer range.
		 */
		Magnitude2g,
		/**
		 * Selects the +/-4g accelerometer range.
		 */
		Magnitude4g,
		/**
		 * Selects the +/-8g accelerometer range.
		 * Cannot be used with the low-noise operating mode.
		 */
		Magnitude8g
	};
	enum OversampleMode {
		Normal,
		LowNoiseLowPower,
		HighResolution,
		LowPower
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
		 * Flag to use the magnetometer. At least the accelerometer or
		 * magnetometer must be selected for use.
		 */
		unsigned int magnetometer          : 1;
		/**
		 * Use the low-noise mode of the accelerometer. This cannot be used
		 * with the Magnitude8g rage.
		 */
		unsigned int accelLowNoise         : 1;
		/**
		 * Enables the high-pass filter.
		 */
		unsigned int highPassFilter        : 1;
		/**
		 * Adjusts the cut-off frequency of the high-pass filter to be lower.
		 */
		unsigned int highPassLowCutoff     : 1;
		/**
		 * The maximum magnetude for the accelerometer.
		 */
		Magnitude maxMagnitude             : 2;
		/**
		 * The oversample mode to use when in the active non-sleep state.
		 */
		OversampleMode oversampleMode      : 2;
		/**
		 * The oversample mode to use when in the sleep state.
		 * @note  Use of the sleep mode is not yet supported by this driver.
		 */
		OversampleMode oversampleSleepMode : 2;
		/**
		 * Affects how many samples are taken by the magnetometer to produce
		 * a single output sample. Lower values decrease power consumption.
		 * Higher values decrease noise. The higher the sampling rate, the less
		 * this value makes a difference.
		 */
		unsigned int oversampleRatio       : 3;
		/**
		 * Uses a thread to periodically read samples from the device.
		 * @note  UnimplementedError.
		 * @todo  Implement a thread for reading.
		 */
		unsigned int threadedSample        : 1;
	};
private:
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> com;
	/**
	 * The conversation used to initialize the device. It is created in init()
	 * and held for later use by resume().
	 */
	duds::hardware::interface::Conversation initialize;
	/**
	 * The conversation used to read in samples from the device.
	 */
	duds::hardware::interface::Conversation input, bufq;
	/**
	 * The values supplied by the device.
	 */
	RawSample accl, magn;
	/**
	 * The currently configured sample rate.
	 */
	float datarate;
	/**
	 * The current various configuration options.
	 */
	Settings cfg;
	/**
	 * The data rate value given to the device.
	 */
	std::int8_t drval;
public:
	/**
	 * Attempts to identify the device, then suspends the device's operation.
	 * This resets the device, which requires 2ms to complete.
	 *
	 * Adafruit's board uses device address 0x1F.
	 *
	 * @param i2ccom   The I2C object to use to communicate with the device.
	 *                 This FXOS8700CQ object will take ownership over
	 *                 @a i2ccom.
	 * @throw DeviceMisidentified  The reported device ID is incorrect.
	 */
	FXOS8700CQ(std::unique_ptr<duds::hardware::interface::I2c> &i2ccom);
	/**
	 * Calls suspend().
	 */
	~FXOS8700CQ();
	/**
	 * Configures the device.
	 * @post  The device will be suspended.
	 * @param freq      The minimum sample frequency. If the rate is too high,
	 *                  FXOS8700CQBadDataRate will be thrown.
	 * @param settings  The requested device configuration.
	 * @throw FXOS8700CQNoInsturment  The configuration does not select either
	 *                                the accelerometer or magnetometer for
	 *                                use.
	 * @throw FXOS8700CQBadMagnitude  The requested maximum magnetude for the
	 *                                accelerometer is either not supported or
	 *                                an invalid value.
	 * @throw FXOS8700CQBadDataRate
	 */
	void configure(float freq, Settings settings);
	/**
	 * Tells the device to start sampling.
	 * @pre  configure() has been called and succeeded.
	 */
	void start();
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
		return datarate;
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
	/**
	 * Returns the magnetometer data as read from the device.
	 * @note  The values will change when the device is sampled and the
	 *        magnetometer is in use.
	 */
	const RawSample &rawMagnetometer() const {
		return magn;
	}
};

} } } }
