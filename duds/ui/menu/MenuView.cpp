/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/MenuView.hpp>
#include <duds/ui/menu/MenuAccess.hpp>
#include <duds/ui/menu/MenuErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace menu {

MenuView::MenuView() :
currSel(0), nextSel(0), nextSelOff(0), updateIdx(-1), outvUsers(0),
choseItem(false) { }

void MenuView::attach(const std::shared_ptr<Menu> &menu) {
	// already attached?
	if (parent) {
		// cannot be attached a second time
		DUDS_THROW_EXCEPTION(MenuViewAlreadyAttached());
	}
	parent = menu;
	parent->addView(shared_from_this());
	title(parent->title());
}

int MenuView::adv(int pos) {
	int off = pos;
	// find next selectable item
	Menu::ItemVec::const_iterator mi = parent->iterator(off);
	while ((mi != parent->items.end()) && !(*mi)->isSelectable()) {
		// advanace toward end of menu
		++mi;
		++off;
	}
	// no selectable item?
	if (mi == parent->items.end()) {
		// look in the opposite direction
		off = pos;
		mi = parent->iterator(off);
		while ((mi != parent->items.begin()) && !(*mi)->isSelectable()) {
			// advanace toward start of menu
			--mi;
			--off;
		}
	}
	// mi is now selectable or the start of the menu
	assert((mi == parent->items.begin()) || (*mi)->isSelectable());
	return off;
}

int MenuView::retr(int pos) {
	int off = pos;
	// find next selectable item
	Menu::ItemVec::const_iterator mi = parent->iterator(off);
	while ((mi != parent->items.begin()) && !(*mi)->isSelectable()) {
		// advanace toward start of menu
		--mi;
		--off;
	}
	// no selectable item?
	if (mi == parent->items.begin() && !(*mi)->isSelectable()) {
		// look in the opposite direction
		off = pos;
		mi = parent->iterator(off);
		while ((mi != parent->items.end()) && !(*mi)->isSelectable()) {
			// advanace toward end of menu
			++mi;
			++off;
		}
		// no selectable item?
		if (mi == parent->items.end()) {
			// stay in same bad spot
			return pos;
		}
	}
	// mi is now selectable or the start of the menu
	assert((mi == parent->items.begin()) || (*mi)->isSelectable());
	return off;
}

