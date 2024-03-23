/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/Path.hpp>

namespace duds { namespace ui {

Path::Path(const PageSptr &first) {
	pages.push_back(first);
	spot = 0;
}

void Path::push(const PageSptr &page) {
	// is the current page not the last page on the stack?
	if (pages.size() > (spot + 1)) {
		// remove pages past the new current page
		pages.resize(spot + 2);
		// place the new page after the current
		pages[++spot] = page;
	} else {
		// add to the end
		pages.emplace_back(page);
		++spot;
	}
}

void Path::push(PageSptr &&page) {
	// is the current page not the last page on the stack?
	if (pages.size() > (spot + 1)) {
		// remove pages past the new current page
		pages.resize(spot + 2);
		// place the new page after the current
		pages[++spot] = std::move(page);
	} else {
		// add to the end
		pages.emplace_back(std::move(page));
		++spot;
	}
}

bool Path::move(int steps) {
	// no change?
	if (!steps) {
		return false;
	}
	int ospot = spot;
	// calcuate the new current page spot
	int nspot = spot + steps;
	// limit it to the range of the stack
	if (steps > 0) {
		spot = std::min((PageStack::size_type)nspot, pages.size() - 1);
	} else {
		spot = std::max(nspot, 0);
	}
	// compare current just set spot with the old one
	return spot != ospot;
}

void Path::clear() {
	pages.clear();
	spot = -1;
}

void Path::clearPastCurrent() {
	pages.resize(spot + 1);
}

} }
