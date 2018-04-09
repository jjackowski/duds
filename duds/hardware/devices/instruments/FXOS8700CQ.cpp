/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/FXOS8700CQ.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>
#include <thread>

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Prevent Doxygen from mixing this up with other source-file specific items
 * from other files.
 * @internal
 */
namespace FXOS8700CQ_internal {

/**
 * The registers. They have been renamed to be more readable, understandable,
 * and consistent. The names in the documentation have the odd property of
 * explicitly stating when something is for the magnetometer, but not always
 * doing the same for the accelerometer.
 */
enum Regs {
	/**
	 * STATUS. Holds either accelerometer data status, or FIFO status.
	 */
	RegStatus,
	/**
	 * OUT_X_MSB. This is the start of the latest accelerometer sample when the
	 * FIFO is not used, and the start of the oldest samples when the FIFO is
	 * used. When both the accelerometer and magnetometer are in use, samples
	 * from both are read starting from here. The documentation states in a
	 * terse summary text that the FIFO buffer is for accelerometer data only,
	 * but the in-depth documentation on reading from the FIFO seems to imply
	 * that the magnetometer data is also buffered mostly because it doesn't
	 * restate that the FIFO isn't used for the magnetometer, and overlooks this
	 * detail in its otherwise detailed description of address increments. The
	 * only way the documentation is correct is if reading from the FIFO is
	 * followed by reading the latest magnetometer sample, in which case the two
	 * samples may not have been taken consecutively.
	 */
	RegSamples,
	RegFifoConfig = 9,
	RegFifoTrigger,
	RegSystemMode,  /**< SYSMOD, the system operating mode and FIFO error status. */
	RegIntFlags,
	RegDeviceId,
	RegAccelConfig,  /**< XYZ_DATA_CFG */
	RegFilterConfig, /**< HP_FILTER_CUTOFF */
	RegConfig1 = 0x2A,  // the answer should have started at zero
	RegConfig2,
	RegConfig3,
	RegConfig4,
	RegConfig5,
	RegAccelOffsetX,  /**< OFF_X */
	RegAccelOffsetY,
	RegAccelOffsetZ,
	RegMagStatus,
	/**
	 * M_OUT_X_MSB. This is the start of the latest magnetometer sample. It is
	 * followed by the accelerometer sample that the documentation describes
	 * as "time aligned", which I suppose means they were taken consecutively.
	 * This may make the accelerometer sample older than the one at RegSamples,
	 * but by no more than one sample period.
	 */
	RegMagSample,
	RegMagOffsetX = 0x3F,
	RegMagOffsetY = 0x41,
	RegMagOffsetZ = 0x43,
	RegMagConfig1 = 0x5B,
	RegMagConfig2,
	RegMagConfig3
};

}

using namespace FXOS8700CQ_internal;

// Device uses big-endian data unaligned to address value

FXOS8700CQ::FXOS8700CQ(std::unique_ptr<duds::hardware::interface::I2c> &i2ccom) :
com(std::move(i2ccom)), datarate(0) {
	try {
		duds::hardware::interface::Conversation firstcon;
		// verify ID
		firstcon.addOutputVector() << (std::int8_t)RegDeviceId;
		firstcon.addInputVector(1);
		com->converse(firstcon);
		duds::hardware::interface::ConversationExtractor ex(firstcon);
		std::uint8_t id;
		ex >> id;
		if (id != 0xC7) {
			DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
				duds::hardware::interface::I2cDeviceAddr(com->address())
			);
		}
		// make the device not active
		cfg.magnetometer = 1;  // in case magnetometer was in use; takes longer
		suspend();
		// attempt reset
		// This reset makes the device stop responding until a power cycle under
		// certain conditions met by connecting the device to my notebook
		// computer's I2C bus on its video output. Works fine on a Raspberry Pi.
		firstcon.clear();
		firstcon.addOutputVector() << (std::uint8_t)(RegConfig2) <<
			(std::uint8_t)(0x40);
		try {
			// the reset causes the device to not ack the message
			com->converse(firstcon);
		} catch (duds::hardware::interface::I2cErrorNoDevice &) {
			// bother; ignore it
		} catch (...) {
			// could be a real issue, but should be unlikely to occur
			throw;
		}
		// give the device time to complete the reset
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	} catch (...) {
		// move the I2C communicator back
		i2ccom = std::move(com);
		throw;
	}
}

