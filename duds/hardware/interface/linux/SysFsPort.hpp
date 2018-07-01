/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPortIndependentPins.hpp>
#include <fstream>

// !@?!#?!#?
// It was bad enough to find an MS header had "#define interface struct".
// I was hoping such things wouldn't be here, but I was wrong.
#undef linux

namespace duds { namespace hardware { namespace interface {

class PinConfiguration;

namespace linux {

/**
 * A GPIO implementation using the Linux kernel's userspace interface in syfs.
 * This implementation expects that the pins to use have already been exported
 * and the process has adequate access rights to use the pins.
 *
 * Support is provided for read-only value and direction files. A read-only
 * value forces the pin to be input only. A read-only direction forces the pin
 * to remain in the direction indicated by the file. If both files are
 * read-only and the direction reads back as "out", open() will throw a
 * PinUnsupportedOperation exception to indicate a useless state.
 *
 * It is assumed that the process using this object for a given pin will be the
 * only process on the host using the pin. This should be a fairly safe
 * assumption since it should only be violated by bad behavior. A host can be
 * configured using specific user accounts or groups to limit access to the
 * pins to particular processes to help enforce good behavior. This can also
 * be used to force a particlar I/O direction and make this class report it
 * in the pin's capabilities.
 *
 * @author  Jeff Jackowski
 */
class SysFsPort : public DigitalPortIndependentPins {
	class FsPin {
		/**
		 * The file used to control the I/O direction of the pin. If the file
		 * cannot be opened for reading and writing, it will be opened for
		 * reading only long enough to record the direction.
		 */
		std::fstream direction;
		/**
		 * The file used to query the pin's input state and change the pins
		 * output state.
		 */
		std::fstream value;
		/**
		 * The GPIO's ID number from the filesystem. This may be different
		 * from the local and global IDs used by the port object.
		 */
		unsigned int fsid;
		/**
		 * The requested output value for the pin. This may be changed while the
		 * pin is an input so that when it is changed to an output this software
		 * can quickly change the state to the requested value. This seems to be
		 * the best possible implementation with the filesystem interface.
		 */
		bool reqoutval;
		/**
		 * The current output value for the pin. This is meaningless when the
		 * pin is an input. When it is an output, this value is used to avoid
		 * writting state changes to the already current state to the @a value
		 * file.
		 */
		bool curoutval;
		/**
		 * True when the pin is configured as an output. This is easier, and
		 * possibly a little faster, to check than inspecting the pin
		 * configuration inside DigitalPort::PinEntry::conf, and it doesn't
		 * make this data structure any larger.
		 */
		bool isoutput;
	public:
		/**
		 * Opens the value and direction files for the pin.
		 * @pre   open() was not previously called (it is called by
		 *        SysFsGpio(unsigned int) ), or failed with an exception in all
		 *        previous calls.
		 * @post  The provided capabilities and configuration objects will be
		 *        changed to best match what is known about the pins and
		 *        available file access.
		 * @param conf          The configuration to change based on the state
		 *                      of the pin.
		 * @param cap           The capabilities to change based on file access.
		 * @param pin           The number used by the kernel to identify the pin.
		 * @throw PinIoError               The pin's value or direction file
		 *                                 cannot be opened, or the data read
		 *                                 from the direction file is neither
		 *                                 "in" nor "out".
		 * @throw PinUnsupportedOperation  The pin has read-only value and direction
		 *                                 files with the direction set to output.
		 */
		void open(DigitalPinConfig &conf, DigitalPinCap &cap, unsigned int pin);
		/**
		 * Reads from the value file of the pin and returns the result.
		 */
		bool read();
		/**
		 * Changes the output value of the pin. If the pin is not an output, the
		 * requested value is stored and will be set later by setDirection() when
		 * the pin changes to an output. Ensuring a particular output state
		 * before beginning to output is not supported by the filesystem
		 * interface.
		 *
		 * The output state is only written to the @a value file if it changed.
		 * Some testing suggests that the pin's output state may strobe when the
		 * @a value or @a direction file is written with the same value that
		 * should already be there. This issue may not exist with all hardware,
		 * and the checks should prevent trouble when it does exist as long as
		 * other processes on the host are not also modifying the pin.
		 *
		 * @param state  The output state, true for high.
		 */
		void write(bool state);
		/**
		 * Changes the pin's direction between input and output. The direction
		 * file is only written if the pin's direction changes. Some testing
		 * suggests that the pin's output state may strobe when the
		 * @a direction or @a value file is written with the same value that
		 * should already be there. This issue may not exist with all hardware,
		 * and the checks should prevent trouble when it does exist as long as
		 * other processes on the host are not also modifying the pin.
		 * @param output  True to make the pin an output, false for input.
		 */
		void setDirection(bool output);
	};
	typedef std::vector<FsPin>  FsPinVector;
	/**
	 * Internal pin objects for each pin that will be made available through
	 * this port object.
	 */
	FsPinVector fspins;
public:
	/* *
	 * Make a SysFsPort object without pins.
	 */
	//SysFsPort() { }
	/**
	 * Make a SysFsPort object with the given pins.
	 * @param ids      The pin numbers from the filesystem. The index of each
	 *                 inside @a ids will be the local pin ID used by this port.
	 *                 A value of -1 will create an unavailable pin and may be
	 *                 used multiple times. Other values must only be used once.
	 * @param firstid  The gloabl ID that will be assigned to the first pin
	 *                 (local ID zero) of this port.
	 * @throw PinIoError               A required file could not be opened.
	 * @throw PinUnsupportedOperation  A pin has read-only value and direction
	 *                                 files with the direction set to output.
	 */
	SysFsPort(const std::vector<unsigned int> &ids, unsigned int firstid);
	/**
	 * Make a SysFsPort object according to the given configuration, and attach
	 * to the configuration.
	 * @param pc    The object with the port configuration data.
	 * @param name  The name of the port in the configuration.
	 * @throw PortDoesNotExistError  There is no port called @a name in the
	 *                               given configuration.
	 */
	static std::shared_ptr<SysFsPort> makeConfiguredPort(
		PinConfiguration &pc,
		const std::string &name = "default"
	);
	/* *
	 * Make a SysFsPort object with the given pins.
	 * @param ids  The key is the pin ID from the filesystem. The value is
	 *             the global ID that will be used by this port.
	 */
	//SysFsPort(const std::map<unsigned int, unsigned int> &ids);
	#ifdef HAVE_LIBBOOST_FILESYSTEM
	// find the pins
	//SysFsPort();
	#endif
	virtual ~SysFsPort();

	// virtual functions required by Digitalport
protected:
	virtual void configurePort(
		unsigned int localPinId,
		const DigitalPinConfig &cfg,
		DigitalPinAccessBase::PortData *
	);
	virtual bool inputImpl(unsigned int gid, DigitalPinAccessBase::PortData *);
	virtual void outputImpl(
		unsigned int lid,
		bool state,
		DigitalPinAccessBase::PortData *
	);
public:
	/**
	 * The sysfs interface does not support simultaneous operations; returns
	 * false.
	 */
	virtual bool simultaneousOperations() const;
};

} } } } // namespaces

