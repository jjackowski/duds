/**
 * @file
 * Test of the duds::general::NddArray container.
 * A complete mess, but still helpful to me.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <duds/general/NddArray.hpp>
#include <sstream>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

namespace dg = duds::general;

struct ArrayTest {
	dg::NddArray<double> array;
	ArrayTest() : array({3,3,3}) {
		// simple test contents
		double *n = &array({0,0,0}), v = 1.0;
		for (std::size_t go = array.numelems(); go; --go, ++n) {
			*n = v;
			v += 1.0;
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(NddArray_General, ArrayTest)

BOOST_AUTO_TEST_CASE(Size) {
	// test initial size set in ArrayTest above
	BOOST_CHECK_EQUAL(array.empty(), false);
	BOOST_CHECK_EQUAL(array.numdims(), 3);
	BOOST_CHECK_EQUAL(array.numelems(), 3 * 3 * 3);
	dg::NddArray<double>::DimVec tval({3,3,3});
	// BOOST_CHECK_EQUAL can't output std::vector<> to std::cout; use check instead
	BOOST_CHECK(array.dim() == tval);
	// test clearing the array
	array.clear();
	BOOST_CHECK_EQUAL(array.empty(), true);
	BOOST_CHECK_EQUAL(array.numdims(), 0);
	BOOST_CHECK_EQUAL(array.numelems(), 0);
	BOOST_CHECK_THROW(array.front(), dg::ZeroSize);
	BOOST_CHECK_THROW(array.back(), dg::ZeroSize);
	// test giving it a new size
	array.remake({4,3,2,4});
	BOOST_CHECK_EQUAL(array.empty(), false);
	BOOST_CHECK_EQUAL(array.numdims(), 4);
	BOOST_CHECK_EQUAL(array.numelems(), 4 * 4 * 3 * 2);
	tval = {4,3,2,4};
	BOOST_CHECK(array.dim() == tval);
	// test sizing to a bad dimension
	BOOST_CHECK_THROW(array.remake({4,0,2,4}), dg::EmptyDimension);
	// should now be clear
	BOOST_CHECK_EQUAL(array.empty(), true);
	BOOST_CHECK_EQUAL(array.numdims(), 0);
	BOOST_CHECK_EQUAL(array.numelems(), 0);
}

BOOST_AUTO_TEST_CASE(Contents) {
	// check contents set in ArrayTest constructor
	BOOST_CHECK_EQUAL(array({0,0,0}), 1.0);
	BOOST_CHECK_EQUAL(array({2,0,0}), 3.0);
	BOOST_CHECK_EQUAL(array({0,1,0}), 4.0);
	BOOST_CHECK_EQUAL(array({0,0,1}), 10.0);
	BOOST_CHECK_EQUAL(array({2,2,2}), (double)array.numelems());
	BOOST_CHECK_EQUAL(array.front(), 1.0);
	BOOST_CHECK_EQUAL(array.back(), (double)array.numelems());
	// check contents again using a vector to specify position
	dg::NddArray<double>::DimVec pos({0,0,0});
	BOOST_CHECK_EQUAL(array(pos), 1.0);
	pos[0] = 2;
	BOOST_CHECK_EQUAL(array(pos), 3.0);
	pos[0] = 0; pos[1] = 1;
	BOOST_CHECK_EQUAL(array(pos), 4.0);
	pos[1] = 0; pos[2] = 1;
	BOOST_CHECK_EQUAL(array(pos), 10.0);
	pos = {2,2,2};
	BOOST_CHECK_EQUAL(array(pos), (double)array.numelems());
	// another array for testing
	ArrayTest at;
	BOOST_CHECK(array == at.array);
	BOOST_CHECK(!(array != at.array));
	// change contents
	array({0,0,1}) = 2.5;
	BOOST_CHECK_EQUAL(array({0,0,1}), 2.5);
	BOOST_CHECK(array != at.array);
	BOOST_CHECK(!(array == at.array));
	// out of range check
	BOOST_CHECK_THROW(array({3,0,0}), dg::OutOfRange);
	BOOST_CHECK_THROW(array({0,3,0}), dg::OutOfRange);
	BOOST_CHECK_THROW(array({0,0,3}), dg::OutOfRange);
	BOOST_CHECK_THROW(array({2,2,3}), dg::OutOfRange);
	BOOST_CHECK_THROW(array({3,2,2}), dg::OutOfRange);
	BOOST_CHECK_THROW(array({0,1}), dg::DimensionMismatch);
	BOOST_CHECK_THROW(array({0,1,2,3}), dg::DimensionMismatch);
	// out of range check (vector)
	pos = {3,0,0};
	BOOST_CHECK_THROW(array(pos), dg::OutOfRange);
	pos = {0,3,0};
	BOOST_CHECK_THROW(array(pos), dg::OutOfRange);
	pos = {0,0,3};
	BOOST_CHECK_THROW(array(pos), dg::OutOfRange);
	pos = {2,2,3};
	BOOST_CHECK_THROW(array(pos), dg::OutOfRange);
	pos = {3,2,2};
	BOOST_CHECK_THROW(array(pos), dg::OutOfRange);
	pos = {0,1};
	BOOST_CHECK_THROW(array(pos), dg::DimensionMismatch);
	pos = {0,1,2,3};
	BOOST_CHECK_THROW(array(pos), dg::DimensionMismatch);
	array.clear();
	BOOST_CHECK_THROW(array({0}), dg::ZeroSize);
	BOOST_CHECK(array != at.array);
	BOOST_CHECK(!(array == at.array));
	array = at.array;
	BOOST_CHECK(array == at.array);
	BOOST_CHECK(!(array != at.array));
}

BOOST_AUTO_TEST_CASE(Resize) {
	dg::NddArray<double> ar(array);
	BOOST_CHECK(array == ar);
	array.resize({4,4,4,4});
	BOOST_CHECK_EQUAL(array.numdims(), 4);
	BOOST_CHECK_EQUAL(array.numelems(), 4 * 4 * 4 * 4);
	dg::NddArray<double>::DimVec tval({4,4,4,4});
	// BOOST_CHECK_EQUAL can't output std::vector<> to std::cout; use check instead
	BOOST_CHECK(array.dim() == tval);
	// contents should have been copied
	BOOST_CHECK_EQUAL(array({0,0,0,0}), 1.0);
	BOOST_CHECK_EQUAL(array({2,0,0,0}), 3.0);
	BOOST_CHECK_EQUAL(array({0,1,0,0}), 4.0);
	BOOST_CHECK_EQUAL(array({0,0,1,0}), 10.0);
	BOOST_CHECK_EQUAL(array({2,2,2,0}), (double)ar.numelems());
	array.resize({3,3});
	BOOST_CHECK_EQUAL(array.numdims(), 2);
	BOOST_CHECK_EQUAL(array.numelems(), 3 * 3);
	tval = {3,3};
	BOOST_CHECK(array.dim() == tval);
	// contents should have been copied
	BOOST_CHECK_EQUAL(array({0,0}), 1.0);
	BOOST_CHECK_EQUAL(array({2,0}), 3.0);
	BOOST_CHECK_EQUAL(array({0,1}), 4.0);
}

BOOST_AUTO_TEST_CASE(MoveConstruct) {
	dg::NddArray<double> ar(std::move(array));
	BOOST_CHECK_EQUAL(ar.empty(), false);
	BOOST_CHECK_EQUAL(array.empty(), true);
}

BOOST_AUTO_TEST_CASE(MoveAssign) {
	dg::NddArray<double> ar;
	BOOST_CHECK_EQUAL(ar.empty(), true);
	ar = std::move(array);
	BOOST_CHECK_EQUAL(ar.empty(), false);
	BOOST_CHECK_EQUAL(array.empty(), true);
}

BOOST_AUTO_TEST_CASE(Serialization) {
	std::stringstream ss;
	{ // scope of output archive
		boost::archive::xml_oarchive oa(ss);
		oa << BOOST_SERIALIZATION_NVP(array);
	}
	dg::NddArray<double> ina;
	BOOST_CHECK_EQUAL(ina.empty(), true);
	BOOST_CHECK(ina != array);
	{ // scope of input archive
		boost::archive::xml_iarchive ia(ss);
		ia >> boost::serialization::make_nvp("array", ina);
	}
	BOOST_CHECK_EQUAL(ina.empty(), false);
	BOOST_CHECK(ina == array);
}

BOOST_AUTO_TEST_CASE(ArrayCopyReg) {
	double a[16] = { 0 };
	BOOST_CHECK_THROW(array.copyTo(a), dg::DimensionMismatch);
	BOOST_CHECK_NO_THROW(array.copyFrom(a));
	BOOST_CHECK_EQUAL(array.numdims(), 1);
	BOOST_CHECK_EQUAL(array.dim(0), 16);
}

BOOST_AUTO_TEST_CASE(ArrayCopyStd) {
	std::array<double, 16> a;
	BOOST_CHECK_THROW(array.copyTo(a), dg::DimensionMismatch);
	BOOST_CHECK_NO_THROW(array.copyFrom(a));
	BOOST_CHECK_EQUAL(array.numdims(), 1);
	BOOST_CHECK_EQUAL(array.dim(0), 16);
}

BOOST_AUTO_TEST_CASE(ArrayCopyVec) {
	std::vector<double> a = { 42 };
	BOOST_CHECK_THROW(array.copyTo(a), dg::DimensionMismatch);
	BOOST_CHECK_NO_THROW(array.copyFrom(a));
	BOOST_CHECK_EQUAL(array.numdims(), 1);
	BOOST_CHECK_EQUAL(array.dim(0), 1);
	BOOST_CHECK_EQUAL(array({0}), 42);
}

BOOST_AUTO_TEST_SUITE_END()



struct Array1D {
	dg::NddArray<int> array;
	Array1D() : array({16}) {
		// simple test contents
		dg::NddArray<int>::iterator i = array.begin();
		for (int n = 1; i != array.end(); ++n, ++i) {
			*i = n;
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(NddArray_1D, Array1D)

BOOST_AUTO_TEST_CASE(RegArray) {
	int a[16];
	array.copyTo(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(array({n}), a[n]);
	}
	dg::NddArray<int> b;
	b.copyFrom(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(b({n}), a[n]);
	}
	BOOST_CHECK(array == b);
}

BOOST_AUTO_TEST_CASE(StdArray) {
	std::array<int, 16> a;
	array.copyTo(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(array({n}), a[n]);
	}
	dg::NddArray<int> b;
	b.copyFrom(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(b({n}), a[n]);
	}
	BOOST_CHECK(array == b);
}

BOOST_AUTO_TEST_CASE(StdVector) {
	std::vector<int> a = { 42 };
	array.copyTo(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(array({n}), a[n]);
	}
	dg::NddArray<int> b;
	b.copyFrom(a);
	for (dg::NddArray<int>::SizeType n = 0; n < 16; ++n) {
		BOOST_CHECK_EQUAL(b({n}), a[n]);
	}
	BOOST_CHECK(array == b);
}

BOOST_AUTO_TEST_SUITE_END()



struct Array2D {
	dg::NddArray<int> array;
	Array2D() : array({4,4}) {
		// simple test contents
		dg::NddArray<int>::iterator i = array.begin();
		for (int n = 1; i != array.end(); ++n, ++i) {
			*i = n;
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(NddArray_2D, Array2D)

BOOST_AUTO_TEST_CASE(RegArray) {
	int a[4][4];
	array.copyTo(a);
	for (dg::NddArray<int>::SizeType x = 0; x < 4; ++x) {
		for (dg::NddArray<int>::SizeType y = 0; y < 4; ++y) {
			BOOST_CHECK_EQUAL(array({x,y}), a[x][y]);
		}
	}
	dg::NddArray<int> b;
	b.copyFrom(a);
	for (dg::NddArray<int>::SizeType x = 0; x < 4; ++x) {
		for (dg::NddArray<int>::SizeType y = 0; y < 4; ++y) {
			BOOST_CHECK_EQUAL(b({x,y}), a[x][y]);
		}
	}
	BOOST_CHECK(array == b);
}

BOOST_AUTO_TEST_SUITE_END()
