/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/Page.hpp>
#include <list>

namespace duds { namespace ui {

/**
 * Stores a list of pages the user has visited in the order of the visits.
 * @todo  Implement.
 * @author  Jeff Jackowski
 */
class Path {
public:
	typedef std::list< std::shared_ptr< Page > >  PageList;
private:
	PageList pages;
	PageList::iterator spot;
public:
	void push(const std::shared_ptr<Page> &page);
	void pop();
	const std::shared_ptr<Page> &currentPage() const;
	PageList::const_iterator begin() const {
		return pages.cbegin();
	}
	PageList::const_iterator end() const {
		return pages.cend();
	}
	PageList::const_iterator current() const {
		return spot;
	}
	// ways to render path string should be elsewhere
};

} }
