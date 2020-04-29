/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * Test of the the user interface Path and related items.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <duds/ui/PathStringGenerator.hpp>

namespace UI = duds::ui;

BOOST_AUTO_TEST_SUITE(Path)

BOOST_AUTO_TEST_CASE(PathTest) {
	UI::Path path;
	UI::PathStringGenerator pstrgen;
	BOOST_REQUIRE_NO_THROW(pstrgen.currentHeader("["));
	BOOST_REQUIRE_NO_THROW(pstrgen.currentFooter("]"));
	BOOST_CHECK_EQUAL(path.empty(), true);
	BOOST_CHECK_THROW(path.currentPage(), std::out_of_range);
	BOOST_CHECK(path.begin() == path.end());
	BOOST_CHECK(pstrgen.generate(path).empty());
	UI::PageSptr p0 = std::make_shared<UI::Page>("0");
	path.push(p0);
	BOOST_CHECK_EQUAL(path.empty(), false);
	BOOST_CHECK_NO_THROW(path.currentPage());
	BOOST_CHECK(path.currentPage() == p0);
	BOOST_CHECK(*path.begin() == p0);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[0]");
	path.back();
	BOOST_CHECK(path.currentPage() == p0);
	path.forward();
	BOOST_CHECK(path.currentPage() == p0);
	UI::PageSptr p1 = std::make_shared<UI::Page>("1");
	path.push(p1);
	BOOST_CHECK_EQUAL(path.size(), 2);
	BOOST_CHECK(path.currentPage() == p1);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0[1]");
	path.forward();
	BOOST_CHECK(path.currentPage() == p1);
	path.back();
	BOOST_CHECK(path.currentPage() == p0);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[0]1");
	path.back();
	BOOST_CHECK(path.currentPage() == p0);
	UI::PageSptr p2 = std::make_shared<UI::Page>("2");
	path.push(p2);
	BOOST_CHECK_EQUAL(path.size(), 2);
	BOOST_CHECK(path.currentPage() == p2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0[2]");
	path.back();
	BOOST_CHECK(path.currentPage() == p0);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[0]2");
	path.push(p1);
	BOOST_CHECK_EQUAL(path.size(), 2);
	path.push(p2);
	BOOST_CHECK_EQUAL(path.size(), 3);
	BOOST_CHECK(path.currentPage() == p2);
	BOOST_CHECK_EQUAL(path.currentPage()->title(),"2");
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01[2]");
	path.back();
	BOOST_CHECK(path.currentPage() == p1);
	BOOST_CHECK_EQUAL(path.currentPage()->title(), "1");
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0[1]2");
	path.back();
	BOOST_CHECK(path.currentPage() == p0);
	BOOST_CHECK_EQUAL(path.currentPage()->title(), "0");
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[0]1");
	path.move(2);
	BOOST_CHECK(path.currentPage() == p2);
	path.move(-4);
	BOOST_CHECK(path.currentPage() == p0);
	path.clear();
	BOOST_CHECK_EQUAL(path.empty(), true);
}

BOOST_AUTO_TEST_SUITE_END()

struct PathStringFixture {
	UI::Path path;
	UI::PathStringGenerator pstrgen;
	UI::PageSptr pages[8];
	static const char *pageNames[8];
	PathStringFixture() : pstrgen("/", "..", 16, 8) {
		{ // make pages
			const char **name = pageNames;
			for (auto page : pages) {
				page = std::make_shared<UI::Page>(*name++);
				path.push(page);
			}
		}
		pstrgen.currentHeader("[");
		pstrgen.currentFooter("]");
	}
};

const char *PathStringFixture::pageNames[8] = {
	"0123456789ABCDEF",
	"0123456789A",
	"0123456789 BCDEF",
	"01 345 789A CDEF",
	"012 456 89A",
	"01234567 9ABCDEF",
	"012345 789ABCDEF",
	"01234567"
};

BOOST_FIXTURE_TEST_SUITE(PathStringTests, PathStringFixture)

BOOST_AUTO_TEST_CASE(PathString_AtEnd) {
	BOOST_CHECK_EQUAL(path.size(), 8);
	BOOST_REQUIRE_THROW(
		pstrgen.ellipsis(PathStringFixture::pageNames[0]),
		UI::PathStringGeneratorParameterError
	);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[01234567]");
	pstrgen.maxLength(19);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012345../[01234567]");
	pstrgen.ellipsis("");
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012345/[01234567]");
	pstrgen.maxTitles(2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012345/[01234567]");
	pstrgen.maxTitles(1);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[01234567]");
	pstrgen.maxLength(-1);
	pstrgen.maxTitles(-1);
	pstrgen.ellipsis("..");
	BOOST_CHECK_EQUAL(
		pstrgen.generate(path),
		"012345../012345../012345../01 345../012 45../012345../012345../[01234567]"
	);
	pstrgen.maxTitleLength(-1);
	BOOST_CHECK_EQUAL(
		pstrgen.generate(path),
		"0123456789ABCDEF/0123456789A/0123456789 BCDEF/01 345 789A CDEF/"
		"012 456 89A/01234567 9ABCDEF/012345 789ABCDEF/[01234567]"
	);
	pstrgen.minTitleLength(12);
	BOOST_CHECK_EQUAL(
		pstrgen.generate(path),
		"0123456789ABCDEF/0123456789A/0123456789 BCDEF/01 345 789A CDEF/"
		"012 456 89A/01234567 9ABCDEF/012345 789ABCDEF/[01234567]"
	);
	pstrgen.maxLength(6);
	BOOST_CHECK_THROW(
		pstrgen.currentHeader(PathStringFixture::pageNames[0]),
		UI::PathStringGeneratorParameterError
	);
	BOOST_CHECK_NO_THROW(pstrgen.currentHeader("01"));
	BOOST_CHECK_THROW(
		pstrgen.currentFooter("01"),
		UI::PathStringGeneratorParameterError
	);
	BOOST_CHECK_NO_THROW(pstrgen.currentFooter("0"));
	BOOST_CHECK_THROW(
		pstrgen.maxLength(5),
		UI::PathStringGeneratorParameterError
	);
	BOOST_CHECK_THROW(
		pstrgen.minTitleLength(8),
		UI::PathStringGeneratorParameterError
	);
}

BOOST_AUTO_TEST_CASE(PathString_Back2) {
	path.move(-2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012345..]");
	pstrgen.maxLength(19);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012 45../[012345..]");
	pstrgen.maxLength(24);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012 45../[012345..]");
	pstrgen.maxLength(27);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012 45../[012345..]");
	pstrgen.maxLength(28);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012 45../[012345..]/012345..");
	pstrgen.showForwardPage(false);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../012 45../[012345..]");
	pstrgen.minTitleLength(2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../012../[012345..]");
	pstrgen.maxTitleLength(7);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01../012../[01234..]");
	pstrgen.maxTitleLength(6);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123../01../012../[0123..]");
	pstrgen.maxLength(34);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123../0123../01../012../[0123..]");
	pstrgen.maxTitleLength(7);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01../012../[01234..]");
	pstrgen.maxLength(35);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01../012../[01234..]");
	pstrgen.maxLength(36);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01234../01../012../[01234..]");
	pstrgen.showForwardPage();
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01../012../[01234..]/01234..");
	pstrgen.showForwardPage(false);
	pstrgen.minTitleLength(3);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01 34../012../[01234..]");
	pstrgen.minTitleLength(4);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01234../01 34../012 4../[01234..]");
}

BOOST_AUTO_TEST_CASE(PathString_Back3) {
	path.move(-3);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012 45..]");
	pstrgen.maxLength(19);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../[012 45..]");
	pstrgen.showWholeCurrentPage();
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012 456 89A]");
	pstrgen.showWholeCurrentPage(false);
	pstrgen.maxTitles(2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../[012 45..]");
	pstrgen.maxTitles(-1);
	pstrgen.maxLength(22);
	pstrgen.maxTitleLength(10);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../[012 456..]");
	pstrgen.maxTitleLength(11);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../[012 456 89A]");
	pstrgen.maxLength(21);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 34../[012 456 89A]");
	pstrgen.maxLength(20);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 3../[012 456 89A]");
	pstrgen.maxLength(19);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012 456 89A]");
	pstrgen.minTitleLength(3);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012 456 89A]");
	pstrgen.minTitleLength(2);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01../[012 456 89A]");
	pstrgen.maxLength(40);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "01 345../[012 456 89A]/01234567..");
	pstrgen.maxTitleLength(13);
	BOOST_CHECK_EQUAL(
		pstrgen.generate(path),
		"01 345 789A../[012 456 89A]/01234567.."
	);
	pstrgen.maxLength(60);
	BOOST_CHECK_EQUAL(
		pstrgen.generate(path),
		"0123456789../01 345 789A../[012 456 89A]/01234567.."
	);
}

