/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/variant/static_visitor.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/variant.hpp>
#include <duds/data/QuantityArray.hpp>
#include <duds/general/LanguageTaggedString.hpp>
#include <duds/time/interstellar/Interstellar.hpp>
#include <array>

#include <duds/data/ExtendedUnit.hpp>

namespace duds { namespace data {

class GenericValueArray;
class GenericValueTable;

template <typename N = double>
class GenericValueNumericVisitor : public boost::static_visitor<N> {
public:
	N operator() (const std::string &s) const {
		throw boost::bad_visit();
	}
	N operator() (const duds::general::LanguageTaggedString &s) const {
		throw boost::bad_visit();
	}
	N operator() (
	const boost::recursive_wrapper<duds::general::LanguageTaggedStringMap> &) const {
		throw boost::bad_visit();
	}
	N operator() (const N &n) const {
		return n;
	}
	N operator() (const Quantity &vu) const {
		return (N)vu.value;
	}
	N operator() (const QuantityNddArray &vu) const {
		throw boost::bad_visit();
	}
	template <class V>
	N operator() (const ExtendedQuantity<V> &vu) const {
		return (N)vu.value;
	}
	N operator() (const int128_w &i) const {
		return (N)i.value;
	}
	template<typename A, std::size_t S>
	N operator() (const std::array<A,S> &a) const {
		throw boost::bad_visit();
	}
	// !!! I sense trouble !!!
	template<class S, class R, template<class, class> class Duration = std::chrono::duration>
	N operator() (const Duration<S,R> &d) const {
		 return std::chrono::duration_cast< Duration< N, std::ratio<1,1> > >(d).count();
	} /*
	N operator() (const std::chrono::duration<S,R> &d) const {
		 return std::chrono::duration_cast< std::chrono::duration<N> >(d).count();
	} */
	N operator() (const boost::uuids::uuid &u) const {
		throw boost::bad_visit();
	}
	N operator() (const GenericValueTable &) const {
		throw boost::bad_visit();
	}
	/*
	N operator() (const GenericValueArray &) const {
		throw boost::bad_visit();
	}
	*/
	N operator() (const std::shared_ptr< std::vector<char> > &) const {
		throw boost::bad_visit();
	}
};

class GenericValueStringVisitor : public boost::static_visitor<std::string> {
public:
	std::string operator() (const std::string &s) const {
		return s;
	}
	std::string operator() (
		const duds::general::LanguageTaggedString &s
	) const {
		return s.string;
	}
	std::string operator() (
	const boost::recursive_wrapper<duds::general::LanguageTaggedStringMap> &) const {
		throw boost::bad_visit();
	}
	std::string operator() (const int128_w &n) const;
	std::string operator() (const double &n) const;
	std::string operator() (const boost::uuids::uuid &u) const;
	std::string operator() (const Quantity &q) const;
	std::string operator() (const QuantityNddArray &vu) const {
		throw boost::bad_visit();
	}
	template <class Q>
	std::string operator() (const ExtendedQuantity<Q> &q) const {
		std::ostringstream os;
		/** @todo  Include the unit name. */
		os << q.value;
		return os.str();
	}
	template<typename A, std::size_t S> //, template<typename, typename> class A = std::array>
	std::string operator() (const std::array<A,S> &a) const {
		std::ostringstream os;
		int l = 0;
		while (true) {
			os << a[l++];
			if (l < S) {
				os << ", ";
			} else {
				break;
			}
		}
		return os.str();
	}
	std::string operator() (const duds::time::interstellar::Femtoseconds &s) const;
	std::string operator() (const duds::time::interstellar::Nanoseconds &s) const;
	std::string operator() (const duds::time::interstellar::FemtoTime &t) const;
	std::string operator() (const duds::time::interstellar::NanoTime &t) const;
	std::string operator() (const GenericValueTable &) const {
		throw boost::bad_visit();
	}
	/*
	std::string operator() (const GenericValueArray &) const {
		throw boost::bad_visit();
	}
	*/
	std::string operator() (const std::shared_ptr< std::vector<char> > &) const {
		throw boost::bad_visit();
	}
};

template<class IST>
class GenericValueTimeVisitor : public boost::static_visitor<IST> {
	template <class I>
	IST conv(const I &in) {
		return IST(typename IST::duration(
			std::chrono::duration_cast<typename IST::period>(
				in.time_since_epoch()
			)
		) );
	}
	//template <>
	IST conv(const IST &in) {
		return in;
	}
public:
	IST operator() (const std::string &) const {
		throw boost::bad_visit();
	}
	IST operator() (
		const duds::general::LanguageTaggedString &
	) const {
		throw boost::bad_visit();
	}
	IST operator() (
	const boost::recursive_wrapper<duds::general::LanguageTaggedStringMap> &) const {
		throw boost::bad_visit();
	}
	IST operator() (const int128_w &) const {
		throw boost::bad_visit();
	}
	IST operator() (const double &) const {
		throw boost::bad_visit();
	}
	IST operator() (const boost::uuids::uuid &) const {
		throw boost::bad_visit();
	}
	IST operator() (const Quantity &q) const {
		throw boost::bad_visit();
	}
	IST operator() (const QuantityNddArray &) const {
		throw boost::bad_visit();
	}
	template<typename A, std::size_t S> //, template<typename, typename> class A = std::array>
	IST operator() (const std::array<A,S> &a) const {
		throw boost::bad_visit();
	}
	IST operator() (const duds::time::interstellar::Femtoseconds &) const {
		throw boost::bad_visit();
	}
	IST operator() (const duds::time::interstellar::Nanoseconds &) const {
		throw boost::bad_visit();
	}
	IST operator() (const duds::time::interstellar::FemtoTime &t) const {
		return conv(t);
	}
	IST operator() (const duds::time::interstellar::NanoTime &t) const {
		return conv(t);
	}
	IST operator() (const GenericValueTable &) const {
		throw boost::bad_visit();
	}
	/*
	IST operator() (const GenericValueArray &) const {
		throw boost::bad_visit();
	}
	*/
	IST operator() (const std::shared_ptr< std::vector<char> > &) const {
		throw boost::bad_visit();
	}
};

template<class IST>
class GenericValueDurationVisitor : public boost::static_visitor<IST> {
	template <class I>
	IST conv(const I &in) {
		return IST(typename IST::rep(
			std::chrono::duration_cast<typename IST::period>(in)
		) );
	}
	//template <>
	IST conv(const IST &in) {
		return in;
	}
public:
	IST operator() (const std::string &) const {
		throw boost::bad_visit();
	}
	IST operator() (
		const duds::general::LanguageTaggedString &
	) const {
		throw boost::bad_visit();
	}
	IST operator() (
	const boost::recursive_wrapper<duds::general::LanguageTaggedStringMap> &) const {
		throw boost::bad_visit();
	}
	IST operator() (const int128_w &) const {
		throw boost::bad_visit();
	}
	IST operator() (const double &) const {
		throw boost::bad_visit();
	}
	IST operator() (const boost::uuids::uuid &) const {
		throw boost::bad_visit();
	}
	IST operator() (const Quantity &q) const {
		// check for units of seconds
		if (q.unit == duds::data::units::Second) {
			return q.toDuration<IST>();
		} else {
			throw boost::bad_visit();
		}
	}
	IST operator() (const QuantityNddArray &) const {
		throw boost::bad_visit();
	}
	template<typename A, std::size_t S> //, template<typename, typename> class A = std::array>
	IST operator() (const std::array<A,S> &) const {
		throw boost::bad_visit();
	}
	IST operator() (const duds::time::interstellar::Femtoseconds &s) const {
		return conv(s);
	}
	IST operator() (const duds::time::interstellar::Nanoseconds &s) const {
		return conv(s);
	}
	IST operator() (const duds::time::interstellar::FemtoTime &) const {
		throw boost::bad_visit();
	}
	IST operator() (const duds::time::interstellar::NanoTime &) const {
		throw boost::bad_visit();
	}
	IST operator() (const GenericValueTable &) const {
		throw boost::bad_visit();
	}
	/*
	IST operator() (const GenericValueArray &) const {
		throw boost::bad_visit();
	}
	*/
	IST operator() (const std::shared_ptr< std::vector<char> > &) const {
		throw boost::bad_visit();
	}
};

} }
