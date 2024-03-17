/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef POLLER_HPP
#define POLLER_HPP

#include <sys/epoll.h>
#include <boost/exception/info.hpp>
#include <boost/noncopyable.hpp>
#include <mutex>
#include <vector>
#include <memory>

#ifdef linux
// !@?!#?!#?
#undef linux
#endif

namespace duds {

/**
 * Operating system specific support for functionality not covered in other
 * namespaces.
 */
namespace os {

/**
 * Linux specific support code.
 */
namespace linux {

/**
 * The base class for all Poller errors; used for general errors.
 */
struct PollerError : virtual std::exception, virtual boost::exception { };

/**
 * The call to epoll_create() failed. The exception will include the error code
 * in a boost::errinfo_errno attribute.
 */
struct PollerCreateError : PollerError { };

/**
 * An operation (remove) resulted in an error from an epoll function indicating
 * that the file descriptor is not present (ENOENT). This can occur if a file
 * descriptor is closed before being removed from the epoll set.
 */
struct PollerLacksFileDescriptor : PollerError { };

/**
 * Attempted to use a non-existent PollResponder object.
 */
struct PollResponderDoesNotExist : PollerError { };

/**
 * Poller error attribute that includes the value of the file descriptor. This
 * might be useful in coordination with a debugger or other debugging output,
 * but otherwise should be ignored.
 */
typedef boost::error_info<struct Info_PollerFileDescriptor, int>
	PollerFileDescriptor;

class Poller;

/**
 * Responds to a poll event. The associated file descriptor(s) should not be
 * closed until after the response entry is removed from the poller (see
 * Poller::remove()). A class stored in a std::weak_ptr is used instead of
 * std::function because it will ensure the object exists prior to being
 * invoked.
 */
class PollResponder {
public:
	/**
	 * Called by Poller::wait(std::chrono::milliseconds) when an event occurs
	 * on the given file descriptor. The PollResponder object may be associated
	 * with multiple file descriptors across one or more Poller objects.
	 *
	 * This function may add or remove PollResponder objects to or from the
	 * invoking @a poller. If @a poller already has a queued event for a given
	 * file descriptor, removing the responder for that descriptor here will
	 * not prevent the responder from being invoked for the queued event.
	 *
	 * @param poller  The Poller object invoking this function.
	 * @param fd      The file descriptor with an event.
	 */
	virtual void respond(Poller *poller, int fd) = 0;
};

typedef std::shared_ptr<PollResponder>  PollResponderSptr;

/**
 * A simple C++ interface to using Linux's epoll functions.
 * This class is mostly thread-safe. It is intended for handling events on one
 * thread at a time, but events may be added and removed from multiple threads,
 * even while waiting on events. A Poller object must not be destructed if it
 * is waiting on events.
 *
 * File descriptors are not managed by this class. They must be usable if given
 * to add(). Once give to add(), file descriptors must not be closed until
 * after given to remove() or the Poller has been destructed. The Poller does
 * not take responsibility for this, or for closing the descriptors. Failure to
 * remove a file descriptor prior to closing it may result in epoll_ctl() not
 * being able to remove the descriptor, while epoll_wait() may still receive
 * events from the underlying kernel object. The Poller will handle this as
 * gracefully as possible, but calls to wait() may end early without handling
 * events because it received an event from a closed file descriptor.
 *
 * @author  Jeff Jackowski
 */
class Poller : boost::noncopyable {
	/**
	 * Holds a PollResponder object and its associated file descriptor.
	 */
	struct ResponderRecord {
		/**
		 * The PollResponder held with a weak pointer.
		 */
		std::weak_ptr<PollResponder> responder;
		/**
		 * The file descriptor.
		 */
		int fd;
		ResponderRecord() = default;
		ResponderRecord(const PollResponderSptr &prs, int f) :
		responder(prs), fd(f) { }
	};
	/**
	 * Type that holds PollResponder objects and their associated
	 * file descriptors.
	 */
	typedef std::vector<ResponderRecord>  ResponderVec;
	/**
	 * The responders and their file descriptors. A vector is used to allow
	 * O(1) lookups when responding to events.
	 */
	ResponderVec responders;
	/**
	 * Free spot list.
	 */
	std::vector<int> flist;
	/**
	 * Used to allow for thread-safe operation.
	 */
	std::mutex block;
	/**
	 * The file descriptor provided by epoll_create().
	 */
	int epfd;
public:
	/**
	 * The maximum number of events that will be read by a single call to
	 * wait(std::chrono::milliseconds).
	 */
	static constexpr int maxEvents = 32;
	/**
	 * Constructs a new Poller and obtains a file descriptor for use with epoll.
	 * @param reserveSize  The size to reserve in the internal vectors. If the
	 *                     maximum number of responders are known, passing in
	 *                     that value here may limit vector resizing.
	 * @throw PollerCreateError  epoll_create() failed.
	 */
	Poller(int reserveSize = 0);
	/**
	 * A move constructor; @b not thread-safe.
	 */
	Poller(Poller &&p);
	/**
	 * Closes the internal file desciptor used with epoll. Does nothing to the
	 * file descriptors used by the Responder functions.
	 * @pre      This Poller is not waiting on events in another thread.
	 * @warning  The destructor must not be invoked while wait() is running on
	 *           another thread. Doing so will result in wait() attempting to
	 *           use members after the object is destructed.
	 */
	~Poller();
	/**
	 * Move assignment. Must not be called if p.wait() is running on another
	 * thread.
	 */
	Poller &operator=(Poller &&p) noexcept;
	/**
	 * Adds a PollResponder to check for events on a file descriptor. The
	 * function uses a free list to run in O(1) time (excluding epoll_ctl()),
	 * but it will need to allocate memory if the @a responders vector isn't
	 * large enough.
	 * @pre           The file descriptor @a fd is not already added to
	 *                this Poller.
	 * @param prs     A shared pointer to the object that will be informed when
	 *                an event on the file descriptor occurs. The same object
	 *                may be used with multiple file descriptors. Internally, a
	 *                std::weak_ptr to the object will be kept to avoid giving
	 *                the Poller any responsibility for the memory or life-span
	 *                of the PollResponder object.
	 * @param fd      The file descriptor. remove() should be called with this
	 *                descriptor prior to closing the file. Destruction of
	 *                @a prs will not automatically remove the entry made
	 *                by add().
	 * @param events  See the
	 *                [documentation for epoll_ctl() and epoll_event::events](http://man7.org/linux/man-pages/man2/epoll_ctl.2.html).
	 *                The defulat is for data availble for reading without
	 *                blocking.
	 * @throw         PollerError   epoll_ctl() reported an error.
	 */
	void add(const PollResponderSptr &prs, int fd, int events = EPOLLIN);
	/**
	 * Removes the entry for the given file descriptor. This requires a search
	 * of a vector, so it runs in O(n) time (excluding epoll_ctl()).
	 * @pre           @a fd is not yet closed.
	 * @param fd      The file descriptor. If the file is already closed, the
	 *                PollResponder entry will be removed, but epoll_ctl() might
	 *                not remove the file from its interest list. If the kernel
	 *                object previously referenced by the file still exists,
	 *                epoll_wait() will continue to report events on the file.
	 * @throw   PollerError                epoll_ctl() reported an error.
	 * @throw   PollerLacksFileDescriptor  Either this Poller has no entry for
	 *                                     the given file descriptor, or
	 *                                     epoll_ctl failed to remove it from
	 *                                     its interest list.
	 */
	void remove(int fd);
	/**
	 * Waits up to the specified time for events, and processes events
	 * immediately. Up to @a maxEvents events may be recorded in a single call;
	 * this maximum was chosen arbitrarily. If more events are availble, the
	 * additional events will be immediately handled on the next call to wait().
	 *
	 * Before the PollResponder::respond() functions can be invoked, they must
	 * be found within the internal vector @a responders. Data registered with
	 * epoll allows the PollResponder object corresponding to an event to be
	 * found in O(1) time. The weak pointer to the PollResponder object is
	 * converted to a shared pointer. If this does not result in an existing
	 * object, the event will be ignored.
	 *
	 * The PollResponder::respond() functions are called in the order that the
	 * associated events were reported by epoll_wait(). Any thrown exceptions
	 * are caught and ignored. This behavior may change to allow the exceptions
	 * to be reported.
	 *
	 * @warning  This function is @b not thread-safe. While add() and remove()
	 *           may be called from multiple threads while wait() is running,
	 *           only one thread at a time can call wait successfully.
	 *
	 * @param timeout  The maximum amount of time to wait for events to occur.
	 *                 The function will begin processing events as soon as they
	 *                 are available, and returns after processing. A value of
	 *                 zero will handle events that are already queued without
	 *                 waiting for more. A value of -1 will wait indefinitely.
	 *
	 * @return   The number of events handled. If zero, the function either
	 *           waited the maximum amount of time, or a reported event lacked
	 *           a corresponding PollResponder object. Returns -1 if
	 *           epoll_wait() reports EINTR (interrupted system call).
	 * @throw    PollerError   epoll_wait() reported an error other than EINTR.
	 */
	int wait(std::chrono::milliseconds timeout);
	/**
	 * Waits indefinitely for events, only returning after an event is received.
	 * A received event will not be handled if there is no corresponding
	 * PollResponder object, but this function will still return.
	 * Same as calling wait(std::chrono::milliseconds(-1)).
	 * @sa wait(std::chrono::milliseconds).
	 */
	int wait() {  // indefinite
		return wait(std::chrono::milliseconds(-1));
	}
	/**
	 * Responds to events that are already waiting. Same as calling
	 * wait(std::chrono::milliseconds(0)).
	 * @sa wait(std::chrono::milliseconds).
	 */
	int respond() { // no block
		return wait(std::chrono::milliseconds(0));
	}
};

} } }

#endif        //  #ifndef POLLER_HPP
