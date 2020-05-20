/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef PATH_HPP
#define PATH_HPP

#include <duds/ui/Page.hpp>
#include <vector>

namespace duds { namespace ui {

/**
 * Stores a list of pages the user has visited in the order of the visits.
 * @note  This is @b not thread-safe. Not sure if that will be a problem.
 * @author  Jeff Jackowski
 */
class Path {
public:
	/**
	 * The type used to store the page path.
	 */
	typedef std::vector<PageSptr>  PageStack;
private:
	/**
	 * The pages in path order.
	 */
	PageStack pages;
	/**
	 * Index of the current page.
	 */
	int spot;
public:
	/**
	 * Constructs a new empty path.
	 */
	Path() : spot(-1) { };
	/**
	 * Constructs a new path with an initial page.
	 * @param first  The first page on the path.
	 */
	Path(const PageSptr &first);
	/**
	 * Pushes a new page after the current page. All pages after the current
	 * page prior to the push are removed.
	 * @param page  The new page.
	 * @post  The new page is now the current page.
	 */
	void push(const PageSptr &page);
	/**
	 * Pushes a new page after the current page. All pages after the current
	 * page prior to the push are removed.
	 * @param page  The new page; its contents will be moved into the internal
	 *              vector.
	 * @post  The new page is now the current page.
	 */
	void push(PageSptr &&page);
	/**
	 * Changes the current page by the given amount. If the amount would go
	 * past the first or last page, the result will be the first or last page
	 * without error.
	 * @param steps  The number of pages to move. A negative value moves
	 *               backwards, towards the first page. Zero does not change
	 *               the page.
	 */
	void move(int steps);
	/**
	 * Changes the current page to the page that was pushed before the current
	 * page. If the current page is the first page in the stack, the current
	 * page will remain unchanged.
	 */
	void back() {
		move(-1);
	}
	/**
	 * Changes the current page to the page that was pushed after the current
	 * page. If the current page is the last page in the stack, the curent
	 * page will remain unchanged.
	 */
	void forward() {
		move(1);
	}
	/**
	 * Clears out the stack of all pages.
	 */
	void clear();
	/**
	 * Returns true if the page stack is empty.
	 */
	bool empty() const {
		return pages.empty();
	}
	/**
	 * Returns the number of pages in the page stack.
	 */
	PageStack::size_type size() const {
		return pages.size();
	}
	/**
	 * Returns the current page.
	 * @pre  The page stack is not empty.
	 * @throw std::out_of_range  There is no current page; the stack is empty.
	 */
	const PageSptr &currentPage() const {
		return pages.at(spot);
	}
	/**
	 * Iterator to the start of the page stack.
	 */
	PageStack::const_iterator begin() const {
		return pages.cbegin();
	}
	/**
	 * Reverse iterator to the end of the page stack.
	 */
	PageStack::const_reverse_iterator rbegin() const {
		return pages.crbegin();
	}
	/**
	 * Iterator to the end of the page stack.
	 */
	PageStack::const_iterator end() const {
		return pages.cend();
	}
	/**
	 * Reverse iterator to the start of the page stack.
	 */
	PageStack::const_reverse_iterator rend() const {
		return pages.crend();
	}
	/**
	 * Iterator to the current page.
	 * @pre  The page stack is not empty.
	 */
	PageStack::const_iterator current() const {
		return pages.cbegin() + spot;
	}
	/**
	 * Reverse iterator to the current page.
	 * @pre  The page stack is not empty.
	 */
	PageStack::const_reverse_iterator rcurrent() const {
		return pages.crend() - spot - 1;
	}
};

} }

#endif        //  #ifndef PATH_HPP