FXOS8700CQ::~FXOS8700CQ() {
	suspend();
}

// search for double freq if both accelerometer and magnetometer will run
static const std::map<float, int> DataRateVals = {
	{ 800.0,    0 },
	{ 400.0,    1 },
	{ 200.0,    2 },
	{ 100.0,    3 },
	{  50.0,    4 },  // Sleep data rate is limited to 50Hz.
	{  12.5,    5 },  // Subtract 4 from value to get sleep ODR value.
	{   6.25,   6 },
	{   1.5625, 7 },
};

void FXOS8700CQ::configure(
	float freq,
	Settings settings
) {
	// one of the instruments must be enabled
	if (!settings.accelerometer && !settings.magnetometer) {
		DUDS_THROW_EXCEPTION(FXOS8700CQNoInsturment());
	}
	if ( // low noise mode cannot be used with 8g range
		(settings.accelLowNoise && (settings.maxMagnitude == Magnitude8g)) ||
		// only thre valid values in 2 bits
		(settings.maxMagnitude == 3)
	) {
		DUDS_THROW_EXCEPTION(FXOS8700CQBadMagnitude());
	}
	{ // sample rate configuration
		float f = freq;
		// the update rate is halved when both the accelerometer and
		// magnetometer are used together
		if (settings.accelerometer && settings.magnetometer) {
			f *= 2;
		}
		// find the supported rate that is at least as great as the requested
		// rate
		std::map<float, int>::const_iterator dv = DataRateVals.lower_bound(f);
		if (dv == DataRateVals.cend()) {
			DUDS_THROW_EXCEPTION(
				FXOS8700CQBadDataRate() << RequestedUpdateRate(freq)
			);
		}
		drval = dv->second;
		datarate = dv->first;
		if (settings.accelerometer && settings.magnetometer) {
			datarate /= 2;
		}
	}
	// prevent device from doing much prior to changing config
	suspend();  // ensures it is inactive

	// the code below could be cleaned up, but until the device gives good
	// data, the code may be wrong, so don't worry about cleaner or more
	// optimal code yet

	// start with magnetometer config
	duds::hardware::interface::Conversation conv;
	std::uint8_t tmpregs[4] = {
		(std::uint8_t)(settings.oversampleRatio << 2),
		0x24,   // guess for degauss freq
		(std::uint8_t)(((settings.oversampleRatio/2) << 4) | 0x80),
		0
	};
	if (settings.magnetometer) {
		tmpregs[0] |= 1;
		if (settings.accelerometer) {
			tmpregs[0] |= 2;
		}
	}
	// set the three magnetometer control registers; they have some other stuff
	conv.addOutputVector() <<
		(std::uint8_t)(RegMagConfig1) << tmpregs[0] << tmpregs[1] << tmpregs[2];
	com->converse(conv);
	conv.clear();
	// accelerometer control reg
	tmpregs[0] = settings.maxMagnitude;
	tmpregs[1] = 0;
	if (settings.highPassFilter) {
		tmpregs[0] |= 0x10;
		// high-pass filter cut-off
		if (settings.highPassLowCutoff) {
			tmpregs[1] = 1;
		}
	}
	conv.addOutputVector() <<
		(std::uint8_t)RegAccelConfig << tmpregs[0] << tmpregs[1];
	com->converse(conv);
	conv.clear();
	// configuration reg 2
	tmpregs[0] = settings.oversampleMode | (settings.oversampleSleepMode << 3);
	conv.addOutputVector() <<
		(std::uint8_t)(RegConfig2) << tmpregs[0];
	com->converse(conv);
	/*
	tmpregs[0] = 0xC0 | (drval << 3);
	if (settings.accelLowNoise) {
		tmpregs[0] |= 4;
	}
	tmpregs[1] = settings.oversampleMode | (settings.oversampleSleepMode << 3);
	conv.addOutputVector() <<
		(std::uint8_t)(RegConfig1) << tmpregs[0] << tmpregs[1];
	com->converse(conv);
	*/
	conv.clear();

	// no FIFO
	conv.addOutputVector() <<
		(std::uint8_t)(RegFifoConfig) << (std::uint8_t)0;
	com->converse(conv);

	cfg = settings;
	// make the conversation to read in the device's sample data
	bufq.clear();
	if (settings.magnetometer) {
		bufq.addOutputVector() << (std::uint8_t)RegMagStatus;
	} else {
		bufq.addOutputVector() << (std::uint8_t)RegStatus;
	}
	bufq.addInputVector(1);

	input.clear();
	if (settings.magnetometer) {
		// prefer to read from the magnetometer side to get the "time aligned"
		// accelerometer sample, because that sounds nice, like it might mean
		// something useful
		input.addOutputVector() << (std::uint8_t)RegMagSample;
		if (settings.accelerometer) {
			input.addInputVector(12).bigEndian(true);
		} else {
			input.addInputVector(6).bigEndian(true);
		}
	} else {
		input.addOutputVector() << (std::uint8_t)(RegSamples);
		input.addInputVector(6).bigEndian(true);
	}
	/*
	if (settings.accelerometer) {
		input.addOutputVector() << (std::uint8_t)(RegSamples);
		if (settings.magnetometer) {
			input.addInputVector(12).bigEndian(true);
		} else {
			input.addInputVector(6).bigEndian(true);
		}
	} else {
		input.addOutputVector() << (std::uint8_t)(RegMagSample);
		input.addInputVector(6).bigEndian(true);
	}
	*/
}

