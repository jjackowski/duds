/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef SPINLOCKYIELDINGWRAPPER_HPP
#define SPINLOCKYIELDINGWRAPPER_HPP

#include <duds/general/Spinlock.hpp>

namespace duds { namespace general {

/**
 * A simple wrapper around a Spinlock object that implements the lockable
 * concept such that attempts to lock an already locked Spinlock will always
 * yield before trying again. This is useful in cases where a spinlock gives
 * better performance a majority of the time, but seldom occuring longer
 * delays are possible. This class can be used when those longer delays are
 * known to occur to mitigate the performance issues of a spinlock. While
 * functions for yielding exist on Spinlock, this class calls them from the
 * lockable concept functions so that other classes like
 * std::condition_variable_any can yield between lock attempts.
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
	void unlock() {
		sl.unlock();
	}
};

/**
 * A convenience typedef for a unique lock using the Spinlock yielding
 * wrapper.
 */
typedef std::unique_lock<duds::general::SpinlockYieldingWrapper>
	UniqueYieldingSpinLock;

} }

#endif        //  #ifndef SPINLOCKYIELDINGWRAPPER_HPP
