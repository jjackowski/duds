/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

namespace duds { namespace hardware { namespace interface { namespace linux {

/**
 * Initializes a gpiohandle_request structure.
 * @param req       The gpiohandle_request structure to be initialized.
 * @param consumer  The consumer name to place inside the gpiohandle_request.
 */
static void InitGpioHandleReq(
	gpiohandle_request &req,
	const std::string &consumer
) {
	memset(&req, 0, sizeof(gpiohandle_request));
	strcpy(req.consumer_label, consumer.c_str());
}

/**
 * Adds a GPIO line offset to a gpiohandle_request object.
 * @param req     The request object that will hold the offset.
 * @param offset  The offset to add. It will be placed at the end.
 */
static void AddOffset(gpiohandle_request &req, std::uint32_t offset) {
	#ifndef NDEBUG
	// check limit on number of lines
	assert(req.lines < GPIOHANDLES_MAX);
	// ensure the offset is not already present
	for (int idx = req.lines - 1; idx >= 0; --idx) {
		assert(req.lineoffsets[idx] != offset);
	}
	#endif
	req.lineoffsets[req.lines++] = offset;
}

/**
 * Finds the array index that corresponds to the given offset. Useful in cases
 * where the two do not match, such as with IoGpioRequest.
 * @param req     The request object to search.
 * @param offset  The pin offset to find.
 * @return        The index into the lineoffsets array in @a req for
 *                @a offset, or -1 if the offset was not found.
 */
static int FindOffset(gpiohandle_request &req, std::uint32_t offset) {
	for (int idx = req.lines - 1; idx >= 0; --idx) {
		if (req.lineoffsets[idx] == offset) {
			return idx;
		}
	}
	return -1;
}

/**
 * Removes a GPIO line offset from a gpiohandle_request object.
 * @param req     The request object that holds the offset.
 * @param offset  The offset to remove. The offset at the end will take its
 *                place.
 * @return        True if the item was found and removed, false otherwise.
 */
static bool RemoveOffset(gpiohandle_request &req, std::uint32_t offset) {
	// non-empty request?
	if (req.lines) {
		// search for offset
		for (int idx = req.lines - 1; idx >= 0; --idx) {
			if (req.lineoffsets[idx] == offset) {
				// move last offset into spot of offset to remove
				req.lineoffsets[idx] = req.lineoffsets[req.lines - 1];
				// also move output state value
				req.default_values[idx] = req.default_values[req.lines - 1];
				// remove last
				--req.lines;
				return true;
			}
		}
	}
	return false;
}

/**
 * Closes the file descriptor in the request object if it appears to have a
 * file, and then sets the descriptor to zero.
 * @param req     The request object to modify.
 */
static void CloseIfOpen(gpiohandle_request &req) {
	if (req.fd) {
		close(req.fd);
		req.fd = 0;
	}
}

/**
 * Requests input states from the kernel.
 * If the request for using input rather than output has not yet been made,
 * it will be made here. This is because it is valid to have set an input
 * state, lose the access object, then create a new access object for the same
 * pins, and assume the pins are still inputs. The request to use them as
 * inputs, however, must be made again to the kernel.
 * @param chipFd  The file descriptor for the GPIO device. Needed for the
 *                GPIO_GET_LINEHANDLE_IOCTL operation that will occur if any
 *                of the pins has not yet been configured as an input.
 * @param result  The input states.
 * @param req     The request object with the input pins.
 */
static void GetInput(int chipFd, gpiohandle_data &result, gpiohandle_request &req) {
	assert(req.flags & GPIOHANDLE_REQUEST_INPUT);
	assert(req.lines > 0);
	if (!req.fd && (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0)) {
		int res = errno;
		DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
			boost::errinfo_errno(res)
		);
	}
	assert(req.fd);
	if (ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &result) < 0) {
		int res = errno;
		DUDS_THROW_EXCEPTION(GpioDevGetLineValuesError() <<
			boost::errinfo_errno(res)
		);
	}
}

/**
 * Sets the output states for all the pins in the request object.
 * If the request for using output rather than input has not yet been made,
 * it will be made here. This is because it is valid to have set an output
 * state, lose the access object, then create a new access object for the same
 * pins, and assume the pins are still outputs. The request to use them as
 * outputs, however, must be made again to the kernel.
 * @internal
 * @param chipFd  The file descriptor for the GPIO device. Needed for the
 *                GPIO_GET_LINEHANDLE_IOCTL operation that will occur if any
 *                of the pins has not yet been configured as an output.
 * @param req     The request object with the output pins and states.
 *                The default_values field is used for the output states,
 *                even when the pins are already configured as outputs.
 */
static void SetOutput(int chipFd, gpiohandle_request &req) {
	assert(req.flags & GPIOHANDLE_REQUEST_OUTPUT);
	assert(req.lines > 0);
	if (!req.fd) {
		// obtain new line handle only when it didn't already exist
		if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
				boost::errinfo_errno(res)
			);
		}
		// else, the above ioctl function succeeded and the output is now set
	} else {
		// already have line handle
		assert(req.fd);
		if (ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &(req.default_values)) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevSetLineValuesError() <<
				boost::errinfo_errno(res)
			);
		}
	}
}


