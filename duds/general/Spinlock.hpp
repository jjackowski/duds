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
 * A simple spinlock following the lockable and timed lockable concepts so that
 * it can be used with std::lock_guard, std::unique_lock, and std::lock.
 * Spinlock is a simple veneer over std::atomic_flag to keep the code using it
 * a little simpler. As long as the locks are held very briefly, or longer
 * locks are very uncommon, this should have less overhead than std::mutex.
 *
 * The spin lock can optionally call std::this_thread::yield() between attempts
 * to acquire the lock. The functions implementing the C++ concepts for
 * compatablity with the C++ lock objects will yield if the host system reports
 * that it can only run a single thread at a time
 * (std::thread::hardware_concurrency() == 1), and otherwise will not yeild.
 * Functions that always yield and never yield are also providied. In cases
 * where a yield will work better on all hosts, SpinlockYieldingWrapper can
 * be used with C++ lock objects to always yield.
 *
 * A class that uses a spin lock that may have a member function using the lock
 * when its destructor is called on another thread should declare the spin lock
 * as its last non-static member to delay destruction until after a lock can be
 * acquired.
 *
 * @note     Should a spinlock show up in the C++ libraries, this class will
 *           be deprecated.
 *
 * @warning  To promote preformance, there are no run-time checks to ensure
 *           proper usage. One thread could unlock what another thread locked.
 *           To help avoid misuse, std::lock_guard, std::unique_lock, or
 *           something similar should be used instead of directly calling
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
		// state now unlocked
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
	 * A spiny busy wait that ends with ownership of the lock if ownership can
	 * be granted before @a time.
	 * @param time  When to give up attempting to lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Clock, class Duration>
	bool tryLockNeverYeildUntil(const std::chrono::time_point<Clock,Duration> &time) {
		bool res;
		// spiny wait
		while ((res = af.test_and_set(std::memory_order_acquire)) &&
			(time <= std::chrono::time_point<Clock,Duration>::now())
		) { }
		return !res;
	}
	/**
	 * A yielding wait that ends with ownership of the lock if ownership can
	 * be granted before @a time.
	 * @param time  When to give up attempting to lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Clock, class Duration>
	bool tryLockAlwaysYeildUntil(const std::chrono::time_point<Clock,Duration> &time) {
		bool res;
		// not-so-spiny wait
		while ((res = af.test_and_set(std::memory_order_acquire)) &&
			(time <= std::chrono::time_point<Clock,Duration>::clock::now())
		) {
			std::this_thread::yield();
		}
		return !res;
	}
	/**
	 * A spiny busy wait or a yielding wait that ends with ownership of the
	 * lock if ownership can be granted before @a time.
	 * @param time  When to give up attempting to lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Clock, class Duration>
	bool try_lock_until(const std::chrono::time_point<Clock,Duration> &time) {
		if (useYield) {
			tryLockAlwaysYeildUntil(time);
		} else {
			tryLockNeverYeildUntil(time);
		}
	}
	/**
	 * A spiny busy wait that ends with ownership of the lock if ownership can
	 * be granted within @a duration.
	 * @param duration  The maximum time span to wait for the lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Rep, class Period>
	bool tryLockNeverYeildFor(const std::chrono::duration<Rep,Period> &duration) {
		return tryLockNeverYeildUntil(std::chrono::steady_clock::now() + duration);
	}
	/**
	 * A yielding wait that ends with ownership of the lock if ownership can
	 * be granted within @a duration.
	 * @param duration  The maximum time span to wait for the lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Rep, class Period>
	bool tryLockAlwaysYeildFor(const std::chrono::duration<Rep,Period> &duration) {
		return tryLockAlwaysYeildUntil(std::chrono::steady_clock::now() + duration);
	}
	/**
	 * A spiny busy wait or yielding wait that ends with ownership of the lock
	 * if ownership can be granted within @a duration.
	 * @param duration  The maximum time span to wait for the lock.
	 * @return  True if ownership of the lock was granted.
	 */
	template <class Rep, class Period>
	bool try_lock_for(const std::chrono::duration<Rep,Period> &duration) {
		if (useYield) {
			tryLockAlwaysYeildFor(duration);
		} else {
			tryLockNeverYeildFor(duration);
		}
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


/**
 * A simple wrapper around a Spinlock object that implements the timed lockable
 * concept such that attempts to lock an already locked Spinlock will always
 * yield before trying again. This is useful in cases where a spinlock gives
 * better performance a majority of the time, but seldom occuring longer
 * delays are possible. This class can be used when those longer delays are
 * known to occur to mitigate the performance issues of a spinlock. While
 * functions for yielding exist on Spinlock, this class calls them from the
 * lockable concept functions so that other classes like
 * std::condition_variable_any can yield between lock attempts.
 * @code
 * // something to lock
 * duds::general::Spinlock block;
 * // the wrapper
 * duds::general::SpinlockYieldingWrapper syw(block);
 * // lock using the wrapper; std::unique_lock will now always
 * // yield, even on a host with multiple processors
 * duds::general::UniqueYieldingSpinLock lock(syw);
 * @endcode
 * @author  Jeff Jackowski
 */
class SpinlockYieldingWrapper : boost::noncopyable {
	Spinlock &sl;
public:
	SpinlockYieldingWrapper(Spinlock &l) : sl(l) { }
	void lock() {
		sl.lockAlwaysYield();
	}
	bool try_lock() {
		return sl.try_lock();
	}
	template <class Clock, class Duration>
	bool try_lock_until(const std::chrono::time_point<Clock,Duration> &time) {
		return sl.tryLockAlwaysYeildUntil(time);
	}
	template <class Rep, class Period>
	bool try_lock_for(const std::chrono::duration<Rep,Period> &duration) {
		return sl.tryLockAlwaysYeildFor(duration);
	}
	void unlock() {
		sl.unlock();
	}
};

/**
 * A convenience typedef for a std::lock_guard using the Spinlock yielding
 * wrapper.
 */
typedef std::lock_guard<duds::general::SpinlockYieldingWrapper>
	YieldingSpinLockGuard;

/**
 * A convenience typedef for a std::unique_lock using the Spinlock yielding
 * wrapper.
 */
typedef std::unique_lock<duds::general::SpinlockYieldingWrapper>
	UniqueYieldingSpinLock;

} }

#endif        //  #ifndef SPINLOCK_HPP