void FXOS8700CQ::start() {
	duds::hardware::interface::Conversation conv;
	// configuration reg 1
	// do this after sending configuration reg 2 so that the device transitions
	// to its active state after having sent the complete configuration
	std::uint8_t tmpreg = 0xC1 | (drval << 3);
	if (cfg.accelLowNoise) {
		tmpreg |= 4;
	}
	// this will cause device to begin sampling
	// if FIFO is used, it will start to fill, so should start thread to read it
	// prior to sending it
	conv.addOutputVector() <<
		(std::uint8_t)RegConfig1 << tmpreg;
	com->converse(conv);
}

void FXOS8700CQ::suspend() {
	duds::hardware::interface::Conversation conv;
	conv.addOutputVector() <<
		// command to start writing at enable register
		(std::uint8_t)RegConfig1 <<
		// make it stop
		(std::uint8_t)(0);
	com->converse(conv);
	// using the magnetometer requires more time to suspend; delay here to avoid
	// adding check to start()
	if (cfg.magnetometer) {
		conv.clear();
		conv.addOutputVector() << (std::int8_t)RegSystemMode;
		conv.addInputVector(1);
		duds::hardware::interface::ConversationExtractor ex;
		std::int8_t modestat;
		do {
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
			com->converse(conv);
			ex.reset(conv);
			ex >> modestat;
		} while (modestat & 3);
	}
}

bool FXOS8700CQ::sample() {
	com->converse(bufq);
	duds::hardware::interface::ConversationExtractor ex(bufq);
	std::int8_t status;
	ex >> status;
	// have new samples for all axes?
	if ((status & 7) == 7) {
		// read them
		com->converse(input);
		// parse input
		duds::hardware::interface::ConversationExtractor ex(input);
		if (cfg.magnetometer) {
			ex >> magn.x >> magn.y >> magn.z;
		}
		if (cfg.accelerometer) {
			ex >> accl.x >> accl.y >> accl.z;
			// accelerometer data is read from different registers depending
			// on the use of the magnetometer
			if (cfg.magnetometer) {
				// the 2 MSb's may not be set properly for a 16-bit signed int
				accl.x = duds::general::SignExtend<14>(accl.x);
				accl.y = duds::general::SignExtend<14>(accl.y);
				accl.z = duds::general::SignExtend<14>(accl.z);
			} else {
				// the registers hold the number shifted so that the 8 most
				// significant bits are all in the same byte, leaving 2 bits
				// in the least significant byte unused; shift them to place
				// properly in 16-bit ints
				accl.x >>= 2;
				accl.y >>= 2;
				accl.z >>= 2;
			}
		}
		return true;
	}
	return false;
}


} } } }
