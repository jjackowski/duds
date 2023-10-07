/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <cstdint>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

namespace duds { namespace os { namespace linux {

/**
 * Base class for errors from libevdev.
 */
struct EvdevError : virtual std::exception, virtual boost::exception { };

/**
 * The input device file could not be opened.
 */
struct EvdevFileOpenError : EvdevError { };

/**
 * An attempt was made to open a device file when one was already open.
 */
struct EvdevFileAlreadyOpenError : EvdevError { };

/**
 * Initializing use of the input device failed. A relevant error code should
 * be included in a boost::errinfo_errno with the exception.
 */
struct EvdevInitError : EvdevError { };

/**
 * The specified event type or code is not supported by the input device.
 */
struct EvdevUnsupportedEvent : EvdevError { };

/**
 * An attempt was made to add an event type to a device that cannot support
 * the type, or the type is invalid.
 */
struct EvdevTypeAddError : EvdevError { };

/**
 * An attempt was made to add an event code to a device that cannot support
 * the code, or the code is invalid.
 */
struct EvdevCodeAddError : EvdevError { };

/**
 * An error occured while attempting to create an input device.
 */
struct EvdevInputCreateError : EvdevError { };

/**
 * The event type integer involved in an EvdevError.
 */
typedef boost::error_info<struct Info_EvdevEventType, unsigned int>
	EvdevEventType;

/**
 * The event code integer involved in an EvdevError.
 */
typedef boost::error_info<struct Info_EvdevEventCode, unsigned int>
	EvdevEventCode;

/**
 * The event type string involved in an EvdevError.
 */
typedef boost::error_info<struct Info_EvdevEventType, std::string>
	EvdevEventTypeName;

/**
 * The event code string involved in an EvdevError.
 */
typedef boost::error_info<struct Info_EvdevEventCode, std::string>
	EvdevEventCodeName;

/**
 * The event value, usually an input value, involved in an EvdevError.
 */
typedef boost::error_info<struct Info_EvdevEventValue, std::int32_t>
	EvdevEventValue;

} } }
