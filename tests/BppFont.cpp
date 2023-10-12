/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2023  Jeff Jackowski
 */
/**
 * @file
 * Test of bit-per-pixel font support.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/ui/graphics/BppFontPool.hpp>
#include <set>
#include <iostream>

namespace BPPN = duds::ui::graphics; // Bit Per Pixel Namespace

BOOST_AUTO_TEST_SUITE(BppFont)

BOOST_AUTO_TEST_CASE(BppFont_Pool) {
	BPPN::BppFontPool pool;
	BPPN::BppFontSptr font = pool.getFont("8x16");
	BOOST_CHECK(!font);
	// find the path to the font
	std::string imgpath(boost::unit_test::framework::master_test_suite().argv[0]);
	{
		int found = 0;
		while (!imgpath.empty() && (found < 3)) {
			imgpath.pop_back();
			if (imgpath.back() == '/') {
				++found;
			}
		}
		imgpath += "images/font_8x16.bppia";
	}
	// test adding font to the pool
	BOOST_REQUIRE_NO_THROW(pool.addWithCache("8x16", imgpath));
	font = pool.getFont("8x16");
	BOOST_CHECK(font);
	BPPN::BppStringCacheSptr scache = pool.getStringCache("8x16");
	BOOST_REQUIRE(scache);
	BOOST_CHECK(scache->font().get() == font.get());
	BOOST_CHECK_EQUAL(scache->strings(), 0);
	// bad font test
	BOOST_CHECK_THROW(pool.render("16x8", "Hi"), BPPN::FontNotFoundError);
	// render a string through the font
	BPPN::ConstBppImageSptr img;
	BOOST_REQUIRE_NO_THROW(img = pool.render("8x16", "Hi"));
	BOOST_CHECK_EQUAL(img->width(), 16);
	BOOST_CHECK_EQUAL(img->height(), 16);
	BOOST_CHECK_EQUAL(scache->strings(), 0);
	// render same string, but through string cache
	BPPN::ConstBppImageSptr imgHi;
	BOOST_REQUIRE_NO_THROW(imgHi = pool.text("8x16", "Hi"));
	BOOST_CHECK_EQUAL(imgHi->width(), 16);
	BOOST_CHECK_EQUAL(imgHi->height(), 16);
	BOOST_CHECK_EQUAL(scache->strings(), 1);
	BOOST_CHECK(img.get() != imgHi.get());
	// render same string again through string cache; should be same image
	BOOST_REQUIRE_NO_THROW(img = pool.text("8x16", "Hi"));
	BOOST_CHECK_EQUAL(scache->strings(), 1);
	BOOST_CHECK(img.get() == imgHi.get());
	// yet again, but with different string type
	BOOST_REQUIRE_NO_THROW(img = pool.text("8x16", U"Hi"));
	BOOST_CHECK_EQUAL(scache->strings(), 1);
	BOOST_CHECK(img.get() == imgHi.get());
	// render a single character string; should not add to the cache
	BOOST_REQUIRE_NO_THROW(img = pool.text("8x16", "W"));
	BOOST_CHECK_EQUAL(scache->strings(), 1);
	// the single character string image should be the same as the one given by
	// the font for the character
	BPPN::ConstBppImageSptr imgW;
	BOOST_REQUIRE_NO_THROW(imgW = font->get('W'));
	BOOST_CHECK(img.get() == imgW.get());
	// alias test -- same name for same font in font pool
	BOOST_CHECK_THROW(pool.alias("16x8", "Hi"), BPPN::FontNotFoundError);
	BOOST_REQUIRE_NO_THROW(pool.alias("8x16", "TallFont"));
	BOOST_CHECK(pool.getFont("8x16") == pool.getFont("TallFont"));
}

BOOST_AUTO_TEST_SUITE_END()
