/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <boost/noncopyable.hpp>

namespace duds { namespace general {

/**
 * A simple spinlock following the lockable concept so that it can be
 * used with std::lock_guard, std::unique_lock, and std::lock.
 * Spinlock is a simple veneer over std::atomic_flag to keep the code using it
 * a little simpler. As long as the locks are held very briefly, or longer
 * locks are very uncommon, this should have less overhead than std::mutex.
 *
 * A class that uses a spin lock should declare it as its last non-static
 * member to delay destruction until after a lock can be aquired.
 *
 * @note     Should a spinlock show up in the C++ libraries, this class will
 *           be deprecated.
 *
 * @warning  To promote preformance, there are no run-time checks to ensure
 *           proper usage. One thread could unlock what another thread locked.
 *           To help avoid misuse and bad error handling, std::lock_guard and
 *           std::unique_lock should be used instead of directly calling
 *           functions on this class.
 *
 * @author  Jeff Jackowski
 */
class Spinlock : boost::noncopyable {
	/**
	 * Used like a mutually exclusive semaphore, but doesn't involve the
	 * operating system.
	 */
	std::atomic_flag af;
	/**
	 * True when lock() should call yield in its loop; good for uniprocessor,
	 * unicore systems.
	 */
	static bool useYield;
public:
	/**
	 * Makes a Spinlock in the unlocked state.
	 */
	Spinlock() {
		// staring from unknown state
		unlock();
	}
	/**
	 * Locks the spinlock before destruction to delay destruction in case of
	 * a lock.
	 */
	~Spinlock() {
		// avoid destruction while in use
		lock();
	}
	/**
	 * A spiny busy wait that ends with ownership of the lock. This is best
	 * used on multi-processor systems; mult-core counts. On single processor
	 * systems, this will eat processor time until the operating system
	 * preempts the thread.
	 */
	void lockNeverYield() {
		// spiny wait
		while (af.test_and_set(std::memory_order_acquire)) { }
	}
	/**
	 * A yielding wait that ends with ownership of the lock. Every time the
	 * lock in not acquired, yield is called to allow other threads a chance
	 * to run, and maybe unlock the resource. This works well on single
	 * processor systems since if the lock is taken another thread must run to
	 * unlock it. On multi-processor systems, this could make the lock take
	 * longer.
	 */
	void lockAlwaysYield() {
		// not-so-spiny wait
		while (af.test_and_set(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}
	/**
	 * A spiny busy or yielding wait that ends with ownership of the lock.
	 * Yield will be used if useYield is true, which is the default for single
	 * processor systems.
	 */
	void lock() {
		if (useYield) {
			lockAlwaysYield();
		} else { 
			lockNeverYield();
		}
	}
	/**
	 * A single attempt at gaining ownership of the lock. This may not be very
	 * useful with a spinlock, but it was easy, so I implemented it.
	 * @return  True if ownership of the lock was granted.
	 */
	bool try_lock() {
		// non-spiny wait
		return !af.test_and_set(std::memory_order_acquire);
	}
	/**
	 * Releases ownership of the lock.
	 */
	void unlock() {
		af.clear(std::memory_order_release);
	}
};

/**
 * A convenience typedef for a std::lock_guard using the Spinlock object.
 */
typedef std::lock_guard<duds::general::Spinlock> SpinLockGuard;
/**
 * A convenience typedef for a std::unique_lock using the Spinlock object.
 */
typedef std::unique_lock<duds::general::Spinlock> UniqueSpinLock;

} }

#endif        //  #ifndef SPINLOCK_HPP
