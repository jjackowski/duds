/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/Menu.hpp>
#include <duds/ui/menu/MenuItem.hpp>
#include <duds/ui/menu/MenuView.hpp>
#include <duds/ui/menu/MenuErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace menu {

Menu::Menu(std::size_t reserve) :
lockCnt(0), updateIdx(0), invis(0), toggles(0) {
	items.reserve(reserve);
}

Menu::Menu(const std::string &title, std::size_t reserve) :
lockCnt(0), updateIdx(0), invis(0), toggles(0), lbl(title) {
	items.reserve(reserve);
}

void Menu::exclusiveLock() {
	if (lockOwner != std::this_thread::get_id()) {
		// obtain lock; may block
		block.lock();
		// assure unlock state is good
		assert(!lockCnt && (lockOwner == std::thread::id()));
		// set data for recursive locking
		lockOwner = std::this_thread::get_id();
	}
	// always count lock for recursive locking
	++lockCnt;
}

void Menu::exclusiveUnlock() {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	// decrement lock count; check for need to unlock
	if (!--lockCnt) {
		// clear thread owner
		lockOwner = std::thread::id();
		// unlock
		block.unlock();
	}
}

Menu::ItemVec::const_iterator Menu::iterator(std::size_t index) const {
	// bounds check
	if (index >= items.size()) {
		DUDS_THROW_EXCEPTION(MenuBoundsError() <<
			MenuObject(shared_from_this()) <<
			MenuItemIndex(index)
		);
	}
	return items.begin() + index;
}

void Menu::clear() noexcept {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	// only change if not empty
	if (!items.empty()) {
		// clear out all items
		items.clear();
		invis = 0;
		toggles = 0;
		// record that a change has occurred
		++updateIdx;
	}
}

void Menu::title(const std::string &newTitle) noexcept {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	// change the title
	lbl = newTitle;
	// record that a change has occurred
	++updateIdx;
}

void Menu::addView(const std::shared_ptr<MenuView> &view) {
	exclusiveLock();
	views[view.get()] = view;
	exclusiveUnlock();
}

void Menu::informViews(
	void (MenuView::* eventFunc)(std::size_t),
	std::size_t idx
) {
	ViewMap::iterator iter = views.begin();
	while (iter != views.end()) {
		// attempt to get a shared pointer to the view
		MenuViewSptr view = iter->second.lock();
		// check for existence
		if (view) {
			// call the given function
			(view.get()->*eventFunc)(idx);
			// advance to the next view
			++iter;
		} else {
			// remove destructed view
			iter = views.erase(iter);
		}
	}
}

void Menu::append(std::shared_ptr<MenuItem> &&mi) {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	if (mi) {
		items.emplace_back(std::move(mi));
		items.back()->parent = this;
		// update invisble count
		if (items.back()->isInvisible()) {
			++invis;
		}
		// update toggle count
		if (items.back()->isToggle()) {
			++toggles;
		}
		++updateIdx;
	} else {
		DUDS_THROW_EXCEPTION(MenuNoItemError() << MenuObject(shared_from_this()));
	}
}

void Menu::append(const std::shared_ptr<MenuItem> &mi) {
	// A copy is always made for storage in the items vector. Make the copy
	// here and forward it for storage.
	std::shared_ptr<MenuItem> item(mi);
	append(std::move(item));
}

void Menu::insert(std::size_t index, std::shared_ptr<MenuItem> &&mi) {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	if (mi) {
		// bounds check
		if (index > items.size()) {
			DUDS_THROW_EXCEPTION(MenuBoundsError() <<
				MenuObject(shared_from_this()) <<
				MenuItemIndex(index)
			);
		}
		ItemVec::iterator iter = items.insert(
			items.begin() + index, std::move(mi)
		);
		(*iter)->parent = this;
		// update invisble count
		if ((*iter)->isInvisible()) {
			++invis;
		}
		// update toggle count
		if ((*iter)->isToggle()) {
			++toggles;
		}
		// note a change has occured
		++updateIdx;
		informViews(&MenuView::insertion, index);
	} else {
		DUDS_THROW_EXCEPTION(MenuNoItemError() << MenuObject(shared_from_this()));
	}
}

void Menu::insert(std::size_t index, const std::shared_ptr<MenuItem> &mi) {
	// A copy is always made for storage in the items vector. Make the copy
	// here and forward it for storage.
	std::shared_ptr<MenuItem> item(mi);
	insert(index, std::move(item));
}

void Menu::remove(const std::shared_ptr<MenuItem> &mi) {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	if (mi) {
		// search
		ItemVec::iterator iter = std::find(items.begin(), items.end(), mi);
		// not there?
		if (iter == items.end()) {
			DUDS_THROW_EXCEPTION(MenuItemDoesNotExist() <<
				MenuObject(shared_from_this()) <<
				MenuItemObject(mi)
			);
		}
		std::size_t idx = iter - items.begin();
		// remove the item
		items.erase(iter);
		// disown the item
		mi->parent = nullptr;
		// note a change has occured
		++updateIdx;
		informViews(&MenuView::removal, idx);
	} else {
		DUDS_THROW_EXCEPTION(MenuNoItemError() << MenuObject(shared_from_this()));
	}
}

void Menu::remove(std::size_t index) {
	// an exclusive lock is required
	assert(lockCnt && (lockOwner == std::this_thread::get_id()));
	// bounds check
	if (index > items.size()) {
		DUDS_THROW_EXCEPTION(MenuBoundsError() <<
			MenuObject(shared_from_this()) <<
			MenuItemIndex(index)
		);
	}
	ItemVec::iterator iter = items.begin() + index;
	// get a weak pointer to the item
	std::weak_ptr<MenuItem> mi(*iter);
	// remove the item
	items.erase(iter);
	// does it still exist?
	std::shared_ptr<MenuItem> smi = mi.lock();
	if (smi) {
		// disown the item
		smi->parent = nullptr;
	}
	// note a change has occured
	++updateIdx;
	informViews(&MenuView::removal, index);
}

} } }
