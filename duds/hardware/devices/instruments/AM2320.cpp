/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/AM2320.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/interface/ConversationExtractor.hpp>
//#include <boost/crc.hpp>
#include <thread>
/* for output to help with CRC bug
#include <iostream>
#include <iomanip>
*/

namespace duds { namespace hardware { namespace devices { namespace instruments {

AM2320::AM2320(std::unique_ptr<duds::hardware::interface::I2c> &c) :
com(std::move(c)) {
	wake.addOutputVector() << (std::int8_t)0;
	read.addOutputVector() << (std::int8_t)3 << (std::int8_t)0 << (std::int8_t)4;
	// cannot use a repeated start
	read.addInputVector(8).bigEndian(true).breakBefore();
	try {
		sample();
	} catch (...) {
		c = std::move(com);
		throw;
	}
}

/* *
 * The AM2320 uses a CRC encoding taken from the Modbus protocol. This produces
 * identical results to the code in the AM2320 datasheet.
 */
//typedef boost::crc_optimal<16, 0x8005, 0xFFFF, 0, true, true>  ModbusCrc;

void AM2320::sample() {
	try {
		// normal to fail with I2C nack
		com->converse(wake);
	} catch (duds::hardware::interface::I2cError &) {
		// bother; ignore it
	}
	// multiple failures followed by a success is a possibility
	for (int attempts = 2; attempts >= 0; --attempts) {
		// short wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		// get sample
		try {
			com->converse(read);
		} catch (duds::hardware::interface::I2cError &) {
			if (!attempts) {
				throw;
			}
		}
	}
	// parse input
	duds::hardware::interface::ConversationExtractor ex(read);
	std::uint16_t data[4];
	ex >> data;
	/* output to help with CRC bug
	std::cout << "AM2320: " << std::hex << data[0] << ' ' << data[1] << ' ' <<
	data[2] << ' ' << data[3] << std::dec << std::endl;
	*/
	// check header-like start
	if (data[0] != 0x304) {
		DUDS_THROW_EXCEPTION(DeviceMisidentified());
	}
	// check CRC
	//ModbusCrc crc;
	//crc.process_bytes(data, 6);
	// CRC sent in little endian; everything else is big endian (why?!?!???!)
	// I gather the CRC is little endian based on the poorly written datasheet.
	// I could be wrong because the CRC never matches, regardless of edianness.
	// The datasheet code, Adafruit code, and the Boost template used here, all
	// produce the same CRC result for the same input. The Adafruit code uses
	// the same data for input, but only requests two register bytes instead
	// of four. It seems to match what the datasheet says. I have no idea why
	// the code here has bad CRCs.
	// In spite of these troubles, the humidity and temperature data appear
	// reasonable and change as expected.
	/*
	if (crc.checksum() != (((data[3] << 8) & 0xFF00) | ((data[3] >> 8) & 0xFF))) {
		//DUDS_THROW_EXCEPTION(AM2320CrcError());
		std::cerr << "AM2320: Bad CRC, correct = " << std::hex << crc.checksum()
		<< std::dec << std::endl;
		// Many attempts at finding a solution. 0xFFFF was also used as the thrid
		// parameter. Nothing matched what the device sent.
		boost::crc_optimal<16, 0x8005, 0, 0, true, false> crc0;
		boost::crc_optimal<16, 0x8005, 0, 0, false, true> crc1;
		boost::crc_optimal<16, 0x8005, 0, 0, false, false> crc2;
		boost::crc_optimal<16, 0xA001, 0, 0, true, true> crc3;
		boost::crc_optimal<16, 0xA001, 0, 0, true, false> crc4;
		boost::crc_optimal<16, 0xA001, 0, 0, false, true> crc5;
		boost::crc_optimal<16, 0xA001, 0, 0, false, false> crc6;
		crc0.process_bytes(data, 6);
		crc1.process_bytes(data, 6);
		crc2.process_bytes(data, 6);
		crc3.process_bytes(data, 6);
		crc4.process_bytes(data, 6);
		crc5.process_bytes(data, 6);
		crc6.process_bytes(data, 6);
		std::cerr << "CRC: 0 " << std::hex << crc0.checksum() << ", 1 " <<
		crc1.checksum() << ", 2 " << crc2.checksum() << ", 3 " <<
		crc3.checksum() << ", 4 " << crc4.checksum() << ", 5 " <<
		crc5.checksum() << ", 6 " << crc6.checksum() << std::endl;
	}
	*/
	// store data
	rh = data[1];
	t = (std::int16_t)data[2];
}

duds::data::Quantity AM2320::relHumidity() const {
	return duds::data::Quantity((double)rh / 10.0, duds::data::Unit(0));
}

duds::data::Quantity AM2320::temperature() const {
	return duds::data::Quantity(
		(double)t / 10.0 + 273.15,
		duds::data::units::Kelvin
	);
}

} } } }