bool MenuView::update() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	// only proceed if there is no other thread using this menu view
	if (!outvUsers++) {
		// nothing on the menu?
		if (parent->empty()) {
			// cancel any selection change request
			nextSel = currSel = nextSelOff = 0;
			choseItem = false;
			// all done
			--outvUsers;
			return true;
		}
		// check for no need to update the current selection
		int uidx = parent->updateIndex();
		if (
			// menu change
			(uidx == updateIdx) &&
			// item selection change
			!choseItem &&
			!nextSelOff &&
			(nextSel == currSel)
		) {
			// all done
			--outvUsers;
			return true;
		}
		// prepare to update the view; requires exclusive menu lock
		MenuAccess ma(parent);
		// record new update index; may have changed
		updateIdx = parent->updateIndex();
		// the new proposed position starts where indicated, even if the option
		// cannot be selected
		int prop = nextSel;
		// adjust negative positions to be from the end of the menu
		if (prop < 0) {
			prop += parent->size();
			// could have been given a very bad number
			if (prop < 0) {
				prop = 0;
			}
		}
		// adjust too large a position to the start of the menu
		if (prop >= parent->size()) {
			prop -= parent->size();
			// much too far?
			if (prop >= parent->size()) {
				prop = parent->size() - 1;
			}
		}
		// no offset?
		if (!nextSelOff) {
			// item to select was within menu bounds?
			if (prop == nextSel) {
				Menu::ItemVec::const_iterator mi = parent->iterator(prop);
				// not selectable?
				if (!(*mi)->isSelectable()) {
					// no change to selected item
					prop = currSel;
				}
			}
			// use the item if selectable, or find the next selectable item
			prop = adv(prop);
		} else {
			// modify proposed item by the offset
			// advance toward end of menu
			if (nextSelOff > 0) {
				// wrap check
				if ((prop == parent->size() - 1) || (adv(prop + 1) == prop)) {
					// select the first item
					prop = adv(0);
				} else {
					// start at current location
					Menu::ItemVec::const_iterator mi = parent->iterator(prop);
					int off = prop;
					// advance!
					for (; nextSelOff && (mi != parent->items.end()); --nextSelOff) {
						++mi;
						++off;
						// move past and do not count invisible items
						while ((mi != parent->items.end()) && (*mi)->isInvisible()) {
							++mi;
							++off;
						}
					}
					// not at end?
					if (mi != parent->items.end()) {
						// use the found item
						prop = adv(off);
					} else {
						// use the last item
						prop = retr(parent->size() - 1);
					}
				}
			}
			// advance toward start of menu
			else {
				// wrap check
				if ((prop == 0) || (retr(prop - 1) == prop)) {
					// select the last item
					prop = retr(parent->size() - 1);
				} else {
					// start at current location
					Menu::ItemVec::const_iterator mi = parent->iterator(prop);
					int off = prop;
					// advance backwards!
					for (; nextSelOff && off; ++nextSelOff) {
						--mi;
						--off;
						// move past and do not count invisible items
						while (off && (*mi)->isInvisible()) {
							--mi;
							--off;
						}
					}
					// not at start?
					if (off) {
						// use the found item
						prop = retr(off);
					} else {
						// use the first item
						prop = adv(0);
					}
				}
			}
		}
		if (prop != currSel) {
			try {
				parent->iterator(currSel)->get()->deselect(*this, ma);
			} catch (...) { }
			try {
				parent->iterator(prop)->get()->select(*this, ma);
			} catch (...) { }
		}
		// prepare the next selection values to move from the current
		// selection
		nextSel = currSel = prop;
		nextSelOff = 0;
		// chose the item?
		if (choseItem) {
			// do not continue chosing
			choseItem = false;
			// invoke the menu item's chose function
			try {
				parent->iterator(currSel)->get()->chose(*this, ma);
			} catch (...) {
				// still must decrement user count
				--outvUsers;
				throw;
			}
		}
	}
	// updating is delayed if outvUsers is not zero after decrement
	return !--outvUsers;
}

void MenuView::insertion(std::size_t idx) {
	// Really should lock block, but if a menu item attempts to remove or insert
	// another menu item when invoked from update() above, then a lock will
	// deadlock the thread. Trying to lock for a short time will mitigate the
	// problem.
	duds::general::SpinlockYieldingWrapper syw(block);
	duds::general::UniqueYieldingSpinLock lock(syw, std::chrono::milliseconds(4));
	// insertion after or at current selection?
	if (currSel >= idx) {
		// modify to track the same menu item
		++currSel;
	}
	// insertion after or at next selection?
	if (nextSel >= idx) {
		// modify to track the same menu item
		++nextSel;
	}
}

void MenuView::removal(std::size_t idx) {
	// Really should lock block, but if a menu item attempts to remove or insert
	// another menu item when invoked from update() above, then a lock will
	// deadlock the thread. Trying to lock for a short time will mitigate the
	// problem.
	duds::general::SpinlockYieldingWrapper syw(block);
	duds::general::UniqueYieldingSpinLock lock(syw, std::chrono::milliseconds(4));
	// removal after current selection or both at end?
	if (currSel && ((currSel > idx) || (parent->size() == currSel))) {
		// modify to track the same menu item
		--currSel;
	}
	// removal after next selection or both at end?
	if (nextSel && ((nextSel > idx) || (parent->size() == nextSel)))  {
		// modify to track the same menu item
		--nextSel;
	}
}

void MenuView::incUser() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	++outvUsers;
}

void MenuView::decUser() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	--outvUsers;
}

void MenuView::backward(int dist) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	// do not change selection further if an item is to be chosen
	if (!choseItem) {
		nextSelOff += dist;
	}
}

void MenuView::jump(int pos) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	// do not change selection further if an item is to be chosen
	if (!choseItem) {
		// set the next selection to the indicated one
		nextSel = pos;
		// clear out relative selection change
		nextSelOff = 0;
	}
}

void MenuView::chose() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	choseItem = true;
}

bool MenuView::queuedInput() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	return choseItem || nextSelOff || (nextSel != currSel);
}

} } }
