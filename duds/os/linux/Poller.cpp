/*
 * This file is part of the Screentouch project. It is subject to the GPLv3
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at
 * https://github.com/jjackowski/screentouch/blob/master/LICENSE.
 * No part of the Screentouch project, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <boost/exception/errinfo_errno.hpp>
#include <duds/os/linux/Poller.hpp>
#include <duds/general/Errors.hpp>
#include <assert.h>

namespace duds { namespace os { namespace linux {

Poller::Poller(int reserveSize) {
	epfd = epoll_create(1);
	if (epfd < 0) {
		DUDS_THROW_EXCEPTION(PollerCreateError() <<
			boost::errinfo_errno(errno)
		);
	}
	if (reserveSize) {
		responders.reserve(reserveSize);
		flist.reserve(reserveSize);
	}
}

Poller::Poller(Poller &&p) :
responders(std::move(p.responders)),
flist(std::move(p.flist)),
epfd(p.epfd) {
	p.epfd = -1;
}

Poller::~Poller() {
	std::lock_guard<std::mutex> lock(block);
	close(epfd);
}

Poller &Poller::operator=(Poller &&p) noexcept {
	std::lock_guard<std::mutex> lock(block);
	std::lock_guard<std::mutex> plock(p.block);
	responders = std::move(p.responders);
	flist = std::move(flist);
	epfd = p.epfd;
	p.epfd = -1;
	return *this;
}

void Poller::add(const PollResponderSptr &prs, int fd, int events) {
	if (!prs) {
		DUDS_THROW_EXCEPTION(PollResponderDoesNotExist());
	}
	epoll_event event;
	event.events = events;
	std::lock_guard<std::mutex> lock(block);
	// set event.data.u32 to the index inside responders that will hold the
	// responder record for this file descriptor
	if (flist.empty()) {
		event.data.u32 = responders.size();
	} else {
		event.data.u32 = flist.back();
	}
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event)) {
		DUDS_THROW_EXCEPTION(PollerError() <<
			boost::errinfo_errno(errno) << PollerFileDescriptor(fd)
		);
	}
	// put the responder record in place
	if (flist.empty()) {
		responders.emplace_back(prs, fd);
	} else {
		responders[event.data.u32].responder = prs;
		responders[event.data.u32].fd = fd;
		flist.pop_back();
	}
}

void Poller::remove(int fd) {
	int err = 0;
	std::lock_guard<std::mutex> lock(block);
	// find the file descriptor in the ResponderRecord objects
	ResponderVec::iterator iter = std::find_if(
		responders.begin(),
		responders.end(),
		[fd](const ResponderRecord &rr) {
			return rr.fd == fd;
		}
	);
	if (iter != responders.end()) {
		errno = 0;
		// Attempt the removal and check for any error other than not finding
		// the given file descriptor. If the descriptor is already closed, it
		// may not be possible to remove the descriptor.
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) && ((err = errno) != ENOENT)) {
			DUDS_THROW_EXCEPTION(PollerError() <<
				boost::errinfo_errno(err) << PollerFileDescriptor(fd)
			);
		}
		// remove the descriptor and handler even if it cannot be removed from
		// what epoll will check
		iter->responder.reset();
		iter->fd = -1;
		// make it available for re-use
		flist.push_back(iter - responders.begin());
		assert(responders[flist.back()].fd == -1);
		// report ENOENT below
		if (!err) {
			return;
		}
	}
	// report ENOENT or other lack of fd
	DUDS_THROW_EXCEPTION(PollerLacksFileDescriptor() <<
		boost::errinfo_errno(err) << PollerFileDescriptor(fd)
	);
}

// Holds response data temporarily.
struct ResponseRecord {
	PollResponderSptr prs;
	int fd;
	ResponseRecord(PollResponderSptr &&p, int f) : prs(std::move(p)), fd(f) { }
};

int Poller::wait(std::chrono::milliseconds timeout) {
	epoll_event events[32];
	int count = epoll_wait(epfd, events, 32, timeout.count());
	if (!count) {
		// all done
		return 0;
	} else if (count < 0) {
		DUDS_THROW_EXCEPTION(PollerError() <<
			boost::errinfo_errno(errno)
		);
	}
	// might be better if this was an array of 32 unconstructed objects to avoid
	// dynamic memory allocation
	std::vector<ResponseRecord> resprec;
	resprec.reserve(count);
	{ // accessing responders needs a lock
		std::lock_guard<std::mutex> lock(block);
		for (int loop = 0; loop < count; ++loop) {
			ResponderVec::iterator iter =
				responders.begin() + events[loop].data.u32;
			PollResponderSptr prs = iter->responder.lock();
			// if the responder still exists . . .
			if (prs) {
				// . . . prepare to invoke it
				resprec.emplace_back(std::move(prs), iter->fd);
			}
			// If the responder does not exist, do nothing. Anything would make
			// this function slower, and would need to handle the possibility of
			// occuring multiple times. Removing the file desriptor is a
			// possibility, but may fail if the descriptor is already closed.
		}
	}
	// invoke all queued responders
	std::for_each(
		resprec.begin(),
		resprec.end(),
		[this](const ResponseRecord &rr) {
			// do not allow an exception to prevent other events from being
			// processed
			try {
				rr.prs->respond(this, rr.fd);
			} catch (...) {
				// maybe record the exceptions and later throw an exception
				// containing all the exceptions?
			}
		}
	);
	return resprec.size();
}

} } }
