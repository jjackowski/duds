/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <memory>

namespace duds { namespace ui {

/**
 * Represents the notion of a page that may be visited, and helps allow a way
 * to track the path of pages the user has seen.
 * @todo  Implement.
 * @author  Jeff Jackowski
 */
class Page : public std::enable_shared_from_this<Page> {
	std::string name;
protected:
	void title(const std::string &t) {
		name = t;
	}
public:
	Page() = default;
	Page(const std::string &title) : name(title) { }
	const std::string title() const {
		return name;
	}
	// no render function here; When object is constructed, it must be known
	// what kind of rendering is done, and when invoked, will have
	// render-specific data. The rendering procedure is not abstracted here,
	// just the concept of a page.
};

/*
class TextPage : public Page {
public:
	virtual void render(duds::hardware::display::TextDisplay &) = 0;
};
*/

} }
