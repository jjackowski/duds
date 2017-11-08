/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef QUANTITYARRAY_HPP
#define QUANTITYARRAY_HPP

#include <duds/data/Quantity.hpp>
#include <duds/general/NddArray.hpp>
#include <array>

namespace duds { namespace data {

struct QuantityNddArray;

/**
 * An iterator template for QuantityArray and QuantityNddArray that provides
 * a Quantity object when dereferenced. It is intended to be a
 * BidirectionalIterator. It fails to be an OutputIterator because
 * dereferencing the iterator creates and returns a new Quantity object.
 * However, the underlying value is available and writable through value().
 * The Unit for all quantities is also accessible, but not writable, through
 * unit().
 *
 * @author  Jeff Jackowski
 */
template <class I>
class QuantityIterator {
	/**
	 * The iterator wrapped by this object.
	 */
	I iter;
	/**
	 * The units used for all Quantities in the array. This could have been
	 * a pointer to the array, but that would use as much or more memory.
	 */
	Unit arrayUnit;
public:
	/**
	 * Make an iterator to nowhere.
	 */
	QuantityIterator() = default;
	/**
	 * Construct a new iterator with the given units.
	 * @param i  The wrapped iterator. It must yield a double when dereferenced.
	 * @param u  The units of all items the iterator can reference.
	 */
	QuantityIterator(const I &i, Unit u) : iter(i), arrayUnit(u) { }
	QuantityIterator &operator ++ () {
		++iter;
		return *this;
	}
	QuantityIterator &operator ++ (int) {
		QuantityIterator i(iter);
		++iter;
		return i;
	}
	QuantityIterator &operator -- () {
		--iter;
		return *this;
	}
	QuantityIterator &operator -- (int) {
		QuantityIterator i(iter);
		--iter;
		return i;
	}
	bool operator == (const QuantityIterator &it) const {
		return iter == it.iter;
	}
	bool operator != (const QuantityIterator &it) const {
		return iter != it.iter;
	}
	/**
	 * Returns a new Quantity object. Since the object is created here, it
	 * cannot be used to change the data in the container.
	 */
	Quantity operator * () const {
		return Quantity(*iter, arrayUnit);
	}
	/**
	 * Returns a reference to the value stored in the container.
	 * @bug  This may fail when @a I is a const_iterator. The problem could
	 *       be fixed easily by specifying a return type of auto, but that
	 *       requires C++14. Not sure how to solve this with type traits.
	 */
	double &value() const {
	//auto value() const { // needs C++14; crashes gcc 4.8.5
		return *iter;
	}
	/**
	 * Returns the units of all quantities stored in the container. The units
	 * cannot be changed through the iterator.
	 */
	Unit unit() const {
		return arrayUnit;
	}
};

/**
 * A fixed size array of quantities all sharing the same units.
 * A std::array is used to store the quantity values, doubles,
 * instead of Quantity objects. This reduces the memory usage over having an
 * array of Quantity objects. Functions are provided that work with Quantity
 * objects.
 *
 * @tparam L  The length of the array.
 *
 * @author  Jeff Jackowski
 */
template <std::size_t L>
struct QuantityArray {
	/**
	 * The type of the array holding quantity values.
	 */
	typedef std::array<double, L>  Array;
	/**
	 * The array of quantity values.
	 */
	Array array;
	/**
	 * The units of all values in the array.
	 */
	Unit unit;
	/**
	 * An iterator type that will yield Quantity objects when dereferenced.
	 */
	typedef QuantityIterator<typename Array::iterator>  iterator;
	/**
	 * A const iterator type that will yield Quantity objects when dereferenced.
	 */
	typedef QuantityIterator<typename Array::const_iterator>  const_iterator;
	iterator begin() {
		return iterator(array.begin(), unit);
	}
	const_iterator begin() const {
		return const_iterator(array.begin(), unit);
	}
	const_iterator cbegin() const {
		return const_iterator(array.cbegin(), unit);
	}
	iterator end() {
		return iterator(array.end(), unit);
	}
	const_iterator end() const {
		return const_iterator(array.end(), unit);
	}
	const_iterator cend() const {
		return const_iterator(array.cend(), unit);
	}
	/**
	 * Returns a new Quantity object for the requested position.
	 * @param pos  The position in the array.
	 * @throw std::out_of_range Thrown by Array::at() when the position is
	 *                          outside the array's boundries.
	 */
	Quantity get(std::size_t pos) const {
		return Quantity(array.at(pos), unit);
	}
	/**
	 * Sets a stored quantity to be the same as given Quantity object.
	 * @pre   The given Quantity has the same units as all quantites in this
	 *        array (@a unit).
	 * @param pos  The position in the array.
	 * @param q    The quantity to store.
	 * @throw UnitMismatch       The units of @a q are not the same as the
	 *                           units of this array.
	 * @throw std::out_of_range  Thrown by Array::at() when the position is
	 *                           outside the array's boundries.
	 */
	void set(std::size_t pos, const Quantity &q) {
		if (unit != q.unit) {
			BOOST_THROW_EXCEPTION(UnitMismatch());
		}
		array.at(pos) = q.value;
	}
	/**
	 * Copies one QuantityArray into another for an exact match.
	 */
	QuantityArray &operator=(const QuantityArray &a) = default;
	/**
	 * Copies one QuantityArray into another; sizes do not need to match.
	 */
	template <std::size_t N>
	QuantityArray &copy(const QuantityArray<N> &a) {
		unit = a.unit;
		std::size_t loop = std::min(L, N);
		typename Array::iterator dest = array.begin();
		typename QuantityArray<N>::Array::const_iterator src = a.array.begin();
		for (; loop; ++dest, ++src, --loop) {
			*dest = *src;
		}
		return *this;
	}
	/**
	 * Copies the contents of a QuantityNddArray into this object.
	 * @param a  The source array. It must be one dimensional. The intersection
	 *           of the elements will be copied; the sizes do not need to
	 *           match. If @a a is smaller than this array, some elements will
	 *           remain unchanged.
	 */
	QuantityArray &operator=(const QuantityNddArray &a); // defined near EOF
};

/**
 * A QuantityArray for the common usage of a three dimentional coordinate or
 * a triple axis sample.
 */
struct QuantityXyz : public QuantityArray<3> {
	Quantity x() const {
		return get(0);
	}
	Quantity y() const {
		return get(0);
	}
	Quantity z() const {
		return get(0);
	}
};

/**
 * An array of quantites of dynamic size and number of dimensions.
 * A duds::general::NddArray is used to hold all quantity values, doubles,
 * and the units are held separately. This reduces the memory usage over
 * having an array of Quantity objects. Functions are provided that work with
 * Quantity objects.
 *
 * @author  Jeff Jackowski
 */
struct QuantityNddArray {
	/**
	 * The type of the array holding quantity values.
	 */
	typedef duds::general::NddArray<double>  Array;
	/**
	 * The array of quantity values.
	 */
	Array array;
	/**
	 * The units of all values in the array.
	 */
	Unit unit;
	/**
	 * The default constructor; expect @a unit to be uninitialized.
	 * @post  @a array is empty and @a unit is uninitialized.
	 */
	QuantityNddArray() = default;
	/**
	 * A default copy constrcutor.
	 */
	QuantityNddArray(const QuantityNddArray &) = default;
	/**
	 * Move constructor; the array of quantities is moved rather than copied.
	 */
	QuantityNddArray(QuantityNddArray &&q) noexcept :
	array(std::move(q.array)), unit(q.unit) { }
	/**
	 * Copies one QuantityNddArray into another for an exact match.
	 */
	QuantityNddArray &operator=(const QuantityNddArray &a) = default;
	/**
	 * Move assignment; the array of quantities is moved rather than copied.
	 */
	QuantityNddArray &operator=(QuantityNddArray &&q) = default;
	/**
	 * Copies the contents of a QuantityArray into this object.
	 * @post   The dimensions of this object will match @a q.
	 * @tparam N  The length of the source array.
	 * @param  q  The source array.
	 */
	template <std::size_t N>
	QuantityNddArray &operator=(const QuantityArray<N> &q) {
		array.copyFrom(q.array);
		unit = q.unit;
		return *this;
	}
	/**
	 * Clears the array and units.
	 */
	void clear() noexcept {
		array.clear();
		unit.clear();
	}
	/**
	 * True if the array is empty; units are immateral.
	 */
	bool empty() const {
		return array.empty();
	}
	/**
	 * An iterator type that will yield Quantity objects when dereferenced.
	 */
	typedef QuantityIterator<typename Array::iterator>  iterator;
	/**
	 * A const iterator type that will yield Quantity objects when dereferenced.
	 */
	typedef QuantityIterator<typename Array::const_iterator>  const_iterator;
	iterator begin() {
		return iterator(array.begin(), unit);
	}
	const_iterator begin() const {
		return const_iterator(array.begin(), unit);
	}
	const_iterator cbegin() const {
		return const_iterator(array.cbegin(), unit);
	}
	iterator end() {
		return iterator(array.end(), unit);
	}
	const_iterator end() const {
		return const_iterator(array.end(), unit);
	}
	const_iterator cend() const {
		return const_iterator(array.cend(), unit);
	}
	/**
	 * Returns a new Quantity object for the requested position.
	 * @tparam Dim   The type holding the position. It must have forward
	 *               iterators.
	 * @param  pos   The position in the array.
	 * @throw  duds::data::general::OutOfRange   Thrown by Array::at() when
	 *                                           the position is outside the
	 *                                           array's boundries.
	 */
	template <class Dim>
	Quantity get(const Dim &pos) const {
		return Quantity(array.at<Dim>(pos), unit);
	}
	/**
	 * Returns a new Quantity object for the requested position.
	 * @param pos  The position in the array.
	 * @throw OutOfRange  Thrown by Array::at() when the position is
	 *                    outside the array's boundries.
	 */
	Quantity get(const Array::DimList &pos) const {
		return Quantity(array.at<Array::DimList>(pos), unit);
	}
	/**
	 * Sets a stored quantity to be the same as given Quantity object.
	 * @pre   The given Quantity has the same units as all quantites in this
	 *        array (@a unit).
	 * @tparam Dim   The type holding the position. It must have forward
	 *               iterators.
	 * @param  pos  The position in the array.
	 * @param  q    The quantity to store.
	 * @throw  UnitMismatch     The units of @a q are not the same as the
	 *                          units of this array.
	 * @throw  duds::data::general::OutOfRange   Thrown by Array::at() when
	 *                                           the position is outside the
	 *                                           array's boundries.
	 */
	template <class Dim>
	Quantity set(const Dim &pos, const Quantity &q) {
		if (unit != q.unit) {
			BOOST_THROW_EXCEPTION(UnitMismatch());
		}
		array.at<Dim>(pos) = q.value;
	}
	/**
	 * Sets a stored quantity to be the same as given Quantity object.
	 * @pre   The given Quantity has the same units as all quantites in this
	 *        array (@a unit).
	 * @param  pos  The position in the array.
	 * @param  q    The quantity to store.
	 * @throw  UnitMismatch     The units of @a q are not the same as the
	 *                          units of this array.
	 * @throw  duds::data::general::OutOfRange   Thrown by Array::at() when
	 *                                           the position is outside the
	 *                                           array's boundries.
	 */
	Quantity set(const Array::DimList &pos, const Quantity &q) {
		if (unit != q.unit) {
			BOOST_THROW_EXCEPTION(UnitMismatch());
		}
		array.at<Array::DimList>(pos) = q.value;
	}

private:
	// serialization support
	friend class boost::serialization::access;
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(array);
		a & BOOST_SERIALIZATION_NVP(unit);
	}
};

// documented above with the QuantityArray class
template <std::size_t L>
QuantityArray<L> &QuantityArray<L>::operator=(const QuantityNddArray &a) {
	unit = a.unit;
	a.array.copyTo(array);
	return *this;
}


} }

#endif        //  #ifndef QUANTITYARRAY_HPP