/**
 * An abstraction for using gpiohandle_request object(s).
 * @author  Jeff Jackowski
 */
class GpioRequest {
public:
	virtual ~GpioRequest() { };
	/**
	 * Configures the pin at the given offset as an input.
	 * @param chipFd  The file descriptor for the GPIO device.
	 * @param offset  The pin offset.
	 */
	virtual void inputOffset(int chipFd, std::uint32_t offset) = 0;
	/**
	 * Configures the pin at the given offset as an output.
	 * @param chipFd  The file descriptor for the GPIO device.
	 * @param offset  The pin offset.
	 * @param state   The output state for the pin.
	 */
	virtual void outputOffset(int chipFd, std::uint32_t offset, bool state) = 0;
	/**
	 * Read from all input pins.
	 * @param chipFd   The file descriptor for the GPIO device.
	 * @param result   The input states.
	 * @param offsets  A pointer to the start of an array with the line
	 *                 offset values for identifying where the input source.
	 * @param length   The number of line offsets.
	 */
	virtual void read(
		int chipFd,
		gpiohandle_data &result,
		std::uint32_t *&offsets,
		int &length
	) = 0;
	/**
	 * Configures pins as outputs and sets their output states.
	 * @param chipFd  The file descriptor for the GPIO device.
	 */
	virtual void write(int chipFd) = 0;
	/**
	 * Sets the output state of a single output pin.
	 * @pre   The pin is already configured as an output.
	 * @param chipFd  The file descriptor for the GPIO device.
	 * @param offset  The pin offset.
	 * @param state   The output state for the pin.
	 */
	virtual void write(int chipFd, std::uint32_t offset, bool state) = 0;
	/**
	 * Reads the input state of the indicated pin. Configures the pin as an
	 * input if not already an input.
	 * @param chipFd  The file descriptor for the GPIO device.
	 * @param offset  The pin offset.
	 */
	virtual bool inputState(int chipFd, std::uint32_t offset) = 0;
	/**
	 * Sets the output state of a single pin in advance of making the output
	 * request to the port. Use write(int) to output the data.
	 * @param offset  The pin offset.
	 * @param state   The output state to store for the pin.
	 */
	virtual void outputState(std::uint32_t offset, bool state) = 0;
};

/**
 * Implements using a single gpiohandle_request object for working with a
 * single pin.
 * @author  Jeff Jackowski
 */
