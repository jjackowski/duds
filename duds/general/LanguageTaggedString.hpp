/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef LANGUAGETAGGEDSTRING_HPP
#define LANGUAGETAGGEDSTRING_HPP

#include <string>
#include <map>
#include <boost/serialization/nvp.hpp>

namespace duds { namespace general {

/**
 * Holds a string and its associated language.
 * @author  Jeff Jackowski
 */
struct LanguageTaggedString {
	/**
	 * The IETF language tag, RFC 5646, maybe. When strings are searched by a
	 * langauge tag, an exact match is preferred, while something close should
	 * be attempted next. Such a process may be complex, may need a set of
	 * pre-made language tags to search for in priority order, or may need
	 * something other than IETF tags to make it easier to find non-exact close
	 * matches.
	 */
	std::string tag;
	/**
	 * A string encoded in UTF-8.
	 */
	std::string string;
	/**
	 * Compares two for sorting.
	 * @todo  Should this use both strings? How will that work with std::multiset?
	 */
	bool operator < (const LanguageTaggedString &lts) const {
		int res = tag.compare(lts.tag);
		if (!res) {
			return string.compare(lts.string) < 0;
		}
		return res < 0;
	}
private:
	friend class boost::serialization::access;
	// needs boost/date_time/posix_time/time_serialize.hpp
	template <class A>
	void serialize(A &a, const unsigned int) {
		a & BOOST_SERIALIZATION_NVP(tag);
		a & BOOST_SERIALIZATION_NVP(string);
	}
};

/*
 * A container that holds LanguageTaggedString objects, and limits a language
 * tag to a single string. The names of items of this type should not be
 * plural or otherwise suggest multiple strings because there is only one
 * string per language and normally only one language will be used at time.
 * @todo  Change this type to something that allows a best match search for a
 *        requested language tag.
 */
//typedef std::set<LanguageTaggedString>  LanguageTaggedStringSet;

// or maybe a Boost multi-index to get the key from LanguageTaggedString
typedef std::map<std::string, std::string>  LanguageTaggedStringMap;

/*
 * A container that holds LanguageTaggedString objects, and can hold several
 * such objects for the same language tag. For a given language tag, each
 * associated sting must be unique.
 * @todo  Explain why or remove.
 */
//typedef std::multiset<LanguageTaggedString> LanguageTaggedStringMultiSet;

} }

#endif        //  #ifndef LANGUAGETAGGEDSTRING_HPP

