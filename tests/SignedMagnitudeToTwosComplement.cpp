/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * Test of the duds::general::SignedMagnitudeToTwosComplement template.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/general/SignedMagnitudeToTwosComplement.hpp>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(SignedMagnitudeToTwosComplement)

BOOST_AUTO_TEST_CASE(SignedMagnitude) {
	std::int16_t v = 0xF;
	BOOST_CHECK_EQUAL(duds::general::SignedMagnitudeToTwosComplement<5>(v), 0xF);
	v = 0x1F;
	BOOST_CHECK_EQUAL(duds::general::SignedMagnitudeToTwosComplement<5>(v), -0xF);
	v = 0x10;
	BOOST_CHECK_EQUAL(duds::general::SignedMagnitudeToTwosComplement<5>(v), 0);
	BOOST_CHECK_EQUAL(duds::general::SignedMagnitudeToTwosComplement<16>(v), 0x10);
	// the following two lines intentionally fail to compile
	//duds::general::SignedMagnitudeToTwosComplement<1>(v);
	//duds::general::SignedMagnitudeToTwosComplement<17>(v);
}

BOOST_AUTO_TEST_SUITE_END()
