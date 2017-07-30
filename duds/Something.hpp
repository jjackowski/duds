/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef SOMETHING_HPP
#define SOMETHING_HPP

#include <boost/uuid/uuid.hpp>
#include <duds/general/LanguageTaggedString.hpp>
#include <memory>

namespace duds {

/**
 * Something specific; a base class for identifying things.
 * @note  All instances of Something should have their memory handled by a
 *        std::shared_prt<Something> object.
 * @author  Jeff Jackowski
 */
class Something : public std::enable_shared_from_this<Something> {
	/**
	 * A unique identifier that is valid across all peers.
	 */
	boost::uuids::uuid someId;
	/**
	 * A name for this item intended for user presentation.
	 */
	general::LanguageTaggedStringMap ltnames;
protected:
	/**
	 * Simple constructor.
	 * @post  The object's UUID, @a someId, is uninitialized.
	 */
	Something() = default;
	/**
	 * Creates Something with the given UUID.
	 * @param id  The initial UUID to identify this object.
	 */
	Something(const boost::uuids::uuid &id) : someId(id) { }
	/**
	 * Sets the UUID that is associated with this object.
	 * @pre  The object is still being prepared for use. The UUID should not
	 *       changed once the object is in use.
	 * @param id  The new UUID.
	 */
	void setUuid(const boost::uuids::uuid &id) {
		someId = id;
	}
public:
	virtual ~Something() = 0;
	/**
	 * Returns the object's unique identifier.
	 */
	const boost::uuids::uuid &uuid() const {
		return someId;
	}
	/**
	 * Returns the set of names for all locales and languages.
	 */
	const general::LanguageTaggedStringMap &names() const {
		return ltnames;
	}
	/**
	 * Returns the name for the current locale.
	 * @todo  Implement properly.
	 */
	std::string getName() const;
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator < (const Something &s) const {
		return someId < s.someId;
	}
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator > (const Something &s) const {
		return someId > s.someId;
	}
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator <= (const Something &s) const {
		return someId <= s.someId;
	}
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator >= (const Something &s) const {
		return someId >= s.someId;
	}
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator == (const Something &s) const {
		return someId == s.someId;
	}
	/**
	 * Something objects are compared using the @a someId member.
	 */
	bool operator != (const Something &s) const {
		return someId != s.someId;
	}
};

typedef std::shared_ptr<Something>  SomethingSptr;
typedef std::weak_ptr<Something>  SomethingWptr;

}

#endif        //  #ifndef SOMETHING_HPP
