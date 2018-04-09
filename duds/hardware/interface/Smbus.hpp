/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef SMBUS_HPP
#define SMBUS_HPP

#include <cstdint>
#include <vector>
#include <boost/noncopyable.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * An interface for communication with a
 * [SMBus](http://en.wikipedia.org/wiki/System_Management_Bus) device.
 * This is intended to communicate with a single device. Use one for each
 * device, even if they use the same bus. As a result, implementations need
 * not be thread-safe since it makes little sense to attempt multiple
 * communications with the same device. However, the bus should be handled in
 * a thread-safe manner; multiple simultaneous attempts to use the same bus
 * to communicate with different devices should work.
 *
 * Some I2C devices communicate in a manner that allows them to be handled as
 * SMBus devices. Such devices should use Smbus instead of I2c to take
 * advantage of the easier-to-use interface of Smbus. The Linux kernel
 * documentation implies that SMBus master hardware exists, so favoring the
 * use of Smbus may also increase compatibility without requiring the use of
 * a software master using GPIOs.
 *
 * Whenever possible, use Packet Error Checking (PEC) with communications. It
 * will help prevent bad data on the bus from causing trouble by noticing
 * the bad data and generating an error. It is not part of the I2C
 * specification, so it should be disabled for use with SMBus-like I2C devices.
 *
 * All word, two byte, std::uint16_t data is in host endianness. The SMBus
 * protocol specifies that words are sent in little endian order. This must
 * work on both little and big endian hosts. To assist in dealing with devices
 * that are not SMBus compliant and use big-endian data, the receiveWordBe()
 * and transmitWordBe() functions will do an endianness conversion on the data.
 * They allow the code to use host endianness while communicating with
 * big-endian data.
 *
 * There isn't a matching access object because SMBus is specified in such a
 * way that it shouldn't be required.
 *
 * @todo  Add ability to query SMBus master's capabilities.
 *
 * @author  Jeff Jackowski
 */
class Smbus : boost::noncopyable {
public:
	virtual ~Smbus() = 0;
	// PEC
	/**
	 * Use with constructors to explicitly specify that Packet Error Checking
	 * (PEC) be used. This should normally be the default.
	 */
	struct UsePec { };
	/**
	 * Use with constructors to specify that Packet Error Checking (PEC) will
	 * not be used. This should be avoided unless it is unsupported, as is the
	 * case with I2C devices.
	 */
	struct NoPec { };
	// some flag for big-endian data; standard is little
	/**
	 * Read a single byte from the device without sending a command/register
	 * byte first.
	 * @return  The byte from the device.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual std::uint8_t receiveByte() = 0;
	/**
	 * Sends a command byte, then reads a single byte from the device.
	 * @param cmd  The command or register byte.
	 * @return     The byte from the device.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual std::uint8_t receiveByte(std::uint8_t cmd) = 0;
	/**
	 * Sends a command byte, then reads a word, two bytes, from the device.
	 * @param cmd  The command or register byte.
	 * @return     The word from the device.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual std::uint16_t receiveWord(std::uint8_t cmd) = 0;
	/**
	 * Sends a command byte, then reads a big-endian word from the device.
	 * @param cmd  The command or register byte.
	 * @return     The word from the device in host endianness.
	 * @note  This function is only useful for devices that are not actually
	 *        SMBus compliant. Some I2C devices that can be used as SMBus
	 *        devices may use big-endian data.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	std::uint16_t receiveWordBe(std::uint8_t cmd) {
		std::uint16_t result = receiveWord(cmd);
		return (result << 8) | (result >> 8);
	}
	/**
	 * Sends a command byte, then reads a block of data from the device.
	 * @param cmd     The command or register byte.
	 * @param in      The input buffer.
	 * @param maxlen  The maximum amount of data to write into @a in.
	 * @return        The number of bytes received from the device.
	 * @throw SmbusErrorMessageLength The buffer @a in could not hold all the
	 *                                received data.
	 * @throw SmbusErrorPec           The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy          The bus was in use for an inordinate length
	 *                                of time. This is not caused by scheduling
	 *                                on the same host computer.
	 * @throw SmbusErrorNoDevice      The device did not respond to its address.
	 * @throw SmbusErrorUnsupported   This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol      Data from the device does not conform to
	 *                                the SMBus protocol.
	 * @throw SmbusErrorTimeout       The operation took too long resulting in a
	 *                                bus timeout.
	 * @throw SmbusError              A general error that doesn't fit one of the
	 *                                other exceptions.
	 */
	virtual int receive(std::uint8_t cmd, std::uint8_t *in,
		const int maxlen) = 0;
	/**
	 * Sends a command byte, then reads a block of data from the device.
	 * @param cmd  The command or register byte.
	 * @param in   The input vector. Upon success, the vector will be
	 *             resized to match the received data.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual void receive(std::uint8_t cmd, std::vector<std::uint8_t> &in) = 0;
	/**
	 * Sends a single bit to the device.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual void transmitBool(bool out) = 0;
	/**
	 * Sends a single byte to the device.
	 * @param byte  The byte to send.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual void transmitByte(std::uint8_t byte) = 0;
	/**
	 * Sends a command byte and a data byte to the device.
	 * @param cmd   The command or register byte.
	 * @param byte  The data byte to send.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual void transmitByte(std::uint8_t cmd, std::uint8_t byte) = 0;
	/**
	 * Sends a command byte and a data word to the device.
	 * @param cmd   The command or register byte.
	 * @param word  The data word to send.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual void transmitWord(std::uint8_t cmd, std::uint16_t word) = 0;
	/**
	 * Sends a command byte and a big-endian data word to the device.
	 * @param cmd   The command or register byte.
	 * @param word  The data word to send in host endianness.
	 * @note  This function is only useful for devices that are not actually
	 *        SMBus compliant. Some I2C devices that can be used as SMBus
	 *        devices may use big-endian data.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	void transmitWordBe(std::uint8_t cmd, std::uint16_t word) {
		transmitWord(cmd, (word << 8) | (word >> 8));
	}
	/**
	 * Sends a command byte and a block of data to the device.
	 * @param cmd   The command or register byte.
	 * @param out   The data to send.
	 * @param len   The number of bytes in the buffer @a out to send. It must be
	 *              between 1 and 32, inclusive.
	 * @throw SmbusErrorMessageLength A request was made to send a block of less
	 *                                than 1 or more than 32 bytes.
	 * @throw SmbusErrorPec           The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy          The bus was in use for an inordinate length
	 *                                of time. This is not caused by scheduling
	 *                                on the same host computer.
	 * @throw SmbusErrorNoDevice      The device did not respond to its address.
	 * @throw SmbusErrorUnsupported   This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol      Data from the device does not conform to
	 *                                the SMBus protocol.
	 * @throw SmbusErrorTimeout       The operation took too long resulting in a
	 *                                bus timeout.
	 * @throw SmbusError              A general error that doesn't fit one of the
	 *                                other exceptions.
	 */
	virtual void transmit(
		std::uint8_t cmd,
		const std::uint8_t *out,
		const int len
	) = 0;
	/**
	 * Sends a command byte and a block of data to the device.
	 * @param cmd   The command or register byte.
	 * @param out   The data to send. It must have between 1 and 32 bytes,
	 *              inclusive.
	 * @throw SmbusErrorMessageLength A request was made to send a block of less
	 *                                than 1 or more than 32 bytes.
	 * @throw SmbusErrorPec           The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy          The bus was in use for an inordinate length
	 *                                of time. This is not caused by scheduling
	 *                                on the same host computer.
	 * @throw SmbusErrorNoDevice      The device did not respond to its address.
	 * @throw SmbusErrorUnsupported   This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol      Data from the device does not conform to
	 *                                the SMBus protocol.
	 * @throw SmbusErrorTimeout       The operation took too long resulting in a
	 *                                bus timeout.
	 * @throw SmbusError              A general error that doesn't fit one of the
	 *                                other exceptions.
	 */
	void transmit(std::uint8_t cmd, const std::vector<std::uint8_t> &out) {
		transmit(cmd, &(out[0]), out.size());
	}
	/**
	 * Does a process call operation. Sends a command byte and a word to the
	 * device, then receives a word.
	 * @param cmd   The command or register byte.
	 * @param word  The word to send.
	 * @return      The word received from the device.
	 * @throw SmbusErrorPec          The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy         The bus was in use for an inordinate length
	 *                               of time. This is not caused by scheduling
	 *                               on the same host computer.
	 * @throw SmbusErrorNoDevice     The device did not respond to its address.
	 * @throw SmbusErrorUnsupported  This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol     Data from the device does not conform to
	 *                               the SMBus protocol.
	 * @throw SmbusErrorTimeout      The operation took too long resulting in a
	 *                               bus timeout.
	 * @throw SmbusError             A general error that doesn't fit one of the
	 *                               other exceptions.
	 */
	virtual std::uint16_t call(std::uint8_t cmd, std::uint16_t word) = 0;
	/**
	 * Does a block process call operation. Sends a command byte and block of
	 * data to the device, then receives a block of data.
	 * @param cmd   The command or register byte.
	 * @param out   The data block to send. It must not have more than 32 bytes.
	 * @param in    The data block received from the device.
	 * @throw SmbusErrorMessageLength A request was made to send a block of more
	 *                                than 32 bytes.
	 * @throw SmbusErrorPec           The PEC checksum was not valid for the data.
	 * @throw SmbusErrorBusy          The bus was in use for an inordinate length
	 *                                of time. This is not caused by scheduling
	 *                                on the same host computer.
	 * @throw SmbusErrorNoDevice      The device did not respond to its address.
	 * @throw SmbusErrorUnsupported   This operation is unsupported by the master.
	 * @throw SmbusErrorProtocol      Data from the device does not conform to
	 *                                the SMBus protocol.
	 * @throw SmbusErrorTimeout       The operation took too long resulting in a
	 *                                bus timeout.
	 * @throw SmbusError              A general error that doesn't fit one of the
	 *                                other exceptions.
	 */
	virtual void call(
		std::uint8_t cmd,
		const std::vector<std::uint8_t> &out,
		std::vector<std::uint8_t> &in
	) = 0;
	/**
	 * Returns the address of the device that this object will attempt to
	 * communicate with.
	 */
	virtual int address() const = 0;
};

} } }

#endif        //  #ifndef SMBUS_HPP
