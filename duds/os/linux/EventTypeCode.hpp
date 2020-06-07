/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef EVENTTYPECODE_HPP
#define EVENTTYPECODE_HPP

#include <libevdev/libevdev.h>
#include <cstdint>
#include <string>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

namespace duds { namespace os { namespace linux {

/**
 * Combines an event type and an event code, as defined by libevdev, for the
 * purpose of using a combination of both to identify an input receiver.
 * @author  Jeff Jackowski
 */
union EventTypeCode {
	struct {
		/**
		 * An event type, such as EV_KEY, EV_ABS, or EV_REL.
		 */
		std::uint16_t type;
		/**
		 * An event code, such as KEY_A, ABS_X, or REL_Y.
		 */
		std::uint16_t code;
	};
	/**
	 * The combined event type and code.
	 */
	std::uint32_t typecode;
	/**
	 * Makes this type trivially constructable.
	 */
	EventTypeCode() = default;
	/**
	 * Constructs an EventTypeCode pre-filled with an event type and code.
	 * @param t  The event type, such as EV_KEY, EV_ABS, or EV_REL.
	 * @param c  The event code, such as KEY_A, ABS_X, or REL_Y.
	 */
	constexpr EventTypeCode(std::uint16_t t, std::uint16_t c) : type(t), code(c) { }
	/**
	 * Returns a string of the macro name for the event type, such as "EV_KEY",
	 * or an empty string if the type is unknown or the strings are unavailable.
	 * The strings are provided by libevdev_event_type_get_name().
	 * @pre  Non-empty strings are only possible if libevdev has been built
	 *       with event type and code strings. This seems to be the default
	 *       build option.
	 */
	std::string typeName() const noexcept;
	/**
	 * Returns a string of the macro name for the event type, such as "EV_KEY",
	 * or the given string if the type is unknown or the strings are
	 * unavailable. The strings are provided by libevdev_event_type_get_name().
	 * @param unknown  The string to return if the type name is not provided by
	 *                 libevdev.
	 */
	std::string typeName(const std::string &unknown) const noexcept;
	/**
	 * Returns a string of the macro name for the event code, such as "REL_Y",
	 * or an empty string if the code is unknown or the strings are unavailable.
	 * The strings are provided by libevdev_event_code_get_name().
	 * @pre  Non-empty strings are only possible if libevdev has been built
	 *       with event type and code strings. This seems to be the default
	 *       build option.
	 */
	std::string codeName() const noexcept;
	/**
	 * Returns a string of the macro name for the event code, such as "REL_Y",
	 * or the given string if the code is unknown or the strings are unavailable.
	 * The strings are provided by libevdev_event_code_get_name().
	 * @param unknown  The string to return if the code name is not provided by
	 *                 libevdev.
	 */
	std::string codeName(const std::string &unknown) const noexcept;
	/**
	 * Less-than comparison for EventTypeCode to allow it to be used as a key in
	 * a container class.
	 */
	constexpr bool operator < (EventTypeCode etc) const {
		return typecode < etc.typecode;
	}
	/**
	 * Obvious equality operator.
	 */
	constexpr bool operator == (EventTypeCode etc) const {
		return typecode == etc.typecode;
	}
	/**
	 * Obvious inequality operator.
	 */
	constexpr bool operator != (EventTypeCode etc) const {
		return typecode != etc.typecode;
	}
};

} } }

// hash implementation
namespace std {
	template<> struct hash<duds::os::linux::EventTypeCode> {
		std::size_t operator()(const duds::os::linux::EventTypeCode &etc) const noexcept {
			return etc.typecode;
		}
	};
}

#endif        //  #ifndef EVENTTYPECODE_HPP
