/**
 * @file
 * Test of the duds::general::SignExtend template.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/general/SignExtend.hpp>

BOOST_AUTO_TEST_SUITE(SignExtend)

BOOST_AUTO_TEST_CASE(SignExtend_Signed) {
	int v = 0xF;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), -1);
	v = 0x8;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), -8);
	v = 0x7;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), 7);
}

BOOST_AUTO_TEST_CASE(SignExtend_Unsigned) {
	unsigned int v = 0xF;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), -1);
	v = 0x8;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), -8);
	v = 0x7;
	BOOST_CHECK_EQUAL(duds::general::SignExtend<4>(v), 7);
}

BOOST_AUTO_TEST_SUITE_END()
