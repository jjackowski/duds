/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/MenuOutput.hpp>
#include <duds/ui/menu/MenuItem.hpp>
#include <duds/ui/menu/MenuErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace menu {

MenuOutput::MenuOutput(const std::shared_ptr<MenuView> &view, int vis) :
mview(view), range(vis) { }

void MenuOutput::attach(const std::shared_ptr<MenuView> &view, int vis) {
	// only make changes if being attached to something different
	if (mview != view) {
		mview = view;
		if (vis > 0) {
			range = vis;
		}
		items.clear();
		updateIdx = -1;
	}
}

const std::shared_ptr<Menu> &MenuOutput::menu() const {
	if (!mview) {
		DUDS_THROW_EXCEPTION(MenuOutputNotAttached());
	}
	return mview->menu();
}

void MenuOutput::lock(std::size_t newRange) {
	// mark view as in use; prevents view updates
	mview->incUser();
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

void MenuOutput::unlock() {
	mview->decUser();
	mview->menu()->block.unlock_shared();
}

void MenuOutput::maxVisible(std::size_t newRange) {
	// something different?
	if (newRange != range) {
		// change the range
		range = newRange;
		// ensure the visble items are updated
		updateIdx = -1;
		updateVisible();
	}
}

bool MenuOutput::fore(Menu::ItemVec::const_iterator &iter) {
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

bool MenuOutput::revr(Menu::ItemVec::const_iterator &iter) {
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

void MenuOutput::updateVisible() {
	if (menu()->empty()) {
		vchg = !items.empty();
		items.clear();
		return;
	}
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
			// the selected item is the first and last on the menu for now
			firstIdx = lastIdx = sel;
		} else {
			// must only happen with an empty menu
			assert(menu()->empty());
			// ensure valid iterator
			seliter = items.cend();
			firstIdx = lastIdx = -1;
		}
		// start with an item before if selection moved towards front
		if ((sel < selected) && fore(front)) {
			items.push_front(front->get());
			firstIdx = front - menu()->cbegin();
			// selected item moved towards end of visible items
			++selectedVis;
		}
		// continue to add items
		bool done = false;
		while (!done && (items.size() < range)) {
			// item behind
			if (revr(back)) {
				items.push_back(back->get());
				lastIdx = back - menu()->cbegin();
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
				firstIdx = front - menu()->cbegin();
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
