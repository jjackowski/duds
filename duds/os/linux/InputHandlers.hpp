/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef INPUTHANDLERS_HPP
#define INPUTHANDLERS_HPP

#include <unordered_map>
#include <boost/signals2/signal.hpp>
#include <duds/os/linux/EventTypeCode.hpp>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

namespace duds { namespace os { namespace linux {

/**
 * The signal type that will handle input events.
 * @param etc    The event type and event code of the input event to handle.
 * @param value  The value of the input.
 */
typedef boost::signals2::signal<void(EventTypeCode etc, std::int32_t value)>
	InputSignal;

/**
 * Maintains a set of InputSignal objects to respond to input events.
 * These are held separately from the input device so that the input handlers
 * can be applied to multiple input devices.
 * @author  Jeff Jackowski
 */
class InputHandlers {
	/**
	 * A type that relates events to signal handlers.
	 */
	typedef std::unordered_map<EventTypeCode, InputSignal>  InputMap;
	/**
	 * Relates events to signal handlers.
	 */
	InputMap receivers;
	/**
	 * Handles input for events that are not listed in the @a receivers InputMap.
	 */
	InputSignal defReceiver;
public:
	/**
	 * Dispatches the provided input event to the appropriate InputSignal.
	 * @param etc     The event type and code of the input event to handle.
	 * @param value   The value of the input.
	 * @throw object  Anything thrown by the functions invoked by the event's
	 *                input signal.
	 */
	void handleEvent(EventTypeCode etc, std::int32_t value);
	/**
	 * Removes all input handlers.
	 */
	void clear();
	/**
	 * Make a connection to an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	boost::signals2::connection connect(
		EventTypeCode etc,
		const InputSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return receivers[etc].connect(slot, at);
	}
	/**
	 * Make a connection to an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	boost::signals2::connection connect(
		EventTypeCode etc,
		const InputSignal::group_type &group,
		const InputSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return receivers[etc].connect(group, slot, at);
	}
	/**
	 * Make a connection to an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	boost::signals2::connection connectExtended(
		EventTypeCode etc,
		const InputSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return receivers[etc].connect_extended(slot, at);
	}
	/**
	 * Make a connection to an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	boost::signals2::connection connectExtended(
		EventTypeCode etc,
		const InputSignal::group_type &group,
		const InputSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return receivers[etc].connect_extended(group, slot, at);
	}
	/**
	 * Disconnect a group from an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code of the items to remove.
	 */
	void disconnect(
		EventTypeCode etc,
		const InputSignal::group_type &group
	) {
		receivers[etc].disconnect(group);
	}
	/**
	 * Disconnect a slot from an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code of the items to remove.
	 */
	template<typename Slot>
	void disconnect(EventTypeCode etc, const Slot &slotFunc) {
		receivers[etc].disconnect(slotFunc);
	}
	/**
	 * Disconnects all slots from an input event signal.
	 * This actually destructs the signal object for the given event.
	 * @param etc  The event type and code of the items to remove.
	 */
	void disconnectAll(EventTypeCode etc) {
		receivers.erase(etc);
	}
	/**
	 * Make a connection to the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection connect(
		const InputSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return defReceiver.connect(slot, at);
	}
	/**
	 * Make a connection to the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection connect(
		const InputSignal::group_type &group,
		const InputSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return defReceiver.connect(group, slot, at);
	}
	/**
	 * Make a connection to the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection connectExtended(
		const InputSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return defReceiver.connect_extended(slot, at);
	}
	/**
	 * Make a connection to the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection connectExtended(
		const InputSignal::group_type &group,
		const InputSignal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return defReceiver.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect a group from the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void disconnect(
		const InputSignal::group_type &group
	) {
		defReceiver.disconnect(group);
	}
	/**
	 * Disconnect a slot from the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename Slot>
	void disconnect(const Slot &slotFunc) {
		defReceiver.disconnect(slotFunc);
	}
	/**
	 * Disconnects all slots from the default input event signal.
	 */
	void disconnectAll() {
		defReceiver.disconnect_all_slots();
	}
};

/**
 * Shared pointer to a InputHandlers class.
 */
typedef std::shared_ptr<InputHandlers>  InputHandlersSptr;

} } }

#endif        //  #ifndef INPUTHANDLERS_HPP
