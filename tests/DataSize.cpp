/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
/**
 * @file
 * Test of the duds::general::DataSize template.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/general/DataSize.hpp>

// operator << must be in the same namespace as what it outputs
namespace duds { namespace general {
template <std::size_t Bits>
std::ostream &operator << (std::ostream &os, const DataSize<Bits> &ds) {
	return os << '(' << ds.blocks() << ',' << Bits << ')';
}
} }

BOOST_AUTO_TEST_SUITE(DataSize)

BOOST_AUTO_TEST_CASE(DataSize) {
	BOOST_CHECK_EQUAL(std::is_trivial<duds::general::Bits>::value, true);
	BOOST_CHECK_EQUAL(std::is_trivially_copyable<duds::general::Bits>::value, true);
	// operations on bit sizes
	constexpr duds::general::Bits oneb(1), eightb(8);
	BOOST_CHECK_EQUAL(oneb.blocks(), 1);
	BOOST_CHECK_EQUAL(eightb.blocks(), 8);
	BOOST_CHECK(oneb != eightb);
	BOOST_CHECK_EQUAL(eightb.size<duds::general::Bytes>().blocks(), 1);

	// this doesn't, and shouldn't, compile
	//constexpr duds::general::Bytes badbyte(oneb);
	// this dies at run-time
	BOOST_CHECK_THROW(
		const duds::general::Bytes badbyte(oneb),
		duds::general::DataSizeConversionError
	);
	// wish this would fail to compile
	BOOST_CHECK_THROW(
		duds::general::Kilobytes(duds::general::Bytes(256)),
		duds::general::DataSizeConversionError
	);
	BOOST_CHECK_NO_THROW(
		duds::general::Kilobytes(duds::general::Bytes(256).sizeRounded<
			duds::general::Kilobytes
		>())
	);

	// try out Bytes type
	constexpr duds::general::Bytes oneB(eightb);
	BOOST_CHECK_EQUAL(oneB.blocks(), 1);
	BOOST_CHECK_EQUAL(oneB.bytes(), 1);
	BOOST_CHECK_EQUAL(oneB.bits(), 8);
	BOOST_CHECK_EQUAL(eightb, oneB);
	constexpr duds::general::Bits anotherEight(oneB);
	BOOST_CHECK_EQUAL(anotherEight.blocks(), 8);
	constexpr duds::general::Bytes foobyte(
		eightb.size<duds::general::Bytes>()
	);

	// nibbles & some conversions
	constexpr duds::general::Nibbles twoNa(oneB), twoNb(eightb);
	BOOST_CHECK_EQUAL(twoNa.blocks(), 2);
	BOOST_CHECK_EQUAL(twoNb.blocks(), 2);
	BOOST_CHECK_EQUAL(twoNa, eightb);
	BOOST_CHECK_EQUAL(
		twoNa.sizeRounded<duds::general::Kilobytes>(),
		duds::general::Kilobytes(1)
	);

	// operator tests
	constexpr duds::general::Bytes sum0(twoNa + duds::general::Kilobytes(1));
	BOOST_CHECK_EQUAL(sum0, duds::general::Bytes(1025));
	BOOST_CHECK_EQUAL(sum0, oneB + duds::general::Kilobytes(1));
	BOOST_CHECK(sum0 > duds::general::Kilobytes(1));
	constexpr duds::general::Bytes halfK(512);
	BOOST_CHECK_EQUAL(halfK / 2, duds::general::Bytes(256));
	BOOST_CHECK_EQUAL(halfK * 4, duds::general::Kilobytes(2));
	// wish this would fail to compile
	BOOST_CHECK_THROW(
		duds::general::Kilobytes kb(halfK),
		duds::general::DataSizeConversionError
	);
	duds::general::Kilobytes halfK2(halfK * 2);
	BOOST_CHECK_EQUAL(halfK * 2, halfK2);
	duds::general::Kilobytes kw(halfK2);
	BOOST_CHECK(kw > halfK);
	BOOST_CHECK_EQUAL(kw, halfK2);
	BOOST_CHECK_EQUAL(kw, duds::general::Kilobytes(1));

	// some more math
	duds::general::Bytes sum;
	BOOST_CHECK_NO_THROW(sum = halfK + duds::general::Kilobytes(1));
	BOOST_CHECK_EQUAL(sum.bytes(), 1536);
	BOOST_CHECK_EQUAL(sum, duds::general::DataSize<1536*8>(1));
	BOOST_CHECK(sum > duds::general::Kilobytes(1));
	BOOST_CHECK(sum < duds::general::Kilobytes(2));
	BOOST_CHECK_THROW(
		duds::general::Kilobytes(1) + sum,
		duds::general::DataSizeConversionError
	);
	BOOST_CHECK_NO_THROW(sum += halfK);
	BOOST_CHECK_EQUAL(sum.bytes(), 2048);
	BOOST_CHECK_EQUAL(sum, duds::general::Kilobytes(2));
	BOOST_CHECK_NO_THROW(sum + duds::general::Kilobytes(1));
	BOOST_CHECK_NO_THROW(duds::general::Kilobytes(1) + sum);
	BOOST_CHECK_NO_THROW(sum /= 2);
	BOOST_CHECK_EQUAL(sum, duds::general::Kilobytes(1));
	BOOST_CHECK_NO_THROW(sum *= 5);
	BOOST_CHECK_EQUAL(sum, duds::general::Kilobytes(5));
}

BOOST_AUTO_TEST_SUITE_END()