class SingleGpioRequest : public GpioRequest {
	/**
	 * The request object.
	 */
	gpiohandle_request req;
public:
	SingleGpioRequest(const std::string &consumer, std::uint32_t offset) {
		InitGpioHandleReq(req, consumer);
		req.lineoffsets[0] = offset;
		req.lines = 1;
	}
	virtual ~SingleGpioRequest() {
		CloseIfOpen(req);
	}
	virtual void inputOffset(int chipFd, std::uint32_t offset) {
		// offset must not change
		assert(offset == req.lineoffsets[0]);
		req.flags = GPIOHANDLE_REQUEST_INPUT;
		CloseIfOpen(req);
		if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
				boost::errinfo_errno(res)
			);
		}
	}
	virtual void outputOffset(int chipFd, std::uint32_t offset, bool state) {
		// offset must not change
		assert(offset == req.lineoffsets[0]);
		req.flags = GPIOHANDLE_REQUEST_OUTPUT;
		CloseIfOpen(req);
		req.default_values[0] = state;
		if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
				boost::errinfo_errno(res)
			);
		}
	}
	virtual void read(
		int chipFd,
		gpiohandle_data &result,
		std::uint32_t *&offsets,
		int &length
	) {
		GetInput(chipFd, result, req);
		offsets = req.lineoffsets;
		length = 1;
	}
	virtual void write(int chipFd) {
		SetOutput(chipFd, req);
	}
	virtual void write(int chipFd, std::uint32_t offset, bool state) {
		// offset must not change
		assert(offset == req.lineoffsets[0]);
		// early exit: already outputing the requested state
		if (req.fd && (state == (req.default_values[0] > 0))) {
			return;
		}
		// might not yet be an output
		if (req.flags != GPIOHANDLE_REQUEST_OUTPUT) {
			req.flags = GPIOHANDLE_REQUEST_OUTPUT;
			CloseIfOpen(req);
		}
		req.default_values[0] = state;
		SetOutput(chipFd, req);
	}
	virtual bool inputState(int chipFd, std::uint32_t offset) {
		// offset must not change
		assert(offset == req.lineoffsets[0]);
		gpiohandle_data result;
		// might not yet be an input
		if (req.flags != GPIOHANDLE_REQUEST_INPUT) {
			req.flags = GPIOHANDLE_REQUEST_INPUT;
			CloseIfOpen(req);
		}
		GetInput(chipFd, result, req);
		return result.values[0];
	}
	virtual void outputState(std::uint32_t offset, bool state) {
		// offset must not change
		assert(offset == req.lineoffsets[0]);
		req.default_values[0] = state;
	}
};

/**
 * Implements using two gpiohandle_requests object for working with multiple
 * pins.
 * @author  Jeff Jackowski
 */
class IoGpioRequest : public GpioRequest {
	/**
	 * Input request.
	 */
	gpiohandle_request inReq;
	/**
	 * Output request.
	 */
	gpiohandle_request outReq;
public:
	IoGpioRequest(const std::string &consumer) {
		InitGpioHandleReq(inReq, consumer);
		InitGpioHandleReq(outReq, consumer);
		inReq.flags = GPIOHANDLE_REQUEST_INPUT;
		outReq.flags = GPIOHANDLE_REQUEST_OUTPUT;
	}
	virtual ~IoGpioRequest() {
		CloseIfOpen(inReq);
		CloseIfOpen(outReq);
	}
	void lastOutputState(bool state) {
		outReq.default_values[outReq.lines - 1] = state;
	}
	/**
	 * Adds an offset for input use.
	 * @pre  The offset is not in either the input or output set.
	 */
	void addInputOffset(std::uint32_t offset) {
		AddOffset(inReq, offset);
	}
	/**
	 * Adds an offset for output use and sets the initial output state.
	 * @pre  The offset is not in either the input or output set.
	 */
	void addOutputOffset(std::uint32_t offset, bool state) {
		AddOffset(outReq, offset);
		lastOutputState(state);
	}
	virtual void inputOffset(int chipFd, std::uint32_t offset) {
		bool rem = RemoveOffset(outReq, offset);
		assert(rem);
		CloseIfOpen(outReq);
		AddOffset(inReq, offset);
		CloseIfOpen(inReq);
		if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &inReq) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
				boost::errinfo_errno(res)
			);
		}
	}
	virtual void outputOffset(int chipFd, std::uint32_t offset, bool state) {
		bool rem = RemoveOffset(inReq, offset);
		assert(rem);
		CloseIfOpen(inReq);
		AddOffset(outReq, offset);
		CloseIfOpen(outReq);
		lastOutputState(state);
		if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &outReq) < 0) {
			int res = errno;
			DUDS_THROW_EXCEPTION(GpioDevGetLinehandleError() <<
				boost::errinfo_errno(res)
			);
		}
	}
	virtual void read(
		int chipFd,
		gpiohandle_data &result,
		std::uint32_t *&offsets,
		int &length
	) {
		GetInput(chipFd, result, inReq);
		offsets = inReq.lineoffsets;
		length = inReq.lines;
	}
	virtual void write(int chipFd) {
		if (outReq.lines) {
			SetOutput(chipFd, outReq);
		}
	}
	virtual void write(int chipFd, std::uint32_t offset, bool state) {
		int idx = FindOffset(outReq, offset);
		assert(idx >= 0);
		// early exit: already outputing the requested state
		if (outReq.fd && (state == (outReq.default_values[idx] > 0))) {
			return;
		}
		outReq.default_values[idx] = state;
		SetOutput(chipFd, outReq);
	}
	virtual bool inputState(int chipFd, std::uint32_t offset) {
		gpiohandle_data result;
		GetInput(chipFd, result, inReq);
		int idx = FindOffset(inReq, offset);
		assert(idx >= 0);
		return result.values[idx] > 0;
	}
	virtual void outputState(std::uint32_t offset, bool state) {
		int idx = FindOffset(outReq, offset);
		if (idx < 0) {
			bool rem = RemoveOffset(inReq, offset);
			// the pin must already be in a request object
			assert(rem);
			CloseIfOpen(inReq);
			AddOffset(outReq, offset);
			CloseIfOpen(outReq);
			lastOutputState(state);
		} else {
			outReq.default_values[idx] = state;
		}
	}
};

