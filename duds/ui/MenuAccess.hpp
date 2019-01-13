/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef MENUACCESS_HPP
#define MENUACCESS_HPP

#include <duds/ui/Menu.hpp>
#include <duds/ui/MenuItem.hpp>

namespace duds { namespace ui {

/**
 * Provides an exclusive lock on a Menu to allow the menu to be changed.
 * The lock is recursive.
 *
 * MenuItem objects can also modify the menu. They will also obtain an
 * exclusive lock. The initial lock has a notable amount of overhead, while
 * changing the recursive count for each recurive lock and unlock has minimal
 * overhead. As a result, modifying many MenuItem objects will go quicker if
 * a MenuAccess object is first made, even if its functions are not used.
 * Using a MenuAccess object this way also ensures that no other thread can
 * modify or render the menu until the modifications are complete.
 *
 * @author  Jeff Jackowski
 */
class MenuAccess {
	/**
	 * The menu to operate upon.
	 */
	Menu *menu;
public:
	/**
	 * Creates a new MenuAccess object to modify the given Menu.
	 * @param m  The menu to access for modifications.
	 */
	MenuAccess(const std::shared_ptr<Menu> &m) : menu(m.get()) {
		if (menu) {
			menu->exclusiveLock();
		}
	}
	/**
	 * Creates a new MenuAccess object to modify the given Menu.
	 * @param m  The menu to access for modifications.
	 */
	MenuAccess(Menu &m) : menu(&m) {
		menu->exclusiveLock();
	}
	/**
	 * Gives up access to the menu.
	 */
	~MenuAccess() {
		retire();
	}
	/**
	 * Gives up access to the menu.
	 */
	void retire() noexcept {
		if (menu) {
			menu->exclusiveUnlock();
		}
	}
	/**
	 * Returns the title of the menu.
	 */
	const std::string &title() const {
		return menu->title();
	}
	/**
	 * Change the title of the menu.
	 * @param newTitle   The new title for the menu.
	 */
	void title(const std::string &newTitle) const {
		menu->title(newTitle);
	}
	/**
	 * Returns the number of items in the menu.
	 */
	std::size_t size() const {
		return menu->size();
	}
	/**
	 * True if the menu has at least one MenuItem that is a toggle.
	 */
	bool haveToggles() const {
		return menu->haveToggles();
	}
	/**
	 * Removes all items from the menu.
	 */
	void clear() const {
		menu->clear();
	}
	/**
	 * Returns the MenuItem object at the given position.
	 * @param index            The position of the menu item to return.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	const MenuItemSptr &item(std::size_t index) const {
		return *(menu->iterator(index));
	}
	/**
	 * Appends a new item to the end of the menu.
	 * @param mi  The menu item to add. It will be moved into an internal vector.
	 * @throw MenuNoItemError  An attempt was made to add nothing.
	 */
	void append(MenuItemSptr &&mi) const {
		menu->append(std::move(mi));
	}
	/**
	 * Appends a new item to the end of the menu.
	 * @param mi  The menu item to add.
	 * @throw MenuNoItemError  An attempt was made to add nothing.
	 */
	void append(const MenuItemSptr &mi) const {
		menu->append(mi);
	}
	/**
	 * Inserts a new item into the menu.
	 * @param index  The location where the item will go.
	 * @param mi     The menu item to insert. It will be moved into an internal
	 *               vector.
	 * @throw MenuNoItemError  An attempt was made to insert nothing.
	 * @throw MenuBoundsError  The insertion location is beyond the bounds
	 *                         of the menu.
	 */
	void insert(std::size_t index, MenuItemSptr &&mi) const {
		menu->insert(index, std::move(mi));
	}
	/**
	 * Inserts a new item into the menu.
	 * @param index  The location where the item will go.
	 * @param mi     The menu item to insert.
	 * @throw MenuNoItemError  An attempt was made to insert nothing.
	 * @throw MenuBoundsError  The insertion location is beyond the bounds
	 *                         of the menu.
	 */
	void insert(std::size_t index, const MenuItemSptr &mi) const {
		menu->insert(index, mi);
	}
	/**
	 * Removes an item from the menu.
	 * @param mi     The menu item to remove.
	 * @throw MenuNoItemError       An attempt was made to remove nothing.
	 * @throw MenuItemDoesNotExist  The MenuItem is not in the menu.
	 */
	void remove(const MenuItemSptr &mi) const {
		menu->remove(mi);
	}
	/**
	 * Removes an item from the menu.
	 * @param index  The position of the menu item to remove.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void remove(std::size_t index) const {
		menu->remove(index);
	}
	/**
	 * Changes the visibility of an item on the menu.
	 * @param index  The index of the menu item to modify.
	 * @param vis    True to make visible, false for invisible. If this is the
	 *               same as the current visible state, the menu's update
	 *               index will not change.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void changeVisibility(std::size_t index, bool vis) const {
		item(index)->changeVisibility(vis);
	}
	/**
	 * Hides an item on the menu from view.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void hide(std::size_t index) const {
		changeVisibility(index, false);
	}
	/**
	 * Shows an item on the menu that was hidden.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void show(std::size_t index) const {
		changeVisibility(index, true);
	}
	/**
	 * Enables or disables an item on the menu.
	 * @param index  The index of the menu item to modify.
	 * @param en     True to enable, false to disable. If this is the same as
	 *               the current enable/disable state, the menu's update
	 *               index will not change.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void changeEnabledState(std::size_t index, bool en) const {
		item(index)->changeEnabledState(en);
	}
	/**
	 * Disables an item on the menu.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void disable(std::size_t index) const {
		changeEnabledState(index, false);
	}
	/**
	 * Enables an item on the menu.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void enable(std::size_t index) const {
		changeEnabledState(index, true);
	}
	/**
	 * Toggles the toggle state of an item on the menu.
	 * @pre          The menu item can be toggled; its
	 *               @ref MenuItem::Toggle "Toggle" flag is set.
	 * @param index  The index of the menu item to modify.
	 * @return       The new toggle state.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	bool toggle(std::size_t index) const {
		return item(index)->toggle();
	}
	/**
	 * Changes the toggle state of an item on the menu to the indicated state.
	 * @pre          The menu item can be toggled; its
	 *               @ref MenuItem::Toggle "Toggle" flag is set.
	 * @param index  The index of the menu item to modify.
	 * @param state  The new toggle state. If this is the same as the current
	 *               state, the menu's update index will not change.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void changeToggle(std::size_t index, bool state) const {
		item(index)->changeToggle(state);
	}
	/**
	 * Clears the toggle state of an item on the menu.
	 * @pre          The menu item can be toggled; its
	 *               @ref MenuItem::Toggle "Toggle" flag is set.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void clearToggle(std::size_t index) const {
		changeToggle(index, false);
	}
	/**
	 * Sets the toggle state of an item on the menu.
	 * @pre          The menu item can be toggled; its
	 *               @ref MenuItem::Toggle "Toggle" flag is set.
	 * @param index  The index of the menu item to modify.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void setToggle(std::size_t index) const {
		changeToggle(index, true);
	}
	/**
	 * Returns a menu item's currently set value. The value is optional, so
	 * an empty string is a normal valid result.
	 * @param index  The index of the menu item to query.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 */
	const std::string &value(std::size_t index) const {
		return item(index)->value();
	}
	/**
	 * Changes a menu item's optional value.
	 * @param index  The index of the menu item to modify.
	 * @param value  The new value to store.
	 * @throw MenuBoundsError       The index is beyond the bounds of this menu.
	 */
	void value(std::size_t index, const std::string &value) {
		item(index)->value(value);
	}
};

} }

#endif        //  #ifndef MENUACCESS_HPP
