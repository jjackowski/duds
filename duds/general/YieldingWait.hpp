/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */

#include <thread>
#include <chrono>

namespace duds { namespace general {

/**
 * Waits for a minumum period of time by calling std::this_thread::yield() in
 * a loop. This can be closer to the requested time for periods under a
 * millisecond than calling std::this_thread::sleep_for() with the same
 * duration when running on Linux, even on a fast computer. Calling yield
 * prevents the wait from monopolizing a processor.
 * @tparam Duration  The type used for the duration. It must be a variation of
 *                   std::chrono::duration.
 * @param  duration  The minimum time to wait.
 * @author Jeff Jackowski
 */
template <class Duration>
void YieldingWait(Duration duration) {
	auto when = std::chrono::high_resolution_clock::now() + duration;
	do {
		std::this_thread::yield();
	} while (std::chrono::high_resolution_clock::now() < when);
}

/**
 * Waits for a minumum period of time in nanoseconds by calling
 * std::this_thread::yield() in a loop. This can be closer to the requested
 * time for periods under a millisecond than calling
 * std::this_thread::sleep_for() with the same duration when running on Linux,
 * even on a fast computer. Calling yield prevents the wait from monopolizing
 * a processor.
 * @param nano  The minimum time to wait in nanoseconds.
 * @author Jeff Jackowski
 */
inline void YieldingWait(int nano) {
	YieldingWait(std::chrono::nanoseconds(nano));
}

} }
