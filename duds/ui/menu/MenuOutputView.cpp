/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/MenuOutputView.hpp>
#include <duds/ui/menu/MenuItem.hpp>

namespace duds { namespace ui { namespace menu {

MenuOutputView::MenuOutputView(const std::shared_ptr<MenuView> &view, int vis) :
Page(view->menu()->title()), mview(view), range(vis), selected(0),
updateIdx(-1) { }

void MenuOutputView::lock(std::size_t newRange) {
	// potentially update the menu; requires exclusive menu lock for
	// an actual update, so must be done prior to getting a shared lock
	mview->update();
	// get a shared lock on the menu to allow multiple threads to render the
	// menu simultaneously
	menu()->block.lock_shared();
	// different range?
	if ((newRange != -1) && (newRange != range)) {
		// change the range
		range = newRange;
		// ensure the visble items are updated
		updateIdx = -1;
	}
	// figure out which menu items should be shown to the user
	updateVisible();
}

void MenuOutputView::unlock() {
	mview->decUser();
	mview->menu()->block.unlock_shared();
}

void MenuOutputView::maxVisible(std::size_t newRange) {
	// something different?
	if (newRange != range) {
		// change the range
		range = newRange;
		// ensure the visble items are updated
		updateIdx = -1;
		updateVisible();
	}
}

bool MenuOutputView::fore(Menu::ItemVec::const_iterator &iter) {
	if (iter != menu()->cbegin()) {
		--iter;
		while ((iter != menu()->cbegin()) && (*iter)->isInvisible()) {
			--iter;
		}
		if (!(*iter)->isInvisible()) {
			return true;
		}
	}
	return false;
}

bool MenuOutputView::revr(Menu::ItemVec::const_iterator &iter) {
	if (iter != menu()->cend()) {
		++iter;
		while ((iter != menu()->cend()) && (*iter)->isInvisible()) {
			++iter;
		}
		if ((iter != menu()->cend()) && !(*iter)->isInvisible()) {
			return true;
		}
	}
	return false;
}

void MenuOutputView::updateVisible() {
	std::size_t sel = mview->selectedIndex();
	int uidx = menu()->updateIndex();
	if ((updateIdx != uidx) || (sel != selected)) {
		// figure out what menu items will be displayed
		items.clear();
		Menu::ItemVec::const_iterator front = menu()->iterator(sel);
		Menu::ItemVec::const_iterator back = front;
		// capture the selected item
		if ((*front)->isVisible()) {
			// start the selected item at the begining of visible items
			items.push_front(front->get());
			// capture iterator to selected item
			seliter = items.cbegin();
			// reset the selected item visible index; the selected item is now
			// at the start of visible items
			selectedVis = 0;
		} else {
			// must only happen with an empty menu
			assert(menu()->empty());
			// ensure valid iterator
			seliter = items.cend();
		}
		// start with an item before if selection moved towards front
		if ((sel < selected) && fore(front)) {
			items.push_front(front->get());
			// selected item moved towards end of visible items
			++selectedVis;
		}
		// continue to add items
		bool done = false;
		while (!done && (items.size() < range)) {
			// item behind
			if (revr(back)) {
				items.push_back(back->get());
			} else {
				done = true;
				//showLast = true;
			}
			// may be at the target size
			if (items.size() == range) {
				break;
			}
			// item in front
			if (fore(front)) {
				items.push_front(front->get());
				// selected item moved towards end of visible items
				++selectedVis;
				done = false;
			}
		}
		// record update status
		updateIdx = uidx;
		selected = sel;
		vchg = true;
		// discover if first & last visible items of the menu are visible in
		// this output view
		showFirst = !fore(front);
		showLast = !revr(back);
	} else {
		vchg = false;
	}
}

} } }
