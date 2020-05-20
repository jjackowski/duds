/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <libevdev/libevdev.h>
#include <boost/signals2/signal.hpp>
#include <duds/os/linux/Poller.hpp>

namespace duds { namespace os { namespace linux {

/**
 * Combines an event type and an event code for the purpose of using a
 * combination of both to identify an input receiver.
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


/**
 * Handles getting input from a specific input device using libevdev.
 * This class is not thread-safe, but this should not be an issue.
 *
 * If used with Poller, this object @b must be managed by a std::shared_ptr.
 *
 * @author  Jeff Jackowski
 */
class EvdevInput :
	boost::noncopyable,
	public PollResponder,
	public std::enable_shared_from_this<EvdevInput>
{
public:
	/**
	 * The signal type that will handle input events.
	 * @param etc  The event type and event code of the input event to handle.
	 * @param val  The value of the input.
	 */
	typedef boost::signals2::signal<void(EventTypeCode etc, std::int32_t val)>
		InputSignal;
private:
	/**
	 * A type that relates events to signal handlers.
	 */
	typedef std::map<EventTypeCode, InputSignal>  InputMap;
	/**
	 * Relates events to signal handlers.
	 */
	InputMap receivers;
	/**
	 * Handles input for events that are not listed in the @a receivers InputMap.
	 */
	InputSignal defReceiver;
	/**
	 * The object provided by libevdev that is needed to work with the input
	 * device.
	 */
	libevdev *dev = nullptr;
	/**
	 * The file descriptor to the input device file.
	 */
	int fd;
public:
	/**
	 * Constructs an EvdevInput object without opening a device file. Before
	 * input events can be handled, open() must be called.
	 */
	EvdevInput();
	/**
	 * Creates a EvdevInput object that will read input from the given device
	 * file.
	 * @param path  The device file. This is normally some variation of
	 *              "/dev/input/event[0-9]+". Read-only access will be
	 *              requested.
	 * @throw EvdevFileOpenError  The device file could not be opened.
	 * @throw EvdevInitError      The attempt to initialize libevdev failed.
	 *                            An error code will be added to the exception
	 *                            in a boost::errinfo_errno attribute.
	 */
	EvdevInput(const std::string &path);
	/**
	 * Move constructor.
	 */
	EvdevInput(EvdevInput &&e);
	/**
	 * Creates a EvdevInput object managed by a std::shared_ptr.
	 * @param path  The device file. This is normally some variation of
	 *              "/dev/input/event[0-9]+". Read-only access will be
	 *              requested.
	 * @throw EvdevFileOpenError  The device file could not be opened.
	 * @throw EvdevInitError      The attempt to initialize libevdev failed.
	 *                            An error code will be added to the exception
	 *                            in a boost::errinfo_errno attribute.
	 */
	static std::shared_ptr<EvdevInput> make(const std::string &path) {
		return std::make_shared<EvdevInput>(path);
	}
	/**
	 * Destructor.
	 */
	~EvdevInput();
	/**
	 * Move assignment.
	 */
	EvdevInput &operator = (EvdevInput &&old) noexcept;
	/**
	 * Opens the given input device file.
	 * @pre   An input device file has not yet been opened by this object.
	 * @param path  The device file. This is normally some variation of
	 *              "/dev/input/event[0-9]+". Read-only access will be
	 *              requested.
	 * @throw EvdevFileOpenError         The device file could not be opened.
	 * @throw EvdevFileAlreadyOpenError  A device file is already open.
	 * @throw EvdevInitError             The attempt to initialize libevdev
	 *                                   failed. An error code will be added to
	 *                                   the exception in a boost::errinfo_errno
	 *                                   attribute.
	 */
	void open(const std::string &path);
	/**
	 * Reports the name of the device using libevdev_get_name().
	 */
	std::string name() const;
	/**
	 * Attempts to gain exclusive access to the input device.
	 * @return  True if exclusive access was granted.
	 */
	bool grab();
	/**
	 * Returns true if the input device can produce events of the given type.
	 * @param et  The event type to check.
	 */
	bool hasEventType(unsigned int et) const;
	/**
	 * Returns true if the input device can produce events of the given type and
	 * code.
	 * @param etc  The event type and code to check.
	 */
	bool hasEvent(EventTypeCode etc) const;
	/**
	 * Returns true if the input device can produce events of the given type and
	 * code.
	 * @param et  The event type to check.
	 * @param ec  The event code to check.
	 */
	bool hasEventCode(unsigned int et, unsigned int ec) const {
		return hasEvent(EventTypeCode(et, ec));
	}
	/**
	 * Returns the number of slots supported by a multitouch input device. Some
	 * such devices support 0 slots. If a device has no slot support, the value
	 * will be -1.
	 * @note  To check for no slots without checking for the difference between
	 *        0 slots and slots not supported, check for less than 1.
	 */
	int numMultitouchSlots() const;
	/**
	 * True if the input device has at least one multitouch slot.
	 */
	bool hasMultitouchSlots() const {
		return numMultitouchSlots() > 0;
	}
	/**
	 * Returns the current input value for the given event.
	 * @param etc  The event type and code to check.
	 * @throw EvdevUnsupportedEvent  The event is not supplied by the input
	 *                               device.
	 */
	int value(EventTypeCode etc) const;
	/**
	 * Returns the current input value for the given event.
	 * @param et  The event type to check.
	 * @param ec  The event code to check.
	 * @throw EvdevUnsupportedEvent  The event is not supplied by the input
	 *                               device.
	 */
	int value(unsigned int et, unsigned int ec) const {
		return value(EventTypeCode(et, ec));
	}
	/**
	 * Returns true if there are events awaiting processing on this device.
	 * When true, respondToNextEvent() will not block.
	 */
	bool eventsAvailable() const;
	/**
	 * Responds to the next input event on the device. If there is currently no
	 * queued event, this function will block until an event is available. If
	 * there are one or more queued events, all queued events will be handled
	 * without blocking for more events.
	 *
	 * The event processing is in a loop. The next event is read and then
	 * provided to the appropriate InputSignal for handling. The InputSignal is
	 * invoked directly; it runs on this thread, and no other events are read
	 * until the signal has completed. The loop will continue while no error
	 * has occured, and there are queued events. The queued event check will
	 * include events that have been queued during the time this function is
	 * running.
	 */
	void respondToNextEvent();
	/**
	 * Same as calling respondToNextEvent(); used with Poller.
	 */
	void respond(Poller *, int);
	/**
	 * Registers this object with the given Poller so that Poller::wait() will
	 * invoke respondToNextEvent().
	 * @pre  This object is managed by a std::shared_ptr.
	 * @param p  The Poller object.
	 */
	void usePoller(Poller &p);
	/**
	 * Provides information about a specified absolute axis.
	 * @param absEc  The event code for the axis to query. It must be for an
	 *               event of type EV_ABS.
	 * @return  A non-null pointer to the data.
	 * @throw EvdevUnsupportedEvent The requested event lacks the information;
	 *                              either isn't an axis, or the axis is not
	 *                              provided by the input device.
	 */
	const input_absinfo *absInfo(unsigned int absEc) const;
	/**
	 * Make a connection to an input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	boost::signals2::connection inputConnect(
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
	boost::signals2::connection inputConnect(
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
	boost::signals2::connection inputConnectExtended(
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
	boost::signals2::connection inputConnectExtended(
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
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	void inputDisconnect(
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
	 * @param etc  The event type and code that will be forwarded to the
	 *             provided slot function.
	 */
	template<typename Slot>
	void inputDisconnect(EventTypeCode etc, const Slot &slotFunc) {
		receivers[etc].disconnect(slotFunc);
	}
	/**
	 * Make a connection to the default input event signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_67_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_67_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection inputConnect(
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
	boost::signals2::connection inputConnect(
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
	boost::signals2::connection inputConnectExtended(
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
	boost::signals2::connection inputConnectExtended(
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
	void inputDisconnect(
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
	void inputDisconnect(const Slot &slotFunc) {
		defReceiver.disconnect(slotFunc);
	}
};

/**
 * A shared pointer to a EvdevInput object.
 */
typedef std::shared_ptr<EvdevInput>  EvdevInputSptr;

} } }
