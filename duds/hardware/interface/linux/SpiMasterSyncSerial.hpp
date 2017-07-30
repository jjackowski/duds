/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/MasterSyncSerial.hpp>
#include <duds/hardware/interface/ChipSelect.hpp>
#include <linux/spi/spidev.h>

// /!@?!#?!#?
#undef linux

namespace duds { namespace hardware { namespace interface { namespace linux {

/**
 * A synchronous serial implementation using the SPI userspace interface
 * provided by the Linux kernel. This requires proper kernel support for the
 * hardware. In addition to a number of SPI master controllers, Linux also
 * has support for SPI using GPIOs. The SPI using GPIO support should have
 * less overhead and better performance than DigitalPinMasterSyncSerial.
 * This class supports only 8-bit words, but otherwise supports all SPI
 * modes. Specific master controllers may not support all modes.
 *
 * @todo  Many errors are reported with SyncSerialIoError exceptions; change
 *        this to provide better information on what failed.
 *
 * @warning  The device will be selected by the SPI hardware only while data
 *           is being transfered; selection will not follow conversations
 *           (bewteen calls to MasterSyncSerialAccess::start() and
 *           MasterSyncSerialAccess::stop()) like it will with
 *           DigitalPinMasterSyncSerial.
 *
 * This could be expanded upon by a class that uses a ChipSelect object to
 * allow multiple devices on the same device file so that more devices can
 * be used on the same bus. However, any process using another device file
 * for the same bus can interrupt communications between transfers. This
 * will need to be handled properly by the circuit to deselect the device
 * selected by the ChipSelect object, and by the devices at the other end
 * of the bus.
 *
 * @author  Jeff Jackowski.
 */
class SpiMasterSyncSerial : public MasterSyncSerial {
	/**
	 * Data for telling the kernel what to send and receive. Placed here to
	 * avoid initializing the whole struct before every transfer.
	 */
	spi_ioc_transfer xfer;
	/**
	 * The file descriptor for SPI access.
	 */
	int spifd;
protected:
	virtual void open();
	virtual void close();
	virtual void start();
	virtual void stop();
	/**
	 * Moves data about.
	 * @warning  Only multiples of 8 are currently supported for @a bits.
	 */
	virtual void transfer(
		const std::uint8_t * __restrict__ out,
		std::uint8_t * __restrict__ in,
		int bits
	);
public:
	/**
	 * Creates the object without a SPI device to use.
	 */
	SpiMasterSyncSerial();
	/**
	 * Creates the object and attempts to open the SPI device.
	 * @param path   The path to the SPI device file.
	 * @param flags  The flags specifying the SPI mode to use. Not all modes
	 *               may be supported.
	 * @param freq   The maximum clock frequency in hertz.
	 */
	SpiMasterSyncSerial(
		const std::string &path,
		Flags flags = MssSpiMode0,
		int freq = 100000
	);
	/**
	 * Creates the object with SPI mode flags but no SPI device.
	 * @param flags  The flags specifying the SPI mode to use. Not all modes
	 *               may be supported.
	 */
	SpiMasterSyncSerial(Flags flags);
	~SpiMasterSyncSerial();
	/**
	 * Opens the SPI device file and configures it
	 * @param path   The path to the SPI device file.
	 * @param flags  The flags specifying the SPI mode to use. Not all modes
	 *               may be supported. If all bits are cleared, the already set
	 *               flags will be used.
	 * @param freq   The maximum clock frequency in hertz.
	 * @pre   The object is not in the open (MssOpen) state; no access object
	 *        exists to use this object.
	 * @post  The object is in the ready (MssReady) state.
	 * @throw SyncSerialIoError  A failure ocurred when opening the file, or
	 *                           when setting the SPI communication parameters.
	 *                           The hardware may not support all SPI modes of
	 *                           operation, but the kernel's SPI over GPIO
	 *                           support should.
	 * @throw SyncSerialInUse    The object is in the open (MssOpen) or
	 *                           communicating (MssCommunicating) state.
	 * @todo  Report errors better to know what exactly failed.
	 */
	void open(
		const std::string &path,
		Flags flags = MssSpiMode0,
		int freq = 100000
	);
	/**
	 * Changes the maximum clock frequency.
	 * @pre  No communication must be taking place; the object must not be in
	 *       the MssCommunicating state.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param freq  The requested maximum clock frequence in hertz.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void setClockFrequency(unsigned int freq);
	/**
	 * Changes the minimum clock period.
	 * @pre  No communication must be taking place; the object must not be in
	 *       the MssCommunicating state.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param nanos  The requested minimum clock period in nanoseconds.
	 * @throw SyncSerialInUse  Called while communication is in progress.
	 */
	void setClockPeriod(unsigned int nanos);
};

} } } } // namespaces

