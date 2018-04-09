/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/interface/I2c.hpp>
#include <duds/hardware/interface/Conversation.hpp>
//#include <duds/hardware/devices/DeviceErrors.hpp>
#include <duds/data/Quantity.hpp>
//#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/* *
 * Base class for all AMG88xx specific errors.
 */
//struct AMG88xxError : DeviceError { };

/* *
 * The received CRC value was not consistent with the message data.
 */
//struct AM2320CrcError : virtual AM2320Error, virtual duds::general::CrcError { };

/**
 * Support for the AMG88xx, a low resolution thermal camera.
 *
 * The first few frames captured may not be a good representation of what
 * the camera is looking at. It clears up quickly, though, and isn't totally
 * wrong, so using the first frame to initialize values for an exponential
 * moving average likely will work ok.
 *
 * The thermal image output is somewhat noisy, so something like an exponential
 * moving average may be needed to smooth out the data. The temperature from
 * the thermistor is much less noisy.
 *
 * The further an object is from the sensor, assuming it is still large enough
 * to cover whole pixels, the more the reported temperature will skew towards
 * the temperature of the sensor. This is readily noticeable within the
 * documented 5 meter range.
 *
 * The device is a bit finicky with how it switches between its sleep and normal
 * operation modes. Earlier in development, the code would sometimes fail to
 * convince the device to sample and result in either frames of all zero
 * temperature (Celsius) or all identical non-zero frames. The failure occurred
 * without any reported error, and the device reported its current operating
 * mode as normal (used for misidentified device check). While the current
 * code does not exhibit the problem, the failure may still be observed if
 * other software uses the device first without an intervening power cycle. A
 * second attempt at using the device may be enough to get past the issue.
 *
 * @author  Jeff Jackowski
 */
class AMG88xx : boost::noncopyable {
	/**
	 * The I2C communication interface.
	 */
	std::unique_ptr<duds::hardware::interface::I2c> com;
	/**
	 * Used to read in sampled data from the device.
	 */
	duds::hardware::interface::Conversation read;
	/**
	 * Temperature image.
	 */
	double img[8][8];
	/**
	 * Thermistor temperature.
	 */
	std::int16_t temp;
	/**
	 * Operating modes.
	 */
	enum Mode {
		/**
		 * Regular sampling at 1Hz or 10Hz.
		 */
		Normal,
		/**
		 * No sampling ; reduced power usage.
		 */
		Sleep,
		/**
		 * Only mention in documentation is "Stand-by mode (60sec intermittence)".
		 * Not used here.
		 */
		StandBy60,
		/**
		 * Only mention in documentation is "Stand-by mode (10sec intermittence)".
		 * Not used here.
		 */
		StandBy10
	};
	struct {
		/**
		 * Current operating mode.
		 */
		unsigned int mode : 2;
		/**
		 * Frame rate setting.
		 */
		unsigned int fps1Not10 : 1;
		/**
		 * True when the last read of the operating mode failed on error, or
		 * provided a result that did not match the operating mode that was just
		 * set. False after an attempt to change the operating mode succeeds.
		 * Used to avoid suspending the device in the constructor if the last
		 * operating mode change attempt fails in an attempt to prevent the next
		 * attempt at using the device from failing. Earlier in development,
		 * the device would work for one run of the test program, and fail on the
		 * next; the alternation of success and failure was consistent. I hoped
		 * this might help resolve the issue.
		 */
		unsigned int misid : 1;
	};
public:
	/**
	 * Attempts to reset the device and put it into sleep mode.
	 * @post     The device is suspended; no samples are being taken. The 10Hz
	 *           sampling rate is selected.
	 * @param c  The I2C interface to communicate with the device. The address
	 *           should be either 0x68 or 0x69.
	 */
	AMG88xx(std::unique_ptr<duds::hardware::interface::I2c> &c);
	/**
	 * Suspends the device operation (sleep mode) if a DeviceMisidentified error
	 * did not occur on the last start() call.
	 */
	~AMG88xx();
	/**
	 * Configures the device. The only option is the frame rate. Calling this
	 * function is not required. The 10Hz sampling rate is the default.
	 * @param fps1  True for 1Hz sampling rate.
	 */
	void configure(bool fps1);
	/**
	 * Configures the device for a 1Hz sampling rate.
	 */
	void oneFps() {
		configure(true);
	}
	/**
	 * Configures the device for a 10Hz sampling rate.
	 */
	void tenFps() {
		configure(false);
	}
	/**
	 * Transitions the device to normal operating mode so that it begins
	 * sampling.
	 * @post  The device will sample at the configured sampling rate.
	 * @throw DeviceMisidentified  The device failed to transition to the normal
	 *                             operating mode. This likely means the device
	 *                             is not an AMG88xx.
	 */
	void start();
	/**
	 * Same as start().
	 */
	void resume() {
		start();
	}
	/**
	 * Transitions the device to sleep mode.
	 * @post  The device will cease sampling.
	 */
	void suspend();
	/**
	 * Reads a sample from the device. This does not cause the device to sample;
	 * it does so at regular intervals based on its own clock. Reading a second
	 * time before the device takes the next sample will result in an identical
	 * sample to the last one read. Two sampled frames will never be mixed
	 * together.
	 * @pre  The device is in its normal operating mode. This is done by start().
	 */
	void sample();
	/**
	 * Image array type.
	 */
	typedef double double8x8[8][8];
	/**
	 * Provides direct access to the most recent sample.
	 * @warning  This is not thread-safe.
	 */
	const double8x8 &image() const {
		return img;
	}
	/**
	 * Returns the temperature of the device as reported by its thermistor.
	 */
	duds::data::Quantity temperature() const;
};

} } } }