// ---------------------------------------------------------------------------

GpioDevPort::GpioDevPort(
	const std::string &path,
	unsigned int firstid,
	const std::string &username
) : DigitalPortIndependentPins(0, firstid), consumer(username), devpath(path) {
	chipFd = open(path.c_str(), 0);
	if (chipFd < 0) {
		DUDS_THROW_EXCEPTION(DigitalPortDoesNotExistError() <<
			boost::errinfo_file_name(path)
		);
	}
	gpiochip_info cinfo;
	if (ioctl(chipFd, GPIO_GET_CHIPINFO_IOCTL, &cinfo) < 0) {
		int res = errno;
		close(chipFd);
		// could improve error reporting, but may not really matter
		DUDS_THROW_EXCEPTION(DigitalPortDoesNotExistError() <<
			boost::errinfo_file_name(path) << boost::errinfo_errno(res)
		);
	}
	name = cinfo.name;
	pins.resize(cinfo.lines);
	for (std::uint32_t pidx = 0; pidx < cinfo.lines; ++pidx) {
		initPin(pidx, pidx);
	}
}

GpioDevPort::GpioDevPort(
	const std::vector<unsigned int> &ids,
	const std::string &path,
	unsigned int firstid,
	const std::string &username
) : DigitalPortIndependentPins(ids.size(), firstid), consumer(username),
devpath(path) {
	chipFd = open(path.c_str(), 0);
	if (chipFd < 0) {
		DUDS_THROW_EXCEPTION(DigitalPortDoesNotExistError() <<
			boost::errinfo_file_name(path)
		);
	}
	gpiochip_info cinfo;
	if (ioctl(chipFd, GPIO_GET_CHIPINFO_IOCTL, &cinfo) < 0) {
		int res = errno;
		close(chipFd);
		// could improve error reporting, but may not really matter
		DUDS_THROW_EXCEPTION(DigitalPortDoesNotExistError() <<
			boost::errinfo_file_name(path) << boost::errinfo_errno(res)
		);
	}
	name = cinfo.name;
	std::vector<unsigned int>::const_iterator iter = ids.begin();
	for (unsigned int pid = 0; iter != ids.end(); ++iter, ++pid) {
		initPin(*iter, pid);
	}
}

std::shared_ptr<GpioDevPort> GpioDevPort::makeConfiguredPort(
	PinConfiguration &pc,
	const std::string &name,
	const std::string &defaultPath,
	bool forceDefault
) {
	// find the port's config object
	const PinConfiguration::Port &port = pc.port(name);
	// work out device file path
	std::string path;
	if (forceDefault || port.typeval.empty()) {
		path = defaultPath;
	} else {
		path = port.typeval;
	}
	// enumerate the pins
	std::vector<unsigned int> gpios;
	unsigned int next = 0;
	gpios.reserve(port.pins.size());
	for (auto const &pin : port.pidIndex()) {
		// pin IDs cannot be assigned arbitrary values
		if ((pin.pid + port.idOffset) != pin.gid) {
			DUDS_THROW_EXCEPTION(PortBadPinIdError() <<
				PortPinId(pin.gid)
			);
		}
		// need empty spots?
		if (pin.pid > next) {
			// add unavailable pins
			gpios.insert(gpios.end(), pin.pid - next, -1);
		}
		// add available pin
		gpios.push_back(pin.pid);
		next = pin.pid + 1;
	}
	std::shared_ptr<GpioDevPort> sp = std::make_shared<GpioDevPort>(
		gpios,
		path,
		port.idOffset
	);
	try {
		pc.attachPort(sp, name);
	} catch (PinError &pe) {
		pe << boost::errinfo_file_name(path);
		throw;
	}
	return sp;
}

