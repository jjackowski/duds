/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/uuid/uuid.hpp>
#include <duds/LanguageTaggedString.hpp>

namespace duds {

/**
 * The identification for something that is unique across all peers.
 * @author  Jeff Jackowski
 */
class Identity {
	/**
	 * A unique identifier that is valid across all peers.
	 */
	boost::uuids::uuid uuid;
	/**
	 * A name for this item intended for user presentation.
	 */
	LanguageTaggedStringSet name;
	protected:
	void setUuid(const boost::uuids::uuid &u);
	LanguageTaggedStringSet &getNames() {
		return name;
	}
	void setNames(const LanguageTaggedStringSet &n);
	public:
	/**
	 * @note  This sets @a uuid to all zeros, an invalid id. This is useful to
	 *        avoid generating UUID when a specific one will be set later.
	 */
	Identity();
	Identity(bool genUuid);
	Identity(const boost::uuids::uuid &u);
	Identity(const LanguageTaggedStringSet &n, bool genUuid = false);
	Identity(const boost::uuids::uuid &u, const LanguageTaggedStringSet &n);
	/**
	 * Returns the object's unique identifier.
	 */
	const boost::uuids::uuid &getUuid() const {
		return uuid;
	}
	/**
	 * Returns the set of names for all locales and languages.
	 */
	const LanguageTaggedStringSet &getNames() const {
		return name;
	}
	/**
	 * Returns the name for the current locale.
	 */
	std::string getName() const;
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator < (const Identity &i) const {
		return uuid < i.uuid;
	}
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator > (const Identity &i) const {
		return uuid > i.uuid;
	}
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator <= (const Identity &i) const {
		return uuid <= i.uuid;
	}
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator >= (const Identity &i) const {
		return uuid >= i.uuid;
	}
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator == (const Identity &i) const {
		return uuid == i.uuid;
	}
	/**
	 * Identity objects are compared using the @a uuid member.
	 */
	bool operator != (const Identity &i) const {
		return uuid != i.uuid;
	}
};

}
