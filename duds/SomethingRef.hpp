/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * An idea; use or pitch?
 */
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
//#include <duds/general/LanguageTaggedString.hpp>
//#include <duds/GenericValue.hpp>
#include <memory>

namespace duds {

using std::shared_ptr;
using std::weak_ptr;

/**
 * A weak reference to a Something object. This object has its own copy of the
 * Something's UUID and can be compared with Something and SomethingRef objects
 * based on the UUID without accessing the referenced Something. This allows
 * for containers that hold sets of sorted Something objects without managing
 * the memory or lifespan of the objects.
 * @tparam  ST   The Something type. It should be Something or a class derived
 *               from Something.
 * @todo    Add templated conversions between compatible SomethingWeakRef
 *          types using different @a ST.
 * @author  Jeff Jackowski
 * @todo    Decide if this should be used.
 */
template <class ST = Something>
class SomethingWeakRef {
	/**
	 * A unique identifier for Something that is valid across all peers.
	 */
	boost::uuids::uuid someId;
	/**
	 * A weak pointer to a Something.
	 */
	weak_ptr<ST> wp;
public:
	/**
	 * Makes an uninitalized reference to nothing.
	 */
	SomethingWeakRef() = default;
	/**
	 * The default copy constructor.
	 */
	SomethingWeakRef(const SomethingRef &) = default;
	/**
	 * Makes a new reference to Something from a weak pointer.
	 * @pre      The weak pointer can produce a shared pointer.
	 * @param s  The weak pointer to Something.
	 * @throw    err? The provided weak pointer cannot produce a shared pointer.
	 */
	SomethingWeakRef(const weak_ptr<ST> &s) {
		shared_ptr<ST> sp = s.lock();
		if (!sp) {
			throw std::runtime_error();  // FIX THIS!!!!
		}
		someId = sp->uuid();
		wp = s;
	}
	/**
	 * Makes a new reference to Something from a shared pointer.
	 * @pre      The shared pointer points to Something.
	 * @param s  The shared pointer to Something.
	 * @throw    err? The provided shared pointer points to nothing.
	 */
	SomethingWeakRef(const shared_ptr<ST> &s) {
		if (!s) {
			throw std::runtime_error();  // FIX THIS!!!!
		}
		someId = s->uuid();
		wp = s;
	}
	/**
	 * Returns the object's unique identifier.
	 */
	const boost::uuids::uuid &uuid() const {
		return someId;
	}
	/**
	 * Returns the weak pointer to Something.
	 */
	const weak_ptr<ST> weak() const {
		return wp;
	}
	/**
	 * Tells if the weak pointer to Something has expired.
	 */
	bool expired() const noexcept {
		return wp.expired();
	}
	/**
	 * Attempts to obtain a shared pointer to the referenced Something.
	 * @return  A shared pointer to the Something if not expired. Otherwise,
	 *          a shared pointer to nothing.
	 */
	shared_ptr<ST> lock() const noexcept {
		return wp.lock();
	}
	/**
	 * Loses the reference to Something.
	 * @pre   This object is not being used as a key for a sorted container.
	 * @post  The weak pointer will be expired, and the UUID will be zero.
	 */
	void reset() noexcept {
		someId = boost::uuids::nil_uuid();
		wp.reset();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator < (const S &s) const {
		return someId < s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator > (const S &s) const {
		return someId > s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator <= (const S &s) const {
		return someId <= s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator >= (const S &s) const {
		return someId >= s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator == (const S &s) const {
		return someId == s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 */
	template <class S>
	bool operator != (const S &s) const {
		return someId != s.uuid();
	}
};

/**
 * A reference to a Something object. This object holds a shared pointer to
 * the Something object and performs comparisons using the Something's UUID.
 * This allows for containers that hold sets of sorted Something objects
 * without holding the actual object or a simple pointer to it.
 * @tparam  ST   The Something type. It should be Something or a class derived
 *               from Something.
 * @todo    Add templated conversions between compatible SomethingRef
 *          types using different @a ST.
 * @author  Jeff Jackowski
 * @todo    Decide if this should be used.
 */
template <class ST = Something>
class SomethingRef {
	/**
	 * A shared pointer to Something.
	 */
	shared_ptr<ST> sp;
public:
	/**
	 * Makes an uninitalized reference to nothing.
	 */
	SomethingRef() = default;
	/**
	 * The default copy constructor.
	 */
	SomethingRef(const SomethingRef &) = default;
	/**
	 * Makes a new reference to Something from a shared pointer.
	 * @pre      The shared pointer points to Something.
	 * @param s  The shared pointer to Something.
	 * @throw    err? The provided shared pointer points to nothing.
	 */
	SomethingRef(const shared_ptr<ST> &s) {
		if (!s) {
			throw std::runtime_error();  // FIX THIS!!!!
		}
		sp = s;
	}
	/**
	 * Returns the object's unique identifier.
	 * @pre  This object references a Something object rather than nothing.
	 */
	const boost::uuids::uuid &uuid() const {
		return sp->uuid();
	}
	/**
	 * Returns the shared pointer to Something.
	 */
	const shared_ptr<ST> shared() const {
		return sp;
	}
	/**
	 * Returns the pointer to the Something object.
	 */
	ST *get() const {
		return sp.get();
	}
	/**
	 * Tells if the this object references Something or nothing.
	 */
	bool operator bool() const noexcept {
		return sp;
	}
	/**
	 * Loses the reference to Something.
	 * @pre   This object is not being used as a key for a sorted container.
	 * @post  The shared pointer will have no object, and the UUID will be zero.
	 */
	void reset() noexcept {
		someId = boost::uuids::nil_uuid();
		sp.reset();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator < (const S &s) const {
		return sp->uuid() < s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator > (const S &s) const {
		return sp->uuid() > s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator <= (const S &s) const {
		return sp->uuid() <= s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator >= (const S &s) const {
		return sp->uuid() >= s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator == (const S &s) const {
		return sp->uuid() == s.uuid();
	}
	/**
	 * Something objects are compared using their UUID.
	 * @pre  This object references a Something object rather than nothing.
	 */
	template <class S>
	bool operator != (const S &s) const {
		return sp->uuid() != s.uuid();
	}
};

/**  <B>OLD</B>
 * Performs a less than comparison on an object contained within a Boost
 * shared pointer. This varies from the operator defined on the shared pointer.
 * Instead of the result being valid for strict weak ordering but otherwise
 * undefined, the result is the same as the less than operator used on the
 * contained objects. If one or more objects do not exist, the operation
 * compares the address of the objects using NULL for the non-existant object(s).
 */
template<class C>
struct CompareSharedContent {
	bool operator() (const std::shared_ptr<C> &l,
	const std::shared_ptr<C> &r) const {
		// get arguments
		const C *pl = l.get();
		const C *pr = r.get();
		// have left?
		if (pl) {
			// have right, too?
			if (pr) {
				// have both arguments, so do the compare
				return *pl < *pr;
			}
		}
		// have one or less of the arguments; compare by pointer address
		return pl < pr;
	}
};

// this is asking for trouble
typedef std::set< const std::shared_ptr<Something>, CompareSharedContent<Something> >
	SomethingSet;

}