GpioDevPort::~GpioDevPort() {
	shutdown();
	close(chipFd);
}

void GpioDevPort::initPin(std::uint32_t offset, unsigned int pid) {
	if (offset == -1) {
		// line cannot be used
		pins[pid].markNonexistent();
		return;
	}
	// prepare data for inquiry to kernel
	gpioline_info linfo;
	memset(&linfo, 0, sizeof(linfo));  // all examples do this; needed?
	linfo.line_offset = offset;
	// request data from the kernel; check for error
	if (ioctl(chipFd, GPIO_GET_LINEINFO_IOCTL, &linfo) < 0) {
		int res = errno;
		close(chipFd);
		DUDS_THROW_EXCEPTION(DigitalPortLacksPinError() <<
			PinErrorId(globalId(offset)) << PinErrorPortId(offset) <<
			boost::errinfo_errno(res) << boost::errinfo_file_name(devpath)
		);
	}
	// used by kernel?
	if (linfo.flags & GPIOLINE_FLAG_KERNEL) {
		// line cannot be used
		pins[pid].markNonexistent();
	} else {
		// set configuration to match reported status
		if (linfo.flags & GPIOLINE_FLAG_IS_OUT) {
			pins[pid].conf.options = DigitalPinConfig::DirOutput;
		} else {
			pins[pid].conf.options = DigitalPinConfig::DirInput;
		}
		if (linfo.flags & GPIOLINE_FLAG_OPEN_DRAIN) {
			pins[pid].conf.options |= DigitalPinConfig::OutputDriveLow;
		} else if (linfo.flags & GPIOLINE_FLAG_OPEN_SOURCE) {
			pins[pid].conf.options |= DigitalPinConfig::OutputDriveHigh;
		} else if (pins[pid].conf.options & DigitalPinConfig::DirOutput) {
			pins[pid].conf.options |= DigitalPinConfig::OutputPushPull;
		}
		// no data on output currents
		pins[pid].conf.minOutputCurrent = pins[pid].conf.maxOutputCurrent = 0;
		// Unfortunately, the kernel reports on the current status of the line
		// and not the line's capabilities. Report the line can do what the
		// kernel supports, and hope this doesn't cause trouble.
		pins[pid].cap.capabilities =
			DigitalPinCap::Input |
			DigitalPinCap::OutputPushPull  /*|
			DigitalPinCap::EventEdgeFalling |  // theses are not yet supported
			DigitalPinCap::EventEdgeRising |
			DigitalPinCap::EventEdgeChange |
			DigitalPinCap::InterruptOnEvent */;
		// no data on output currents
		pins[pid].cap.maxOutputCurrent = 0;
	}
}

bool GpioDevPort::simultaneousOperations() const {
	return true;
}

void GpioDevPort::madeAccess(DigitalPinAccess &acc) {
	portData(acc).pointer = new SingleGpioRequest(consumer, acc.localId());
}

void GpioDevPort::madeAccess(DigitalPinSetAccess &acc) {
	// create the request objects
	IoGpioRequest *igr = new IoGpioRequest(consumer);
	// fill stuff?

	for (auto pid : acc.localIds()) {
		const PinEntry &pe = pins[pid];
		if (pe.conf.options & DigitalPinConfig::DirInput) {
			//AddOffset(hreq->inReq, pid);
			igr->addInputOffset(pid);
		} else if (pe.conf.options & DigitalPinConfig::DirOutput) {
			//AddOffset(hreq->outReq, pid);
			igr->addOutputOffset(
				pid,
				(pe.conf.options & DigitalPinConfig::OutputState) > 0
			);
		}
		assert(pe.conf.options & DigitalPinConfig::DirMask);
	}
	portData(acc).pointer = igr;
}

void GpioDevPort::retiredAccess(const DigitalPinAccess &acc) noexcept {
	SingleGpioRequest *sgr;
	portDataPtr(acc, &sgr);
	delete sgr;
}

void GpioDevPort::retiredAccess(const DigitalPinSetAccess &acc) noexcept {
	IoGpioRequest *igr;
	portDataPtr(acc, &igr);
	delete igr;
}