BOOST_AUTO_TEST_CASE(PathString_Back4) {
	path.move(-4);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[01 345..]");
	pstrgen.minTitleLength(3);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "012../[01 345..]");
	pstrgen.maxLength(32);
	pstrgen.minTitleLength(5);
	pstrgen.maxTitleLength(14);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789../[01 345 789A..]");
	pstrgen.showWholeCurrentPage();
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789../[01 345 789A CDEF]");
	pstrgen.showWholeCurrentPage(false);
	pstrgen.maxTitleLength(15);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789../[01 345 789A..]");
	pstrgen.maxTitleLength(16);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789../[01 345 789A CDEF]");
	pstrgen.maxLength(34);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789../[01 345 789A CDEF]");
	pstrgen.maxLength(35);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "0123456789 BCDEF/[01 345 789A CDEF]");
}

BOOST_AUTO_TEST_CASE(PathString_AtStart) {
	path.move(-10);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012345..]");
	pstrgen.minTitleLength(3);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012345..]/012..");
	pstrgen.maxLength(64);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[012345..]/012345..");
	pstrgen.maxTitleLength(24);
	BOOST_CHECK_EQUAL(pstrgen.generate(path), "[0123456789ABCDEF]/0123456789A");
}

BOOST_AUTO_TEST_SUITE_END()
