/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <duds/data/Quantity.hpp>

namespace duds { namespace data {

/**
 * A collection of useful constant Quantity objects.
 *
 * The units for the constants are produced by the DUDS_UNIT_VALUE macro
 * because the math operators on Unit cannot be constexpr functions.
 */
namespace constants {

using duds::data::Quantity;
using duds::data::Unit;
using namespace duds::data::units;

/**
 * A definition for Earth-normal acceleration from gravity that comes from
 * the 1901 General Conference on Weights and Measures. The actual acceleration
 * varies by loaction and over time.
 */
constexpr Quantity EarthSurfaceGravity(
	9.80665, // Meter / (Second * Second)
	//               A  cd   K  kg   m  mol  s  rad  sr
	DUDS_UNIT_VALUE( 0,  0,  0,  0,  1,  0, -2,  0,  0)
);

} } }

#endif        //  #ifndef CONSTANTS_HPP
