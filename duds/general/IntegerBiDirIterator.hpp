/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef INTEGERBIDIRITERATOR_HPP
#define INTEGERBIDIRITERATOR_HPP

#include <iterator>

namespace duds { namespace general {

/**
 * A bidirectional iterator intended to work with sequential integers.
 * @tparam Int   The integer type.
 * @author  Jeff Jackowski
 */
template <typename Int>
class IntegerBiDirIterator {
	Int val;
public:
	typedef std::bidirectional_iterator_tag iterator_category;
	typedef Int value_type;
	typedef int difference_type;
	typedef value_type* pointer;
	typedef value_type& reference;
	IntegerBiDirIterator() = default;
	IntegerBiDirIterator(Int i) : val(i) { }
	IntegerBiDirIterator &operator++() {
		++val;
		return *this;
	}
	IntegerBiDirIterator operator++(int) {
		return IntegerBiDirIterator(val++);
	}
	IntegerBiDirIterator &operator--() {
		--val;
		return *this;
	}
	IntegerBiDirIterator operator--(int) {
		return IntegerBiDirIterator(val--);
	}
	Int operator * () const {
		return val;
	}
	bool operator != (const IntegerBiDirIterator &i) const {
		return val != i.val;
	}
};

} }

#endif        //  #ifndef INTEGERBIDIRITERATOR_HPP
