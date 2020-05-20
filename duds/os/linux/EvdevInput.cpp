/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
// for open() and related items; may be more than needed
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <duds/os/linux/EvdevErrors.hpp>
#include <duds/os/linux/EvdevInput.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace os { namespace linux {

std::string EventTypeCode::typeName() const noexcept {
	return typeName(std::string());
}

std::string EventTypeCode::typeName(const std::string &unknown) const noexcept {
	const char *str = libevdev_event_type_get_name(type);
	if (str) {
		return str;
	} else {
		return unknown;
	}
}

std::string EventTypeCode::codeName() const noexcept {
	return codeName(std::string());
}

std::string EventTypeCode::codeName(const std::string &unknown) const noexcept {
	const char *str = libevdev_event_code_get_name(type, code);
	if (str) {
		return str;
	} else {
		return unknown;
	}
}

EvdevInput::EvdevInput() : fd(-1) { }

EvdevInput::EvdevInput(const std::string &path) {
	open(path);
}

void EvdevInput::open(const std::string &path) {
	if (dev) {
		DUDS_THROW_EXCEPTION(EvdevFileAlreadyOpenError());
	}
	fd = ::open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		DUDS_THROW_EXCEPTION(EvdevFileOpenError() <<
			boost::errinfo_file_name(path)
		);
	}
	int result = libevdev_new_from_fd(fd, &dev);
	if (result < 0) {
		close(fd);
		DUDS_THROW_EXCEPTION(EvdevInitError() <<
			boost::errinfo_errno(-result) <<
			// the file may have nothing to do with the error, but it will
			// add context
			boost::errinfo_file_name(path)
		);
	}
}

EvdevInput::EvdevInput(EvdevInput &&e) :
receivers(std::move(e.receivers)),
dev(e.dev),
fd(e.fd) {
	e.dev = nullptr;
	e.fd = -1;
}

EvdevInput::~EvdevInput() {
	if (dev) {
		libevdev_free(dev);
	}
	if (fd > 0) {
		close(fd);
	}
}

EvdevInput &EvdevInput::operator=(EvdevInput &&old) noexcept {
	receivers = std::move(old.receivers);
	dev = old.dev;
	old.dev = nullptr;
	fd = old.fd;
	old.fd = -1;
	return *this;
}

std::string EvdevInput::name() const {
	return libevdev_get_name(dev);
}

bool EvdevInput::grab() {
	return libevdev_grab(dev, LIBEVDEV_GRAB) == 0;
}

bool EvdevInput::hasEventType(unsigned int et) const {
	return libevdev_has_event_type(dev, et) == 1;
}

bool EvdevInput::hasEvent(EventTypeCode etc) const {
	return libevdev_has_event_code(dev, etc.type, etc.code) == 1;
}

int EvdevInput::numMultitouchSlots() const {
	return libevdev_get_num_slots(dev);
}

int EvdevInput::value(EventTypeCode etc) const {
	int val;
	if (!libevdev_fetch_event_value(dev, etc.type, etc.code, &val)) {
		DUDS_THROW_EXCEPTION(EvdevUnsupportedEvent() <<
			EvdevEventType(etc.type) << EvdevEventCode(etc.code)
		);
	}
	return val;
}

bool EvdevInput::eventsAvailable() const {
	return libevdev_has_event_pending(dev) > 0;
}

void EvdevInput::respondToNextEvent() {
	input_event ie;
	int result;
	do {
		result = libevdev_next_event(
			dev,
			LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING,
			&ie
		);
		if (result == LIBEVDEV_READ_STATUS_SUCCESS) {
			EventTypeCode etc(ie.type, ie.code);
			InputMap::const_iterator iter = receivers.find(etc);
			const InputSignal *is;
			if (iter != receivers.end()) {
				is = &iter->second;
			} else {
				is = &defReceiver;
			}
			// don't let input handlers prevent handling all the input
			try {
				(*is)(etc, ie.value);
			} catch (...) { }
		}
	} while ((result >= 0) && (libevdev_has_event_pending(dev) > 0));
}

void EvdevInput::respond(Poller *, int) {
	respondToNextEvent();
}

void EvdevInput::usePoller(Poller &p) {
	p.add(shared_from_this(), fd);
}

const input_absinfo *EvdevInput::absInfo(unsigned int absEc) const {
	const input_absinfo *ia = libevdev_get_abs_info(dev, absEc);
	if (!ia) {
		DUDS_THROW_EXCEPTION(EvdevUnsupportedEvent() <<
			EvdevEventType(EV_ABS) << EvdevEventCode(absEc)
		);
	}
	return ia;
}

} } }
