/**
 * @file
 * Test of bit-per-pixel image support.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/hardware/display/BppImageArchive.hpp>

#include <iostream>

namespace BPPN = duds::hardware::display; // Bit Per Pixel Namespace

BOOST_AUTO_TEST_SUITE(BppImage)

BOOST_AUTO_TEST_CASE(BppImage_Archive) {
	BPPN::BppImageArchive arc;
	BOOST_CHECK_THROW(arc.get("not_there"), BPPN::ImageNotFoundError);
	BOOST_CHECK_THROW(arc.load("not_there"), BPPN::ImageArchiveStreamError);
	BOOST_CHECK_NO_THROW(arc.load(TEST_PATH "BppImageGood.bppia"));
	std::shared_ptr<BPPN::BppImage> img;

	// test image Zebra
	BOOST_REQUIRE_NO_THROW(img = arc.get("Zebra"));
	BOOST_REQUIRE(img);
	BOOST_CHECK(!img->empty());
	BOOST_CHECK_EQUAL(img->width(), 8);
	BOOST_CHECK_EQUAL(img->height(), 8);
	BOOST_CHECK_EQUAL(img->blocksPerLine(), 1);

	// test image Bars
	BOOST_REQUIRE_NO_THROW(img = arc.get("Bars"));
	BOOST_REQUIRE(img);
	BOOST_CHECK(!img->empty());
	BOOST_CHECK_EQUAL(img->width(), 5);
	BOOST_CHECK_EQUAL(img->height(), 5);
	BOOST_CHECK_EQUAL(img->blocksPerLine(), 1);
}

BOOST_AUTO_TEST_SUITE_END()



struct ZebraImageFixture {
	std::shared_ptr<BPPN::BppImage> img;
	ZebraImageFixture() {
		BPPN::BppImageArchive arc;
		arc.load(TEST_PATH "BppImageGood.bppia");
		img = arc.get("Zebra");
	}
};

BOOST_FIXTURE_TEST_SUITE(BppImage_ZebraTests, ZebraImageFixture)

BOOST_AUTO_TEST_CASE(BppImage_ZebraBufferLine) {
	for (int y = 0; y < 8; ++y) {
		std::uint8_t row = *img->bufferLine(y);
		if (y & 1) {
			BOOST_CHECK_EQUAL(row, 0xAA);
		} else {
			BOOST_CHECK_EQUAL(row, 0x55);
		}
	}
}

BOOST_AUTO_TEST_CASE(BppImage_ZebraHorizInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin();
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	BOOST_CHECK_EQUAL(*cp, true);
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++cp, ++x) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x);
			BOOST_CHECK_EQUAL(cp.absY(), y);
			if (y & 1) {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_ZebraHorizDec) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::HorizDec);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int y = 7; y >= 0; --y) {
		for (int x = 7; x >= 0; ++cp, --x) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x);
			BOOST_CHECK_EQUAL(cp.absY(), y);
			if (y & 1) {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_ZebraVertInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::VertInc);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int x = 7; x >= 0; --x) {
		for (int y = 0; y < 8; ++cp, ++y) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x);
			BOOST_CHECK_EQUAL(cp.absY(), y);
			if (y & 1) {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_ZebraVertDec) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::VertDec);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int x = 0; x < 8; ++x) {
		for (int y = 7; y >= 0; ++cp, --y) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x);
			BOOST_CHECK_EQUAL(cp.absY(), y);
			if (y & 1) {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_SubZebraHorizInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(
		BPPN::ImageLocation(2, 1),
		BPPN::ImageDimensions(3, 4)
	);
	BOOST_CHECK_EQUAL(cp.location(), BPPN::ImageLocation(0, 0));
	BOOST_CHECK_EQUAL(cp.absLocation(), BPPN::ImageLocation(2, 1));
	BOOST_CHECK_EQUAL(cp.origin(), BPPN::ImageLocation(2, 1));
	BOOST_CHECK_EQUAL(cp.dimensions(), BPPN::ImageDimensions(3, 4));
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	BOOST_CHECK_EQUAL(*cp, false);
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 3; ++cp, ++x) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x + 2);
			BOOST_CHECK_EQUAL(cp.absY(), y + 1);
			if (y & 1) {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_SubZebraVertInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(
		BPPN::ImageLocation(2, 1),
		BPPN::ImageDimensions(3, 4),
		BPPN::BppImage::VertInc
	);
	BOOST_CHECK_EQUAL(cp.location(), BPPN::ImageLocation(2, 0));
	BOOST_CHECK_EQUAL(cp.absLocation(), BPPN::ImageLocation(4, 1));
	BOOST_CHECK_EQUAL(cp.origin(), BPPN::ImageLocation(2, 1));
	BOOST_CHECK_EQUAL(cp.dimensions(), BPPN::ImageDimensions(3, 4));
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	BOOST_CHECK_EQUAL(*cp, false);
	for (int x = 2; x >= 0; --x) {
		for (int y = 0; y < 4; ++cp, ++y) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(cp.absX(), x + 2);
			BOOST_CHECK_EQUAL(cp.absY(), y + 1);
			if (y & 1) {
				BOOST_CHECK_EQUAL(!(x & 1), cp.state());
				BOOST_CHECK_EQUAL(!(x & 1), *cp);
			} else {
				BOOST_CHECK_EQUAL((x & 1), cp.state());
				BOOST_CHECK_EQUAL((x & 1), *cp);
			}
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_SUITE_END()



struct BarsImageFixure {
	std::shared_ptr<BPPN::BppImage> img;
	BarsImageFixure() {
		BPPN::BppImageArchive arc;
		arc.load(TEST_PATH "BppImageGood.bppia");
		img = arc.get("Bars");
	}
};

BOOST_FIXTURE_TEST_SUITE(BppImage_BarsTests, BarsImageFixure)

BOOST_AUTO_TEST_CASE(BppImage_BarsBufferLine) {
	for (int y = 0; y < 5; ++y) {
		std::uint8_t row = *img->bufferLine(y);
		BOOST_CHECK_EQUAL(row, 21);
	}
}

BOOST_AUTO_TEST_CASE(BppImage_BarsHorizInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin();
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++cp, ++x) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(!(x & 1), cp.state());
			BOOST_CHECK_EQUAL(!(x & 1), *cp);
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_BarsHorizDec) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::HorizDec);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int y = 4; y >= 0; --y) {
		for (int x = 4; x >= 0; ++cp, --x) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(!(x & 1), cp.state());
			BOOST_CHECK_EQUAL(!(x & 1), *cp);
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_BarsVertInc) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::VertInc);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int x = 4; x >= 0; --x) {
		for (int y = 0; y < 5; ++cp, ++y) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(!(x & 1), cp.state());
			BOOST_CHECK_EQUAL(!(x & 1), *cp);
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_CASE(BppImage_BarsVertDec) {
	BPPN::BppImage::ConstPixel cp = img->cbegin(BPPN::BppImage::VertDec);
	BOOST_CHECK(cp != BPPN::BppImage::EndPixel());
	BPPN::BppImage::ConstPixel cend = img->cend();
	for (int x = 0; x < 5; ++x) {
		for (int y = 4; y >= 0; ++cp, --y) {
			BOOST_CHECK(cp != cend);
			BOOST_CHECK_EQUAL(cp.x(), x);
			BOOST_CHECK_EQUAL(cp.y(), y);
			BOOST_CHECK_EQUAL(!(x & 1), cp.state());
			BOOST_CHECK_EQUAL(!(x & 1), *cp);
		}
	}
	BOOST_CHECK(cp == cend);
	BOOST_CHECK(cp == BPPN::BppImage::EndPixel());
}

BOOST_AUTO_TEST_SUITE_END()
