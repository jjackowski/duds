/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef PAGE_HPP
#define PAGE_HPP

#include <memory>
#include <string>

namespace duds { namespace ui {

/**
 * Represents the notion of a page that may be visited, and helps allow a way
 * to track the path of pages the user has seen.
 * @note    Pages must be managed by std::shared_ptr.
 * @author  Jeff Jackowski
 */
class Page : public std::enable_shared_from_this<Page> {
	/**
	 * The page's name or title.
	 */
	std::string name;
protected:
	/**
	 * Changes the name of the page.
	 * @param t  The new name or title of the page.
	 */
	void title(const std::string &t) {
		name = t;
	}
public:
	/**
	 * Constructs a page with no name.
	 */
	Page() = default;
	/**
	 * Constructs a page with the given name.
	 * @param t  The new name or title of the page.
	 */
	Page(const std::string &t) : name(t) { }
	/**
	 * Returns the name or title of the page.
	 */
	const std::string &title() const {
		return name;
	}
};

/**
 * A shared pointer to a Page.
 */
typedef std::shared_ptr<Page>  PageSptr;

/**
 * A weak pointer to a Page.
 */
typedef std::weak_ptr<Page>  PageWptr;

} }

#endif        //  #ifndef PAGE_HPP
