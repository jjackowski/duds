/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef NDDARRAY_HPP
#define NDDARRAY_HPP

#include <vector>
#include <initializer_list>
#include <duds/general/Errors.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

namespace duds { namespace general {

/**
 * Base class for exceptions thrown by NddArray.
 * @todo  Consider dropping this in favor of using more general exception
 *        classes.
 */
struct NddArrayError : virtual std::exception, virtual boost::exception { };

/**
 * A specified dimension is zero.
 */
struct EmptyDimensionError : NddArrayError { };

/**
 * A specified dimension is beyond the range of the array.
 * Would be nice to also be a std::out_of_range, but having trouble getting
 * it to build.
 */
struct OutOfRangeError : NddArrayError /*, virtual std::out_of_range {
	OutOfRangeError() : std::out_of_range("") { }
}; */  { };

/**
 * A specified position has a different number of dimensions than the array.
 */
struct DimensionMismatchError : NddArrayError { };

/**
 * An empty array, one with zero dimensions, cannot be indexed.
 */
struct ZeroSizeError : DimensionMismatchError { };

/**
 * N-Dimensional Dynamic Array.
 *
 * A dynamically allocated array with a dynamic number of dimensions. Storage
 * internally is a std::vector for the dimensions, and a one dimensional array
 * for elements. Indexing into the array is expensive compared with arrays
 * of statically declared dimensions, but it allows for generalized data
 * storage when the dimensions, including the number of dimensions, are only
 * known at run-time.
 *
 * Unlike std::vector, resizing is always expensive. No extra space is ever
 * allocated.
 *
 * To specifiy positions or new dimensions, std::initializer_list and
 * std::vector may be used. The std::initializer_list is efficent for literal
 * and temporary values, and std::vector works well when less is known about
 * the dimensions at compile-time.
 *
 * Support is provided for Boost serialization with name-value pairs.
 *
 * One day there will be better iterators.
 * @todo   The functions begin() and end() allow for iterating over the
 *         elements already using a pointer as the iterator. Would be nice to
 *         have an iterator that provides position-value pairs, too, so the
 *         array position of the element can be easily queried.
 *
 * @tparam T  The element type to store. It must have a default constructor.
 *            Some operations require the assignment, equality, or inequality
 *            operators, but those operators are only required if used. Boost
 *            serialization support is required if the array is serialized.
 *
 * @author    Jeff Jackowski
 */
template <class T>
class NddArray {
public:
	/**
	 * The type used to store dimension lengths and the total number of elements.
	 */
	typedef std::size_t SizeType;
	/**
	 * The type used to store the dimensions of the array.
	 */
	typedef std::vector<SizeType> DimVec;
	/**
	 * A type that can specify the dimensions of the array, or a position of
	 * an element.
	 */
	typedef std::initializer_list<SizeType>  DimList;
	/**
	 * Simple iterator.
	 */
	typedef T* iterator;
	/**
	 * Simple const iterator.
	 */
	typedef const T* const_iterator;
private:
	/**
	 * The lengths of each dimension within the array.
	 */
	DimVec dsize;
	/**
	 * The array's element storage.
	 */
	T *array;
	/**
	 * Total number of elements; pre-calculated to speed up some operations.
	 */
	SizeType elems;
	/**
	 * Computes the new total number of elements and allocates space for the
	 * elements.
	 * @pre   @a dsize is not empty and is filled with the new dimensions.
	 *        @a array does not point to alloacted memory and may be
	 *        uninitialized.
	 * @post  @a array and @a elems are set. The default constructor of @a T
	 *        has been called for each element.
	 * @throw EmptyDimensionError   One of the elements in @a dims is zero. The
	 *                              array is changed to the empty state before
	 *                              throwing.
	 * @todo  Handle overflow of @a elems.
	 */
	void makeArray() {
		// work out the total number of elements
		DimVec::const_iterator iter = dsize.begin();
		elems = *(iter++);
		while (iter != dsize.end()) {
			elems *= *(iter++);
		}
		// check for a zero-size dimension
		if (!elems) {
			array = nullptr;
			dsize.clear();
			DUDS_THROW_EXCEPTION(EmptyDimensionError());
		}
		// allocate memory
		array = new T[elems];
	}
	/**
	 * Alloactes space for and copies the elements of the array. Used in the
	 * copy constructor and assignment operator.
	 * @pre   @a dsize and @a elems have already been copied. @a array does not
	 *        point to allocated memory and may be uninitialized.
	 * @post  @a array is allocated with copied elements, or is NULL if there
	 *        are no elements. If an element failed to copy, the array will
	 *        be cleared.
	 * @param src  The source array for the copy.
	 * @throw exception  An exception thrown from the copy assignment operator
	 *                   of @a T. The array is cleared before rethorwing.
	 */
	void copyElements(const NddArray &src) {
		// is there something to copy?
		if (elems) {
			// would be nice if constructors could be avoided here and the copy
			// constructor used instead of assignment in the first loop below
			array = new T[elems];
			T *t = array, *ot = src.array;
			try {
				// assign each element
				for (SizeType i = elems; i > 0; --i, ++t, ++ot) {
					*t = *ot;
				}
			} catch (...) {
				// dismantle what has been built
				clear();
				throw;
			}
		} else {
			// source is empty; array pointer could be anything
			array = nullptr;
		}
	}
	/**
	 * Finds the location in @a array that holds the element for the given
	 * n-dimensional array position and returns the element.
	 * @tparam Dim  The type holding the position. It must have forward
	 *              iterators.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not
	 *                                 match the number of dimensions in this
	 *                                 object.
	 * @throw  OutOfRangeError         A position is beyond the range of the
	 *                                 array.
	 */
	template <class Dim>
	const T &indexArray(const Dim &pos) const {
		// cannot index into an array with zero dimensions
		if (dsize.empty()) {
			DUDS_THROW_EXCEPTION(ZeroSizeError());
		}
		// the position must have as many items as dimensions in the array
		if (pos.size() != dsize.size()) {
			DUDS_THROW_EXCEPTION(DimensionMismatchError());
		}
		typename Dim::const_iterator p = pos.begin();
		DimVec::const_iterator d = dsize.begin();
		SizeType index = 0, step = 1;
		do {
			// range check
			if (*p >= *d) {
				DUDS_THROW_EXCEPTION(OutOfRangeError());
			}
			// advance index
			index += *p * step;
			// compute number of items to skip per unit in the next dimension
			step *= *d;
			// next position & dimension
			++p;
			++d;
		} while (d != dsize.end());
		// send back the requested elenent
		return array[index];
	}
public:
	/**
	 * Makes an empty array. It must be given a new size, by calling remake()
	 * or resize(), before it can be used for storage.
	 */
	NddArray() : array(nullptr), elems(0) { }
	/**
	 * Makes an array of the given size.
	 * @param dims  The initial dimensions.
	 */
	NddArray(const DimList &dims) : dsize(dims) {
		if (dims.size() > 0) {
			makeArray();
		}
	}
	/**
	 * Makes an array of the given size.
	 * @param dims  The initial dimensions.
	 */
	NddArray(const DimVec &dims) : dsize(dims) {
		if (!dims.empty()) {
			makeArray();
		}
	}
	/**
	 * Makes an array of the given size.
	 * @tparam Dim   The type holding the dimensions. It must have
	 *               forward iterators.
	 * @param  dims  The initial dimensions.
	 */
	template <class Dim>
	NddArray(const Dim &dims) : dsize(dims.first(), dims.last()) {
		if (!dsize.empty()) {
			makeArray();
		}
	}
	/**
	 * Copy constructor; uses the assignment operator of @a T.
	 * @param ndda  The array to copy.
	 * @throw exception  An exception thrown from the copy assignment operator
	 *                   of @a T. The array is cleared before rethorwing.
	 */
	NddArray(const NddArray &ndda) : elems(ndda.elems), dsize(ndda.dsize) {
		copyElements(ndda);
	}
	/**
	 * Move constructor; the elements are not copied.
	 * @param ndda  The array to move.
	 * @post  The source array, @a ndda, will be empty. The constructed array
	 *        will have the previous contents of @a ndda.
	 *
	 * @post  All iterators from @a ndda will continue to work with
	 *        this object.
	 */
	NddArray(NddArray &&ndda) noexcept :
	elems(ndda.elems), dsize(std::move(ndda.dsize)) {
		array = ndda.array;
		ndda.array = nullptr;
		ndda.elems = 0;
	}
	/**
	 * Destructor; destroys the elements of the array.
	 */
	~NddArray() noexcept {
		clear();
	}
	/**
	 * Copy assignment; uses the assignment operator of @a T.
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 * @param ndda  The array to copy.
	 * @throw exception  An exception thrown from the copy assignment operator
	 *                   of @a T. The array is cleared before rethorwing.
	 */
	NddArray &operator=(const NddArray &ndda) {
		dsize = ndda.dsize;
		elems = ndda.elems;
		delete [] array;
		copyElements(ndda);
		return *this;
	}
	/**
	 * Move assignment; the elements are not copied.
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used. All iterators from @a ndda will continue to work with
	 *        this object.
	 * @param ndda  The array to copy.
	 */
	NddArray &operator=(NddArray &&ndda) noexcept {
		elems = ndda.elems;
		dsize = std::move(ndda.dsize);
		delete [] array;
		array = ndda.array;
		ndda.array = nullptr;
		ndda.elems = 0;
		return *this;
	}
	/**
	 * Copies from a one dimensional container into this array.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam AV  A one dimensional container type with forward iterators.
	 * @param  av  The source container.
	 */
	template <class AV>
	void copyFrom(const AV &av) {
		// size the array to match the array/vector
		remake({av.size()});
		// copy elements
		T *dest = array;
		typename AV::const_iterator src = av.begin();
		for (; src != av.end(); ++src, ++dest) {
			*dest = *src;
		}
	}
	/**
	 * Copies from a std::array into this array.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam N  The length of the array.
	 * @param  a  The source array.
	 */
	template <std::size_t N>
	void copyFrom(const std::array<T, N> &a) {
		copyFrom< std::array< T, N > >(a);
	}
	/**
	 * Copies from a std::vector into this array.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @param  v  The source vector.
	 */
	void copyFrom(const std::vector<T> &v) {
		copyFrom< std::vector< T > >(v);
	}
	/**
	 * Copies from a one dimensional array type into this array object.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam N  The length of the array.
	 * @param  a  The source array.
	 */
	template <std::size_t N>
	void copyFrom(const T (&a)[N]) {
		// size the array to match the array
		remake({N});
		// copy elements
		T *dest = array;
		const T *src = a;
		for (std::size_t loop = N; loop; ++src, ++dest, --loop) {
			*dest = *src;
		}
	}
	/**
	 * Copies from a two dimensional array type into this array object.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam X  The length of the source array's first dimension.
	 * @tparam Y  The length of the source array's second dimension.
	 * @param  a  The source array.
	 */
	template <std::size_t X, std::size_t Y>
	void copyFrom(const T (&a)[X][Y]) {
		// size the array to match the array
		remake({X,Y});
		// copy elements
		T *dest = array;
		std::size_t x, y = 0;
		for (; y < Y; ++y) {
			for (x = 0; x < X; ++dest, ++x) {
				*dest = a[x][y];
			}
		}
	}
	/**
	 * Copies from a std::array into this array.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam N  The length of the array.
	 * @param  a  The source array.
	 */
	template <std::size_t N>
	NddArray &operator=(const std::array<T, N> &a) {
		copyFrom< std::array< T, N > >(a);
		return *this;
	}
	/**
	 * Copies from a std::vector into this array.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @param  v  The source vector.
	 */
	NddArray &operator=(const std::vector<T> &v) {
		copyFrom< std::vector< T > >(v);
		return *this;
	}
	/**
	 * Copies from a one dimensional array type into this array object.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam N  The length of the array.
	 * @param  a  The source array.
	 */
	template <std::size_t N>
	NddArray &operator=(const T (&a)[N]) {
		copyFrom<N>(a);
		return *this;
	}
	/**
	 * Copies from a two dimensional array type into this array object.
	 * Each element is copied using the assignment operator.
	 *
	 * @post   The dimensions of this array will match the source.
	 *
	 * @post   All iterators on this object are invalid and must no longer
	 *         be used.
	 *
	 * @tparam X  The length of the array's first dimension.
	 * @tparam Y  The length of the array's second dimension.
	 * @param  a  The source array.
	 */
	template <std::size_t X, std::size_t Y>
	NddArray &operator=(const T (&a)[X][Y]) {
		copyFrom<X,Y>(a);
		return *this;
	}
	/**
	 * Copies the contents of this object into a std::array.
	 * Each element is copied using the assignment operator.
	 * @pre    This object must have a single dimension.
	 * @post   The number of copied elements is the smaller of the length
	 *         of this array and the length of the destination, @a N. Elements
	 *         in the destination array that are not copied over are also not
	 *         modified.
	 * @tparam N  The size of the array.
	 * @param  a  The destination array.
	 * @throw  DimensionMismatchError  This array does not have exactly one
	 *                            dimension.
	 */
	template <std::size_t N>
	void copyTo(std::array<T, N> &a) const {
		// must be exactly one dimension
		if (dsize.size() != 1) {
			DUDS_THROW_EXCEPTION(DimensionMismatchError());
		}
		// copy elements
		const T *src = array;
		typename std::array<T, N>::iterator i = a.begin();
		std::size_t loop = std::min(elems, a.size());
		for (; loop; ++i, ++src, --loop) {
			*i = *src;
		}
	}
	/**
	 * Copies the contents of this object into a std::vector.
	 * Each element is copied using the assignment operator.
	 * @pre    This object must have a single dimension.
	 * @post   The destination's size will match this array's size.
	 * @param  v  The destination vector.
	 * @throw  DimensionMismatchError  This array does not have exactly one
	 *                            dimension.
	 */
	void copyTo(std::vector<T> &v) const {
		// must be exactly one dimension
		if (dsize.size() != 1) {
			DUDS_THROW_EXCEPTION(DimensionMismatchError());
		}
		// resize the vector to match
		v.resize(elems);
		// copy elements
		const T *src = array;
		typename std::vector<T>::iterator i = v.begin();
		for (; i != v.end(); ++i, ++src) {
			*i = *src;
		}
	}
	/**
	 * Copies the contents of this object into a one dimensional array.
	 * Each element is copied using the assignment operator.
	 * @pre    This object must have a single dimension.
	 * @post   The number of copied elements is the smaller of the length
	 *         of this array and the length of the destination, @a N. Elements
	 *         in the destination array that are not copied over are also not
	 *         modified.
	 * @tparam N  The size of the array.
	 * @param  a  The destination array.
	 * @throw  DimensionMismatchError  This array does not have exactly one
	 *                            dimension.
	 */
	template <std::size_t N>
	void copyTo(T (&a)[N]) const {
		// must be exactly one dimension
		if (dsize.size() != 1) {
			DUDS_THROW_EXCEPTION(DimensionMismatchError());
		}
		// copy elements
		const T *src = array;
		T *dest = a;
		std::size_t loop = std::min(elems, N);
		for (; loop; ++dest, ++src, --loop) {
			*dest = *src;
		}
	}
	/**
	 * Copies the contents of this object into a two dimensional array.
	 * Each element is copied using the assignment operator.
	 * @pre    This object must have exactly two dimensions.
	 * @post   The copied area is the intersection of the source and
	 *         destination arrays. Elements in the destination array that
	 *         are not copied over are also not modified.
	 * @tparam X  The length of the destination array's first dimension.
	 * @tparam Y  The length of the destination array's second dimension.
	 * @param  a  The destination array.
	 * @throw  DimensionMismatchError  This array does not have exactly two
	 *                            dimensions.
	 */
	template <std::size_t X, std::size_t Y>
	void copyTo(T (&a)[X][Y]) const {
		// must be exactly two dimensions
		if (dsize.size() != 2) {
			DUDS_THROW_EXCEPTION(DimensionMismatchError());
		}
		// copy elements
		const T *src;
		const std::size_t xs = std::min(X, dsize[0]);
		const std::size_t ys = std::min(Y, dsize[1]);
		std::size_t x, y = 0;
		for (; y < ys; ++y) {
			src = &at({0,y});
			for (x = 0; x < xs; ++src, ++x) {
				a[x][y] = *src;
			}
		}
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @tparam Dim  The type holding the position. It must have forward
	 *              iterators.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	template <class Dim>
	T &operator()(const Dim &pos) {
		// remove the const specifier from the return value of indexArray()
		// to avoid writing the function twice
		return const_cast<T&>(indexArray(pos));
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	T &operator()(const DimList &pos) {
		// remove the const specifier from the return value of indexArray()
		// to avoid writing the function twice
		return const_cast<T&>(indexArray<DimList>(pos));
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @tparam Dim  The type holding the position. It must have forward
	 *              iterators.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	template <class Dim>
	const T &operator()(const Dim &pos) const {
		return indexArray(pos);
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	const T &operator()(const DimList &pos) const {
		return indexArray<DimList>(pos);
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @tparam Dim  The type holding the position. It must have forward
	 *              iterators.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	template <class Dim>
	T &at(const Dim &pos) {
		// remove the const specifier from the return value of indexArray()
		// to avoid writing the function twice
		return const_cast<T&>(indexArray(pos));
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	T &at(const DimList &pos) {
		// remove the const specifier from the return value of indexArray()
		// to avoid writing the function twice
		return const_cast<T&>(indexArray<DimList>(pos));
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @tparam Dim  The type holding the position. It must have forward
	 *              iterators.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	template <class Dim>
	const T &at(const Dim &pos) const {
		return indexArray(pos);
	}
	/**
	 * Obtain an element from the array stored at a specific position.
	 * @param  pos  The position. It's number of elements must match the number
	 *              of dimensions of this array.
	 * @return      The element at the position.
	 * @throw  ZeroSizeError           This object is empty and has no dimensions.
	 * @throw  DimensionMismatchError  The number of items in @a pos does not match
	 *                            the number of dimensions in this object.
	 * @throw  OutOfRangeError         A position is beyond the range of the array.
	 */
	const T &at(const DimList &pos) const {
		return indexArray<DimList>(pos);
	}
	/**
	 * The first element of the array. Its position is zero for all dimensions.
	 * @return  The first element.
	 * @throw   ZeroSizeError   This object is empty; there is no first element.
	 */
	T &front() {
		if (!array) {
			DUDS_THROW_EXCEPTION(ZeroSizeError());
		}
		return *array;
	}
	/**
	 * The first element of the array. Its position is zero for all dimensions.
	 * @return  The first element.
	 * @throw   ZeroSizeError   This object is empty; there is no first element.
	 */
	const T &front() const {
		if (!array) {
			DUDS_THROW_EXCEPTION(ZeroSizeError());
		}
		return *array;
	}
	/**
	 * The last element of the array. Its position is the maximum value for
	 * all dimensions.
	 * @return  The last element.
	 * @throw   ZeroSizeError   This object is empty; there is no last element.
	 */
	T &back() {
		if (!array) {
			DUDS_THROW_EXCEPTION(ZeroSizeError());
		}
		return array[elems - 1];
	}
	/**
	 * The last element of the array. Its position is the maximum value for
	 * all dimensions.
	 * @return  The last element.
	 * @throw   ZeroSizeError   This object is empty; there is no last element.
	 */
	const T &back() const {
		if (!array) {
			DUDS_THROW_EXCEPTION(ZeroSizeError());
		}
		return array[elems - 1];
	}
	/**
	 * An iterator (really just a pointer) to the first element of the array.
	 * If the array is empty, the pointer will be null. There is nothing in
	 * place to prevent misuse of the iterator since it is really a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	T *begin() {
		return array;
	}
	/**
	 * An iterator (really just a pointer) to the first element of the array.
	 * If the array is empty, the pointer will be null. There is nothing in
	 * place to prevent misuse of the iterator since it is really a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	const T *begin() const {
		return array;
	}
	/**
	 * An iterator (really just a pointer) to the first element of the array.
	 * If the array is empty, the pointer will be null. There is nothing in
	 * place to prevent misuse of the iterator since it is really a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	const T *cbegin() const {
		return array;
	}
	/**
	 * An iterator (really just a pointer) to one past the last element of
	 * the array. If the array is empty, the pointer will be null. There is
	 * nothing in place to prevent misuse of the iterator since it is really
	 * a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	T *end() {
		return array + elems;
	}
	/**
	 * An iterator (really just a pointer) to one past the last element of
	 * the array. If the array is empty, the pointer will be null. There is
	 * nothing in place to prevent misuse of the iterator since it is really
	 * a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	const T *end() const {
		return array + elems;
	}
	/**
	 * An iterator (really just a pointer) to one past the last element of
	 * the array. If the array is empty, the pointer will be null. There is
	 * nothing in place to prevent misuse of the iterator since it is really
	 * a pointer.
	 *
	 * The iterator is valid until the array is resized. If the array is
	 * swapped with another, the iterator will function on the same dataset;
	 * it will follow the swap.
	 */
	const T *cend() const {
		return array + elems;
	}
	/**
	 * Equality operator; uses the equality operator of @a T. Elements are
	 * only inspected if the array dimensions are the same.
	 * @param ndda  The array to compare.
	 */
	bool operator==(const NddArray &ndda) const {
		// check dimensions
		if ((elems != ndda.elems) || (dsize != ndda.dsize)) {
			return false;
		}
		// same dimensions; check array elements
		T *t = array, *o = ndda.array;
		for (SizeType i = elems; i; --i, ++t, ++o) {
			if (!(*t == *o)) {
				return false;
			}
		}
		// everything matches
		return true;
	}
	/**
	 * Inequality operator; uses the inequality operator of @a T. Elements are
	 * only inspected if the array dimensions are the same.
	 * @param ndda  The array to compare.
	 */
	bool operator!=(const NddArray &ndda) const {
		// check dimensions
		if ((elems != ndda.elems) || (dsize != ndda.dsize)) {
			return true;
		}
		// same dimensions; check array elements
		T *t = array, *o = ndda.array;
		for (SizeType i = elems; i; --i, ++t, ++o) {
			if (*t != *o) {
				return true;
			}
		}
		// everything matches
		return false;
	}
	/**
	 * Returns the number of dimensions in the array.
	 */
	SizeType numdims() const {
		return dsize.size();
	}
	/**
	 * Returns the size of dimension @a n.
	 * @param n  The number of the dimension to inspect. Zero is the first
	 *           dimension.
	 * @throw OutOfRangeError  The array has fewer than @a n dimensions.
	 */
	SizeType dim(SizeType n) const {
		if (n >= dsize.size()) {
			DUDS_THROW_EXCEPTION(OutOfRangeError());
		}
		return dsize[n];
	}
	/**
	 * Provides access to the vector containing the array dimensions.
	 */
	const DimVec &dim() const {
		return dsize;
	}
	/**
	 * Returns the total number of elements within the array. The value is
	 * already computed, so the result is quickly obtained.
	 */
	SizeType numelems() const {
		return elems;
	}
	/**
	 * True if the array has zero dimensions.
	 */
	bool empty() const {
		return !array;
	}
	/**
	 * Destroys the contents of the array.
	 * @post  The elements are destructed. The internal array is deallocated.
	 *        The array is empty; it has zero dimensions.
	 *
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 */
	void clear() noexcept { // destructors must never throw
		delete [] array;
		array = nullptr;
		dsize.clear();
		elems = 0;
	}
	/**
	 * Clears the array and allocates a new one of the given dimensions.
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 * @param dims  The new dimensions for the array. If it has no elements,
	 *              the array will have zero dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero.
	 */
	void remake(const DimList &dims) {
		// no dimensions?
		if (dims.size() == 0) {
			// be empty
			clear();
			return;
		}
		// destroy existing elements
		delete [] array;
		// store new dimensions
		dsize = dims;
		// make stuff
		makeArray();
	}
	/**
	 * Clears the array and allocates a new one of the given dimensions.
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 * @param dims  The new dimensions for the array. If it is empty,
	 *              the array will have zero dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero.
	 */
	void remake(const DimVec &dims) {
		// no dimensions?
		if (dims.empty()) {
			// be empty
			clear();
			return;
		}
		// destroy existing elements
		delete [] array;
		// store new dimensions
		dsize = dims;
		// make stuff
		makeArray();
	}
	/**
	 * Makes a new array with a new size and copies elements who's position is
	 * within bounds of the new array's dimensions. The operation uses the copy
	 * assignment operator of @a T to place the existing elements. On failure,
	 * the new array is destroyed.
	 *
	 * The cases of no dimensions (empty) for the existing array, and none for
	 * the new array, are handled as special cases so they can complete faster.
	 *
	 * @param dims  The new dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero.
	 * @throw exception       An exception thrown from the copy assignment
	 *                        operator of @a T.
	 */
	template <class Dim>
	NddArray makeWithNewSize(const Dim &dims) const {
		// no dimensions in new size?
		if (dims.size() == 0) {
			// empty
			return NddArray();
		}
		// no dimensions in current size?
		if (!array) {
			// make array of given size
			return NddArray(dims);
		}
		// new array for the new size
		NddArray na(dims);  // may throw EmptyDimensionError
		// union of source & destination dimensions
		DimVec uniondim(std::min(dsize.size(), dims.size()));
		typename Dim::const_iterator diter = dims.begin();
		SizeType idx = 0;  // dimension index
		do {
			// Find the length covered by old & new size. The result will be
			// greater than zero. If any length was zero, the construction of na
			// above threw an exception, so no need to check here.
			uniondim[idx] = (std::min(dsize[idx], *diter));
			++diter;
		} while (++idx < uniondim.size());
		idx = 0;
		// position vectors for source & destination
		DimVec spos(numdims(), 0), dpos(na.numdims());
		// pointers to inside the arrays
		const T *sitm = array;  // source
		T *ditm = na.array;     // destination
		// loop to copy
		do {
			// copy element; first element can always be copied
			*ditm = *sitm;
			// loop to advance position
			do {
				// next position
				spos[idx] += 1;
				// check range on this dimension
				if (spos[idx] >= uniondim[idx]) {
					// went beyond; go back to start of this dimension
					spos[idx] = 0;
				} else {
					// in bounds; use this position
					break;
				}
			} while (++idx < uniondim.size());
			// modified 1st dimension only?
			if (!idx) {
				// The next element is the next in the 1D storage array. Advance
				// the pointers to quickly get to the next element.
				++sitm;
				++ditm;
			// modified more than one dimension in the position?
			} else if (idx < uniondim.size()) {
				// next time, start position advance at 1st dimension
				idx = 0;
				// new source address
				sitm = &at(spos);
				// set new destination position
				dpos = spos;
				dpos.resize(na.numdims());
				// new destination address
				ditm = &na.at(dpos);
			// modified all dimensions to zero?
			} else {
				// all done
				break;
			}
		} while (true);
		// return the new array
		return na;
	}
	/**
	 * Makes a new array with a new size and copies elements who's position is
	 * within bounds of the new array's dimensions. The operation uses the copy
	 * assignment operator of @a T to place the existing elements. On failure,
	 * the new array is destroyed.
	 *
	 * The cases of no dimensions (empty) for the existing array, and none for
	 * the new array, are handled as special cases so they can complete faster.
	 *
	 * @param dims  The new dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero.
	 * @throw exception       An exception thrown from the copy assignment
	 *                        operator of @a T.
	 */
	NddArray makeWithNewSize(const DimList &dims) const {
		return makeWithNewSize<DimList>(dims);
	}
	/**
	 * Resizes the array and keeps elements who's position is within bounds
	 * of the new dimensions. The operation creates a new array of the new
	 * dimensions and uses the copy assignment operator of @a T to place the
	 * existing elements. On failure, the array does not change.
	 *
	 * No dimensions for the new or current size are handled as special cases
	 * so they can complete faster.
	 *
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 *
	 * @param dims  The new dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero. The array
	 *                        will be unchanged.
	 * @throw exception       An exception thrown from the copy assignment
	 *                        operator of @a T. The array will be unchanged.
	 * @todo  Make another version (noexcept?) of this function that uses the
	 *        move assignment operator. Failures will recover slower, but
	 *        successes could be much faster.
	 */
	template <class Dim>
	void resize(const Dim &dims) {
		*this = std::move(makeWithNewSize(dims));
	}
	/**
	 * Resizes the array and keeps elements who's position is within bounds
	 * of the new dimensions. The operation creates a new array of the new
	 * dimensions and uses the copy assignment operator of @a T to place the
	 * existing elements. On failure, the array does not change.
	 *
	 * No dimensions for the new or current size are handled as special cases
	 * so they can complete faster.
	 *
	 * @post  All iterators on this object are invalid and must no longer
	 *        be used.
	 *
	 * @param dims  The new dimensions.
	 * @throw EmptyDimensionError  One of the elements in @a dims is zero. The array
	 *                        will be unchanged.
	 * @throw exception       An exception thrown from the copy assignment
	 *                        operator of @a T. The array will be unchanged.
	 * @todo  Make another version (noexcept?) of this function that uses the
	 *        move assignment operator. Failures will recover slower, but
	 *        successes could be much faster.
	 */
	void resize(const DimList &dims) {
		*this = std::move(makeWithNewSize<DimList>(dims));
	}
	/**
	 * Returns the number of elements stored in the array.
	 */
	SizeType size() const {
		return elems;
	}
	/**
	 * Swaps array contents. The elements are not copied; it should be fairly
	 * quick.
	 * @post  Iterators working on this object will continue to work on
	 *        @a other.
	 * @param other  The other array involved in the swap.
	 */
	void swap(NddArray &other) {
		dsize.swap(other.dsize);
		std::swap(array, other.array);
		std::swap(elems, other.elems);
	};
private:
	// serialization support
	friend class boost::serialization::access;
	BOOST_SERIALIZATION_SPLIT_MEMBER();
	/**
	 * Boost serialization support.
	 */
	template <class A>
	void save(A &a, const unsigned int) const {
		// store the dimensions
		a & BOOST_SERIALIZATION_NVP(dsize);
		// store more only if not empty
		if (!dsize.empty()) {
			// write out elements
			const T *t = array;
			for (SizeType i = elems; i > 0; --i, ++t) {
				a & boost::serialization::make_nvp("item", *t);
			}
		}
	}
	/**
	 * Boost serialization support.
	 */
	template <class A>
	void load(A &a, const unsigned int) {
		// destroy existing contents
		if (array) {
			clear();
		}
		// recover the dimensions
		a & BOOST_SERIALIZATION_NVP(dsize);
		// there is only more if the array isn't empty
		if (!dsize.empty()) {
			try {
				// allocate space
				makeArray();
				// read in elements
				T *t = array;
				for (SizeType i = elems; i > 0; --i, ++t) {
					a & boost::serialization::make_nvp("item", *t);
				}
			} catch (...) {
				// not sure if this is needed
				clear();
				throw;
			}
		}
	}
};

/**
 * Makes NddAray meet the requirements of Swappable to assist in using
 * std::swap().
 */
template <class T>
void swap(NddArray<T> &one, NddArray<T> &two) {
	one.swap(two);
}

} }

#endif        //  #ifndef NDDARRAY_HPP

