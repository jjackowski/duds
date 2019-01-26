/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/GenericMenuItem.hpp>
#include <duds/ui/menu/Menu.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace menu {

constexpr MenuItem::Flags MenuItem::Disabled;
constexpr MenuItem::Flags MenuItem::Invisible;
constexpr MenuItem::Flags MenuItem::HasValue;
constexpr MenuItem::Flags MenuItem::Toggle;
constexpr MenuItem::Flags MenuItem::ToggledOn;

void MenuItem::label(const std::string &l) {
	if (parent) {
		parent->exclusiveLock();
		lbl = l;
		++parent->updateIdx;
		parent->exclusiveUnlock();
	} else {
		lbl = l;
	}
}

void MenuItem::description(const std::string &d) {
	if (parent) {
		parent->exclusiveLock();
		descr = d;
		++parent->updateIdx;
		parent->exclusiveUnlock();
	} else {
		descr = d;
	}
}

void MenuItem::value(const std::string &v) {
	if (parent) {
		if (!(flgs & HasValue)) {
			DUDS_THROW_EXCEPTION(MenuItemLacksValue() <<
				MenuObject(parent->shared_from_this()) <<
				MenuItemObject(shared_from_this())
			);
		}
		parent->exclusiveLock();
		val = v;
		++parent->updateIdx;
		parent->exclusiveUnlock();
	} else {
		if (!(flgs & HasValue)) {
			DUDS_THROW_EXCEPTION(MenuItemLacksValue() <<
				MenuItemObject(shared_from_this())
			);
		}
		val = v;
	}
}

void MenuItem::changeEnabledState(bool state) {
	if (parent) {
		parent->exclusiveLock();
	}
	flgs.setTo(Disabled, !state);
	if (parent) {
		++parent->updateIdx;
		parent->exclusiveUnlock();
	}
}

void MenuItem::changeVisibility(bool vis) {
	if (flgs.test(Invisible) != vis) {
		return;
	}
	if (parent) {
		parent->exclusiveLock();
		// update inivisble count on the menu
		if (flgs & Invisible) {
			// there must be at least one invisble item on the menu
			assert(parent->invis);
			--parent->invis;
		} else {
			++parent->invis;
		}
	}
	flgs.setTo(Invisible, !vis);
	if (parent) {
		++parent->updateIdx;
		parent->exclusiveUnlock();
	}
}

bool MenuItem::toggle() {
	if (parent) {
		if (!(flgs & Toggle)) {
			DUDS_THROW_EXCEPTION(MenuItemNotAToggle() <<
				MenuObject(parent->shared_from_this()) <<
				MenuItemObject(shared_from_this())
			);
		}
		parent->exclusiveLock();
	} else if (!(flgs & Toggle)) {
		DUDS_THROW_EXCEPTION(MenuItemNotAToggle() <<
			MenuItemObject(shared_from_this())
		);
	}
	flgs.toggle(ToggledOn);
	if (parent) {
		++parent->updateIdx;
		parent->exclusiveUnlock();
	}
	return flgs & ToggledOn;
}

void MenuItem::changeToggle(bool state) {
	// in requested toggle state already?
	if (flgs.test(ToggledOn) == state) {
		return;
	}
	if (parent) {
		if (!(flgs & Toggle)) {
			DUDS_THROW_EXCEPTION(MenuItemNotAToggle() <<
				MenuObject(parent->shared_from_this()) <<
				MenuItemObject(shared_from_this())
			);
		}
		parent->exclusiveLock();
	} else if (!(flgs & Toggle)) {
		DUDS_THROW_EXCEPTION(MenuItemNotAToggle() <<
			MenuItemObject(shared_from_this())
		);
	}
	flgs.setTo(ToggledOn, state);
	if (parent) {
		++parent->updateIdx;
		parent->exclusiveUnlock();
	}
}

void MenuItem::remove() {
	if (parent) {
		// parent will be changed to nullptr during this function
		Menu *p = parent;
		p->exclusiveLock();
		p->remove(shared_from_this());
		p->exclusiveUnlock();
	}
}

void MenuItem::select(MenuView &invokingView, const MenuAccess &access) { }

void MenuItem::deselect(MenuView &invokingView, const MenuAccess &access) { }

} } }
