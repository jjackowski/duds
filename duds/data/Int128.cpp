/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/BuildConfig.h>
#ifdef HAVE_INT128
#include <boost/multiprecision/cpp_int.hpp>
#include <sstream>

namespace duds { namespace data {

std::istream &operator >> (std::istream &is, __int128 &b) {
	boost::multiprecision::int128_t mpi;
	is >> mpi;
	b = static_cast<__int128>(mpi);
	return is;
}
std::ostream &operator << (std::ostream &os, __int128 const &b) {
	boost::multiprecision::int128_t mpi(b);
	return os << mpi;
}

} }

#endif