void GpioDevPort::configurePort(
	unsigned int lid,
	const DigitalPinConfig &cfg,
	DigitalPinAccessBase::PortData *pdata
) try {
	GpioRequest *gr = (GpioRequest*)pdata->pointer;
	DigitalPinConfig &dpc = pins[lid].conf;
	// change in config?
	if (
		(dpc.options & DigitalPinConfig::DirMask) !=
		(cfg & DigitalPinConfig::DirMask)
	) {
		if (cfg & DigitalPinConfig::DirInput) {
			gr->inputOffset(chipFd, lid);
		} else if (cfg & DigitalPinConfig::DirOutput) {
			gr->outputOffset(
				chipFd,
				lid,
				dpc.options & DigitalPinConfig::OutputState
			);
		}
	}
} catch (PinError &pe) {
	pe << PinErrorId(globalId(lid)) << boost::errinfo_file_name(devpath);
	throw;
}

bool GpioDevPort::inputImpl(
	unsigned int gid,
	DigitalPinAccessBase::PortData *pdata
) try {
	GpioRequest *gr = (GpioRequest*)pdata->pointer;
	int lid = localId(gid);
	bool res = gr->inputState(chipFd, lid);
	pins[lid].conf.options.setTo(DigitalPinConfig::InputState, res);
	return res;
} catch (PinError &pe) {
	pe << PinErrorId(gid) << boost::errinfo_file_name(devpath);
	throw;
}

std::vector<bool> GpioDevPort::inputImpl(
	const std::vector<unsigned int> &pvec,
	DigitalPinAccessBase::PortData *pdata
) try {
	GpioRequest *gr = (GpioRequest*)pdata->pointer;
	gpiohandle_data result;
	std::uint32_t *offsets;
	int length;
	gr->read(chipFd, result, offsets, length);
	assert(length >= pvec.size());
	// record input states
	for (int idx = 0; idx < length; ++idx) {
		pins[offsets[idx]].conf.options.setTo(
			DigitalPinConfig::InputState,
			result.values[idx] > 0
		);
	}
	// return input states
	std::vector<bool> outv(pvec.size());
	int idx = 0;
	for (const unsigned int &gid : pvec) {
		outv[idx++] =
			(pins[localId(gid)].conf.options & DigitalPinConfig::InputState) > 0;
	}
	return outv;
} catch (PinError &pe) {
	pe << boost::errinfo_file_name(devpath);
	throw;
}

void GpioDevPort::outputImpl(
	unsigned int lid,
	bool state,
	DigitalPinAccessBase::PortData *pdata
) try {
	// find the pin configuration
	DigitalPinConfig &dpc = pins[lid].conf;
	// get the request object to make modifications
	GpioRequest *gr = (GpioRequest*)pdata->pointer;
	// is output?
	if (dpc.options & DigitalPinConfig::DirOutput) {
		// set output state
		gr->write(chipFd, lid, state);
	}
	// store new state; no change if error above
	dpc.options.setTo(DigitalPinConfig::OutputState, state);
} catch (PinError &pe) {
	pe << PinErrorId(globalId(lid)) << boost::errinfo_file_name(devpath);
	throw;
}

void GpioDevPort::outputImpl(
	const std::vector<unsigned int> &pvec,
	const std::vector<bool> &state,
	DigitalPinAccessBase::PortData *pdata
) try {
	// get the request object to make modifications
	GpioRequest *gr = (GpioRequest*)pdata->pointer;
	// loop through all pins to alter
	std::vector<unsigned int>::const_iterator piter = pvec.begin();
	std::vector<bool>::const_iterator siter = state.begin();
	int outs = 0;
	for (; piter != pvec.end(); ++piter, ++siter) {
		// configured for output? might be changing state ahead of config change
		if (pins[*piter].conf.options & DigitalPinConfig::DirOutput) {
			// configure the port data; no output happens yet
			gr->outputState(*piter, *siter);
			++outs;
		}
		// store new state
		pins[*piter].conf.options.setTo(DigitalPinConfig::OutputState, *siter);
	}
	// send output if already in output state
	if (outs) {
		gr->write(chipFd);
	}
} catch (PinError &pe) {
	pe << boost::errinfo_file_name(devpath);
	throw;
}

} } } } // namespaces
