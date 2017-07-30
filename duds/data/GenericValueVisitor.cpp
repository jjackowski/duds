/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/data/GenericValueVisitor.hpp>
#include <duds/time/interstellar/Hectoform.hpp>

namespace duds { namespace data {

std::string GenericValueStringVisitor::operator() (const int128_w &n) const {
	std::ostringstream os;
	os << n;
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const double &n) const {
	std::ostringstream os;
	os << n;
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const boost::uuids::uuid &u) const {
	return boost::uuids::to_string(u);
}

std::string GenericValueStringVisitor::operator() (const Quantity &q) const {
	std::ostringstream os;
	/** @todo  Include the unit name. */
	os << q.value;
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const duds::time::interstellar::Femtoseconds &s) const {
	std::ostringstream os;
	os << s.count() << "fs";
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const duds::time::interstellar::Nanoseconds &s) const {
	std::ostringstream os;
	os << s.count() << "ns";
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const duds::time::interstellar::FemtoTime &t) const {
	duds::time::interstellar::Hectoform hf(t);
	std::ostringstream os;
	os << hf;
	return os.str();
}

std::string GenericValueStringVisitor::operator() (const duds::time::interstellar::NanoTime &t) const {
	duds::time::interstellar::Hectoform hf(t);
	std::ostringstream os;
	os << hf;
	return os.str();
}

} }
