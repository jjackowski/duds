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

void InputHandlers::handleEvent(EventTypeCode etc, std::int32_t value) {
	InputMap::const_iterator iter = receivers.find(etc);
	const InputSignal *is;
	if (iter != receivers.end()) {
		is = &iter->second;
	} else {
		is = &defReceiver;
	}
	(*is)(etc, value);
}

void InputHandlers::clear() {
	receivers.clear();
	defReceiver.disconnect_all_slots();
}

} } }
