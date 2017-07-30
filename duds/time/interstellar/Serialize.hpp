/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * Serialization support for Interstellar Time data types.
 */
#include <duds/time/interstellar/Interstellar.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>

namespace boost { namespace serialization {

/**
 * Makes serialization functions. I tried with just templates, but it didn't
 * go well, so I used macros to avoid the compiler errors I was getting.
 */
#define TIMESER(IST, INT, GET, SETOP, FLD) \
template <class Archive> \
void save(Archive &ar, const duds::time::interstellar::IST &t, const unsigned int) { \
	INT val(t.GET()); \
	ar & boost::serialization::make_nvp(#FLD, val); \
} \
template <class Archive> \
void load(Archive &ar, duds::time::interstellar::IST &t, const unsigned int) { \
	INT val; \
	ar & boost::serialization::make_nvp(#FLD, val); \
	t = duds::time::interstellar::IST(SET ## SETOP(val)); \
} \
template <class Archive> \
void serialize(Archive &a, duds::time::interstellar::IST &t, const unsigned int v) { \
	split_free(a, t, v); \
}

#define SETDUR(val)  val.value
#define SETPTR(val)  duds::time::interstellar::Femtoseconds(val.value)
#define SETSELF(val) val
#define SETMILLI(val) duds::time::interstellar::Milliseconds(val)
#define SETNANO(val) duds::time::interstellar::Nanoseconds(val)
#define SETSEC(val) duds::time::interstellar::Seconds(val)

TIMESER(Femtoseconds, duds::data::int128_w, count, DUR, duration)
TIMESER(Milliseconds, std::int64_t, count, SELF, duration)
TIMESER(Nanoseconds, std::uint64_t, count, SELF, duration)
TIMESER(Seconds, std::int64_t, count, SELF, duration)

TIMESER(FemtoTime, duds::data::int128_w, time_since_epoch().count, PTR, time)
TIMESER(MilliTime, std::int64_t, time_since_epoch().count, MILLI, time)
TIMESER(NanoTime, std::uint64_t, time_since_epoch().count, NANO, time)
TIMESER(SecondTime, std::int64_t, time_since_epoch().count, SEC, time)

#undef TIMESER
#undef SETDUR
#undef SETPTR
#undef SETSELF
#undef SETMILLI
#undef SETNANO
#undef SETSEC

} }

