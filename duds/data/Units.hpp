/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef UNITS_HPP
#define UNITS_HPP

/**
 * @file
 * Defines Unit objects for specific common base and derived SI units.
 */
#include <duds/data/Unit.hpp>

namespace duds { namespace data {

/**
 * A collection of Unit objects representing commonly used SI units.
 * @todo    Add ability to get a string, both long formal names in multiple
 *          languages and short symbols, that represents what the Unit contains.
 * @author  Jeff Jackowski
 */
namespace units {

// base units
constexpr Unit Ampere   (DUDS_UNIT_VALUE(1, 0, 0, 0, 0, 0, 0, 0, 0));
constexpr Unit Candela  (DUDS_UNIT_VALUE(0, 1, 0, 0, 0, 0, 0, 0, 0));
constexpr Unit Kilogram (DUDS_UNIT_VALUE(0, 0, 1, 0, 0, 0, 0, 0, 0));
constexpr Unit Kelvin   (DUDS_UNIT_VALUE(0, 0, 0, 1, 0, 0, 0, 0, 0));
constexpr Unit Meter    (DUDS_UNIT_VALUE(0, 0, 0, 0, 1, 0, 0, 0, 0));
constexpr Unit Mole     (DUDS_UNIT_VALUE(0, 0, 0, 0, 0, 1, 0, 0, 0));
constexpr Unit Second   (DUDS_UNIT_VALUE(0, 0, 0, 0, 0, 0, 1, 0, 0));
constexpr Unit Radian   (DUDS_UNIT_VALUE(0, 0, 0, 0, 0, 0, 0, 1, 0));
constexpr Unit Steradian(DUDS_UNIT_VALUE(0, 0, 0, 0, 0, 0, 0, 0, 1));

// derived units                          A  cd   K  kg   m  mol  s  rad  sr
constexpr Unit Hertz    (DUDS_UNIT_VALUE( 0,  0,  0,  0,  0,  0, -1,  0,  0));
constexpr Unit Newton   (DUDS_UNIT_VALUE( 0,  0,  0,  1,  1,  0, -2,  0,  0));
constexpr Unit Pascal   (DUDS_UNIT_VALUE( 0,  0,  0,  1, -1,  0, -2,  0,  0));
constexpr Unit Joule    (DUDS_UNIT_VALUE( 0,  0,  0,  1,  2,  0, -2,  0,  0));
constexpr Unit Watt     (DUDS_UNIT_VALUE( 0,  0,  0,  1,  2,  0, -3,  0,  0));
constexpr Unit Coulomb  (DUDS_UNIT_VALUE( 1,  0,  0,  0,  0,  0,  1,  0,  0));
constexpr Unit Volt     (DUDS_UNIT_VALUE(-1,  0,  0,  1,  2,  0, -3,  0,  0));
constexpr Unit Farad    (DUDS_UNIT_VALUE( 2,  0,  0, -1, -2,  0,  4,  0,  0));
constexpr Unit Ohm      (DUDS_UNIT_VALUE(-2,  0,  0,  1,  2,  0, -3,  0,  0));
constexpr Unit Siemens  (DUDS_UNIT_VALUE( 2,  0,  0, -1, -2,  0,  3,  0,  0));
constexpr Unit Weber    (DUDS_UNIT_VALUE(-1,  0,  0,  0,  2,  0, -2,  0,  0));
constexpr Unit Tesla    (DUDS_UNIT_VALUE(-1,  0,  0,  1,  0,  0, -2,  0,  0));
constexpr Unit Henry    (DUDS_UNIT_VALUE(-2,  0,  0,  1,  2,  0, -2,  0,  0));
constexpr Unit Lumen    (DUDS_UNIT_VALUE( 0,  1,  0,  0,  0,  0,  0,  0,  1));
constexpr Unit Lux      (DUDS_UNIT_VALUE( 0,  1,  0,  0, -2,  0,  0,  0,  1));
constexpr Unit Becquerel(DUDS_UNIT_VALUE( 0,  0,  0,  0,  0,  0, -1,  0,  0));
constexpr Unit Gray     (DUDS_UNIT_VALUE( 0,  0,  0,  0,  2,  0, -2,  0,  0));
constexpr Unit Sievert  (DUDS_UNIT_VALUE( 0,  0,  0,  0,  2,  0, -2,  0,  0));
constexpr Unit Katal    (DUDS_UNIT_VALUE( 0,  0,  0,  0,  0,  1, -1,  0,  0));

} } }

#endif        //  #ifndef UNITS_HPP

