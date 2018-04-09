/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/LSM9DS1.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>
#include <duds/data/Constants.hpp>
#include <thread>

#include <iostream>  // for debugging

namespace duds { namespace hardware { namespace devices { namespace instruments {

/**
 * Prevent Doxygen from mixing this up with other source-file specific items
 * from other files.
 * @internal
 */
namespace LSM9DS1_internal {

/**
 * The registers for the accelerometer and gyroscope device.
 * They have been renamed to be more readable, understandable, and consistent
 * with other code in this library.
 */
enum AccelGyroRegs {
	AgrDeviceId = 0xF,  // stores 0x68
	AgrGyroConfig1,  /**< CTRL_REG1_G */
	AgrGyroConfig2,  /**< CTRL_REG2_G */
	AgrGyroConfig3,  /**< CTRL_REG3_G */
	AgrTemp = 0x15,
	/**
	 * A general status register; same data as AgrStatusPreAccel.
	 */
	AgrStatusPreGyro = 0x17,
	AgrGyroSampleX,         /**< OUT_X_G */
	AgrGyroSampleY = 0x1A,  /**< OUT_Y_G */
	AgrGyroSampleZ = 0x1C,  /**< OUT_Z_G */
	AgrGyroConfig4 = 0x1E,  /**< CTRL_REG4 */  // don't change its contents
	AgrAccelConfig5,        /**< CTRL_REG5_XL */
	AgrAccelConfig6,        /**< CTRL_REG6_XL */
	AgrAccelConfig7,        /**< CTRL_REG7_XL */
	AgrConfig8,             /**< CTRL_REG8 */
	AgrConfig9,             /**< CTRL_REG9 */
	/**
	 * A general status register; same data as AgrStatusPreGyro.
	 */
	AgrStatusPreAccel = 0x27,
	AgrAccelSampleX,         /**< OUT_X_XL */
	AgrAccelSampleY = 0x2A,  /**< OUT_Y_XL */
	AgrAccelSampleZ = 0x2C,  /**< OUT_Z_XL */
	AgrFifoConfig = 0x2E,    /**< FIFO_CTRL */
	AgrFifoStatus            /**< FIFO_SRC */
};

/**
 * The registers for the magnetometer device.
 * They have been renamed to be more readable, understandable, and consistent
 * with other code in this library.
 */
enum MagRegs {
	MagOffsetX = 5,
	MagOffsetY = 7,
	MagOffsetZ = 9,
	MagDeviceId = 0xF,  // stores 0x3D
	MagConfig1 = 0x20,  /**< CTRL_REG1_M */
	MagConfig2,         /**< CTRL_REG2_M */
	MagConfig3,         /**< CTRL_REG3_M */
	MagConfig4,         /**< CTRL_REG4_M */
	MagConfig5,         /**< CTRL_REG5_M */
	MagStatus = 0x27,   /**< STATUS_REG_M */
	MagSampleX,         /**< OUT_X_L_M */
	MagSampleY = 0x2A,  /**< OUT_Y_L_M */
	MagSampleZ = 0x2C   /**< OUT_Z_L_M */
};

/**
 * Available sampling rates for the accelerometer when the gyroscope is not
 * in use.
 */
static const std::map<float, int> AccelDataRateVals = {
	{   0,    0 },  // use to suspend sampling
	{  10.0,  1 },
	{  50.0,  2 },
	{ 119.0,  3 },
	{ 238.0,  4 },
	{ 476.0,  5 },
	{ 952.0,  6 }
};

/**
 * Available sampling rates for the accelerometer and gyroscope when both are
 * in use.
 */
static const std::map<float, int> AccelGyroDataRateVals = {
	{   0,    0 },  // use to suspend sampling
	{  14.9,  1 },
	{  59.5,  2 },
	{ 119.0,  3 },
	{ 238.0,  4 },
	{ 476.0,  5 },
	{ 952.0,  6 }
};

/**
 * Sample rates for the magnetometer.
 */
static const std::map<float, int> MagDataRateVals = {
	{  0.625, 0 },  // low power (device) mode always uses this rate
	{  1.25,  1 },
	{  2.5,   2 },
	{  5.0,   3 },
	{ 10.0,   4 },
	{ 20.0,   5 },
	{ 40.0,   6 },
	{ 80.0,   7 }
};

/**
 * Finds the lowest data rate that is at least as fast as requested, and the
 * value needed to select this rate with the device. No exception is thrown
 * so that the caller may throw an object based on which data rate has trouble.
 * @param destVal     Will be set to the value to send to the device to
 *                    select the data rate.
 * @param actualRate  Will be set to the actual rate to use if one is found.
 * @param reqRate     The requested data rate. The actual rate may be greater,
 *                    but will not be lower.
 * @param rateMap     The map containing the data rates and values.
 * @return   True if a rate was found, false if the requested rate is faster
 *           than all supported rates. If false, @a destVal and @a actualRate
 *           will not be altered.
 * @todo     Make something generic like this.
 */
static bool matchDataRate(
	std::int8_t &destVal,
	float &actualRate,
	float reqRate,
	const std::map<float, int> &rateMap
) {
	std::map<float, int>::const_iterator dri = rateMap.lower_bound(reqRate);
	if (dri == rateMap.cend()) {
		return false;
	}
	destVal = dri->second;
	actualRate = dri->first;
	return true;
}

// documentation uses milli-G; converted here to m/(s*s)
static const double AccelScaleToUnits[] = {  // also resolution
	0.061 * 1e-3 * duds::data::constants::EarthSurfaceGravity.value,
	0.732 * 1e-3 * duds::data::constants::EarthSurfaceGravity.value,
	0.122 * 1e-3 * duds::data::constants::EarthSurfaceGravity.value,
	0.244 * 1e-3 * duds::data::constants::EarthSurfaceGravity.value
};

// documentation uses millidegrees per second; convert to radians per second
static const double GyroScaleToUnits[] = {  // also resolution
	8.75 * 1e-3 * M_PI / 180.0,
	17.5 * 1e-3 * M_PI / 180.0,
	0, // bogus; index not used by the device
	70.0 * 1e-3 * M_PI / 180.0
};

// documentation uses milli(?)gauss; converted here to Tesla
static const double MagScaleToUnits[] = {  // also resolution
	0.14 * 1e-7,
	0.29 * 1e-7,
	0.43 * 1e-7,
	0.58 * 1e-7
};

}

using namespace LSM9DS1_internal;

// Device uses little-endian data, some unaligned on magnetometer,
// to address value

// ----- Accelerometer and gyroscope -----

LSM9DS1AccelGyro::LSM9DS1AccelGyro(
	std::unique_ptr<duds::hardware::interface::I2c> &i2c
) : agcom(std::move(i2c)), agdatarate(0)
{
	try {
		duds::hardware::interface::Conversation firstcon;
		// verify ID
		firstcon.addOutputVector() << (std::int8_t)AgrDeviceId;
		firstcon.addInputVector(1);
		agcom->converse(firstcon);
		duds::hardware::interface::ConversationExtractor ex(firstcon);
		std::uint8_t id;
		ex >> id;
		if (id != 0x68) {
			DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
				duds::hardware::interface::I2cDeviceAddr(agcom->address())
			);
		}
		firstcon.clear();
		// reset
		firstcon.addOutputVector() << (std::int8_t)AgrConfig8 <<
			// Set the BOOT and SW_RESET flags, since they both seem to be a
			// reset of the device, and the docs aren't specific on the
			// differences. Also, set the IF_ADD_INC (address increment)
			// flag. Should be set by default; keep it set.
			(std::int8_t)0x85;
		try {
			agcom->converse(firstcon);
			std::cout << "LSM9DS1AccelGyro accel reset; got ack ?!?" << std::endl;
		} catch (duds::hardware::interface::I2cErrorNoDevice &) {
			// bother; ignore it
			//std::cout << "LSM9DS1AccelGyro accel reset; no ack" << std::endl;
		}
		// give the device(s) time to complete the reset
		// big guess here
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	} catch (...) {
		// move the I2C communicator back
		i2c = std::move(agcom);
		throw;
	}
	// devices should be suspended by the reset operations
}

LSM9DS1AccelGyro::~LSM9DS1AccelGyro() {
	suspend();
}

void LSM9DS1AccelGyro::configure(float freq, Settings settings) {
	// one of the instruments must be enabled
	if (!settings.accelerometer && !settings.gyroscope) {
		DUDS_THROW_EXCEPTION(LSM9DS1NoInsturment());
	}
	// gyroscope can only be used if the accelerometer is used
	if (settings.gyroscope) {
		// provide something close to the request
		settings.accelerometer = 1;
	}
	// invalid value, two isn't used by the device
	if (settings.gyroRange == 2) {
		DUDS_THROW_EXCEPTION(LSM9DS1BadMagnitude());
	}
	// accelerometer data rate
	if (!settings.accelerometer) {
		freq = 0;
	} else if (
		// the sampling rate cannot be negative or zero
		(freq <= 0) ||
		(  // data rate for accelerometer only
			!settings.gyroscope &&
			!matchDataRate(agdrval, agdatarate, freq, AccelDataRateVals)
		) ||
		(  // data rate for accelerometer and gyroscope
			settings.gyroscope &&
			!matchDataRate(agdrval, agdatarate, freq, AccelGyroDataRateVals)
		)
	) {
		// the sampling rate cannot be negative or zero
		DUDS_THROW_EXCEPTION(
			LSM9DS1BadDataRate() << RequestedUpdateRate(freq)
		);
	}
	// prevent device from doing much prior to changing config
	suspend();  // ensures it is inactive

	duds::hardware::interface::Conversation conv;
	// start with accel/gyro; in power-down state after suspend()
	if (settings.accelerometer) {
		// no FIFO; already done by reset
		// config gyro
		std::uint8_t tmpregs[3] = {
			(std::uint8_t)((agdrval << 5) | settings.gyroRange),
			0,
			(std::uint8_t)(
				(settings.gyroLowPower << 7) |
				(settings.gyroHighPass << 6)
			)
		};
		conv.addOutputVector() << (std::uint8_t)AgrGyroConfig1 <<
			tmpregs[0] << tmpregs[1] << tmpregs[2];
		agcom->converse(conv);
		conv.clear();
		// config accel
		tmpregs[0] = (agdrval << 5) | (settings.accelRange << 3);
		conv.addOutputVector() << (std::uint8_t)AgrAccelConfig6 << tmpregs[0];
		agcom->converse(conv);
		conv.clear();
		// gyro sleep
		if (!settings.gyroscope) {
			conv.addOutputVector() << (std::uint8_t)AgrConfig9 <<
				(std::uint8_t)0x40;
			agcom->converse(conv);
			conv.clear();
		}
	}
	cfg = settings;

	// make the conversation to read in the device's status
	statq.clear();
	statq.addOutputVector() << (std::uint8_t)AgrStatusPreAccel;
	statq.addInputVector(1);
	// conversation to read accelerometer and gyroscope samples
	agsample.clear();
	if (settings.accelerometer) {
		if (settings.gyroscope) {
			agsample.addOutputVector() << (std::uint8_t)AgrGyroSampleX;
			agsample.addInputVector(12);
		} else {
			agsample.addOutputVector() << (std::uint8_t)AgrAccelSampleX;
			agsample.addInputVector(6);
		}
	}
}

void LSM9DS1AccelGyro::suspend() {
	duds::hardware::interface::Conversation conv;
	// for accel & gyro, set sample rates (ODR) to zero
	conv.addOutputVector() << (std::uint8_t)AgrGyroConfig1 << (std::uint8_t)0;
	(
		conv.addOutputVector() << (std::uint8_t)AgrAccelConfig6 <<
		(std::uint8_t)0
	).breakBefore();
	agcom->converse(conv);
}

bool LSM9DS1AccelGyro::sample() {
	if (cfg.accelerometer) {
		agcom->converse(statq);
		duds::hardware::interface::ConversationExtractor ex(statq);
		std::int8_t status;
		ex >> status;
		if (status & 3) {
			agcom->converse(agsample);
			ex.reset(agsample);
			if (cfg.gyroscope) {
				ex >> gyro.x >> gyro.y >> gyro.z;
			}
			ex >> accl.x >> accl.y >> accl.z;
			return true;
		}
	}
	return false;
}

void LSM9DS1AccelGyro::accelerometerQuantity(ConvertedQuantity &ps) const {
	ps.x() = accl.x * AccelScaleToUnits[cfg.accelRange];
	ps.y() = accl.y * AccelScaleToUnits[cfg.accelRange];
	ps.z() = accl.z * AccelScaleToUnits[cfg.accelRange];
	ps.unit = duds::data::constants::EarthSurfaceGravity.unit;
}

void LSM9DS1AccelGyro::gyroscopeQuantity(ConvertedQuantity &ps) const {
	ps.x() = gyro.x * GyroScaleToUnits[cfg.gyroRange];
	ps.y() = gyro.y * GyroScaleToUnits[cfg.gyroRange];
	ps.z() = gyro.z * GyroScaleToUnits[cfg.gyroRange];
	ps.unit = duds::data::units::Radian / duds::data::units::Second;
}

// ----- Magnetometer -----

LSM9DS1Mag::LSM9DS1Mag(
	std::unique_ptr<duds::hardware::interface::I2c> &i2c
) : magcom(std::move(i2c)), mdatarate(0)
{
	try {
		duds::hardware::interface::Conversation firstcon;
		// verify ID
		firstcon.addOutputVector() << (std::int8_t)MagDeviceId;
		firstcon.addInputVector(1);
		// test the accelerometer/gyroscope first
		magcom->converse(firstcon);
		duds::hardware::interface::ConversationExtractor ex(firstcon);
		std::uint8_t id;
		ex >> id;
		if (id != 0x3D) {
			DUDS_THROW_EXCEPTION(DeviceMisidentified() <<
				duds::hardware::interface::I2cDeviceAddr(magcom->address())
			);
		}
		firstcon.clear();
		// reset
		firstcon.addOutputVector() << (std::int8_t)MagConfig2 <<
			// Set the REBOOT and SOFT_RST flags. Love how they are slight
			// varitions on the names used in the accelerometer; who needs
			// consistency? The docs are consistent in not documenting the
			// flags well. Reboot memory content? Does that mean kill power
			// to memory, then reapply power? Just try them both.
			(std::int8_t)0xC0;
		try {
			magcom->converse(firstcon);
			std::cout << "LSM9DS1Mag mag reset; got ack ?!?" << std::endl;
		} catch (duds::hardware::interface::I2cErrorNoDevice &) {
			// bother; ignore it
			//std::cout << "LSM9DS1Mag mag reset; no ack" << std::endl;
		}
		// give the device(s) time to complete the reset
		// big guess here
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	} catch (...) {
		// move the I2C communicators back
		i2c = std::move(magcom);
		throw;
	}
	// devices should be suspended by the reset operations
}

LSM9DS1Mag::~LSM9DS1Mag() {
	suspend();
}

void LSM9DS1Mag::configure(float freq,	Settings settings) {
	// one of the instruments must be enabled
	if (!settings.magnetometer) {
		DUDS_THROW_EXCEPTION(LSM9DS1NoInsturment());
	}
	// magnetometer data rate
	if (
		// rate must be positive
		(freq < 0) ||
		// rate must have a suitable match with device capabilities
		!matchDataRate(mdrval, mdatarate, freq, MagDataRateVals) ||
		// rate must be the slowest if low-power mode is used
		(settings.magnetometer && settings.magLowPower && mdrval)
	) {
		DUDS_THROW_EXCEPTION(
			LSM9DS1BadDataRate() << RequestedUpdateRate(freq)
		);
	}
	if (settings.magnetometer && settings.magLowPower) {
		freq = 0;
	}
	// prevent device from doing much prior to changing config
	suspend();  // ensures it is inactive

	duds::hardware::interface::Conversation conv;
	// configure magnetometer; in power-down state after suspend()
	if (settings.magnetometer) {
		std::uint8_t tmpregs[4] = {
			(std::uint8_t)(
				(settings.magTempComp << 7) |
				(settings.xyMagMode << 5)  |
				(mdrval << 2)
			),
			(std::uint8_t)(settings.magRange << 5),
			(std::uint8_t)(settings.magLowPower << 6),
			(std::uint8_t)(settings.zMagMode << 2)
		};
		conv.addOutputVector() << (std::uint8_t)MagConfig1 << tmpregs[0] <<
			tmpregs[1] << tmpregs[2] << tmpregs[3];
		magcom->converse(conv);
	}
	cfg = settings;

	// make the conversation to read in the device's status; works for both
	// devices
	statq.clear();
	statq.addOutputVector() << (std::uint8_t)MagStatus;
	statq.addInputVector(1);
	// conversation to read magnetometer samples
	magsample.clear();
	if (settings.magnetometer) {
		magsample.addOutputVector() << (std::uint8_t)MagSampleX;
		magsample.addInputVector(6);
	}
}

void LSM9DS1Mag::suspend() {
	duds::hardware::interface::Conversation conv;
	// for mag, set config reg 3 to 3; operating mode power-down
	conv.addOutputVector() << (std::uint8_t)MagConfig3 << (std::uint8_t)3;
	magcom->converse(conv);
}

bool LSM9DS1Mag::sample() {
	if (cfg.magnetometer) {
		magcom->converse(statq);
		duds::hardware::interface::ConversationExtractor ex(statq);
		std::int8_t status;
		ex >> status;
		if (status & 7) {
			magcom->converse(magsample);
			ex.reset(magsample);
			ex >> magn.x >> magn.y >> magn.z;
			// The documentation has a confusing diagram of the axes showing that
			// the magnetometer's axes are different from the others while rotating
			// the chip 90 degrees. Modify the axes to match the axes of the others.
			magn.x *= -1;
			return true;
		}
	}
	return false;
}

void LSM9DS1Mag::quantity(ConvertedQuantity &ps) const {
	ps.x() = magn.x * MagScaleToUnits[cfg.magRange];
	ps.y() = magn.y * MagScaleToUnits[cfg.magRange];
	ps.z() = magn.z * MagScaleToUnits[cfg.magRange];
	ps.unit = duds::data::units::Tesla;
}

} } } }
