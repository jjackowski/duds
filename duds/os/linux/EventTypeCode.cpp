/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/os/linux/EventTypeCode.hpp>
#include <libevdev/libevdev.h>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

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

} } }
