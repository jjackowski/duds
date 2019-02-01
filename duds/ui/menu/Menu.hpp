/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef MENU_HPP
#define MENU_HPP

#include <duds/ui/menu/MenuErrors.hpp>
#include <boost/noncopyable.hpp>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <map>

namespace duds { namespace ui {

/**
 * Holds the generalized menu infrastructure code.
 */
namespace menu {

class MenuAccess;
class MenuItem;
class MenuOutputView;
class MenuOutputViewAccess;
class MenuView;

/**
 * Stores the data that defines a menu and provides thread-safe access to
 * that data. Modifying the menu is done through a MenuAccess object which
 * obtains an exclusive and recursive lock on the menu's data.
 *
 * See the @ref DUDSmenuArchMenu page for how this relates to the rest of the
 * menu system.
 *
 * @author  Jeff Jackowski
 */
class Menu : public std::enable_shared_from_this<Menu>, boost::noncopyable {
public:
	/**
	 * The type that holds the menu's items.
	 */
	typedef std::vector< std::shared_ptr<MenuItem> >  ItemVec;
	/**
	 * Container type used to track views of the menu.
	 */
	typedef std::map< MenuView*, std::weak_ptr<MenuView> >  ViewMap;
private:
	/**
	 * The store of menu items for the menu.
	 */
	ItemVec items;
	/**
	 * The views; used to inform the views that menu items have been added or
	 * removed.
	 */
	ViewMap views;
	/**
	 * Used to enable thread-safe operations.
	 */
	std::shared_timed_mutex block;
	/**
	 * Used with @a block to implement recursive locking.
	 */
	std::thread::id lockOwner;
	/**
	 * The menu's name; optional.
	 */
	std::string lbl;
	/**
	 * The number of items that are currently flagged as invisible. Used to
	 * assist working out the number of items to show in a menu view.
	 */
	std::size_t invis;
	/**
	 * The number of items that are toggles.
	 */
	std::size_t toggles;
	/**
	 * The number of exclusive locks held by the thread indicated by
	 * @a lockOwner. Used to implement recusrive locking.
	 */
	int lockCnt;
	/**
	 * This value is incremented every time the menu is changed. It is used to
	 * tell when a MenuOutputView needs to be re-rendered to show an up-to-date
	 * menu. This value should only be changed by a thread with an exclusive
	 * lock on @a block.
	 */
	int updateIdx;
	friend MenuAccess;
	friend MenuItem;
	friend MenuOutputView;
	friend MenuOutputViewAccess;
	friend MenuView;
public:
	/**
	 * Construct a new menu.
	 * @note  The menu object must be managed by a std::shared_ptr.
	 * @param reserve  The size to reserve in the vector of menu items.
	 * @sa make(std::size_t)
	 */
	Menu(std::size_t reserve = 0);
	/**
	 * Construct a new menu.
	 * @note  The menu object must be managed by a std::shared_ptr.
	 * @param title    The optional name of the menu.
	 * @param reserve  The size to reserve in the vector of menu items.
	 * @sa make(const std::string &title, std::size_t)
	 */
	Menu(const std::string &title, std::size_t reserve = 0);
	/**
	 * Makes a new menu that is managed by a std::shared_ptr.
	 * @param reserve  The size to reserve in the vector of menu items.
	 * @return         A shared pointer with the new menu.
	 */
	static std::shared_ptr<Menu> make(std::size_t reserve = 0) {
		return std::make_shared<Menu>(reserve);
	}
	/**
	 * Makes a new menu that is managed by a std::shared_ptr.
	 * @param title    The optional name of the menu.
	 * @param reserve  The size to reserve in the vector of menu items.
	 * @return         A shared pointer with the new menu.
	 */
	static std::shared_ptr<Menu> make(
		const std::string &title,
		std::size_t reserve = 0
	) {
		return std::make_shared<Menu>(title, reserve);
	}
	/**
	 * Returns the title of the menu. Since the title is optional, it may be
	 * an empty string.
	 */
	const std::string &title() const {
		return lbl;
	}
	/**
	 * Returns the number of MenuItem objects in the menu. This includes items
	 * that are not visible or disabled.
	 */
	std::size_t size() const {
		return items.size();
	}
	/**
	 * True if the menu has no items.
	 */
	bool empty() const {
		return items.empty();
	}
	/**
	 * Returns a number that is incremented every time the menu is changed.
	 */
	int updateIndex() const {
		return updateIdx;
	}
	/**
	 * True if the menu has at least one MenuItem that is a toggle.
	 */
	bool haveToggles() const {
		return toggles > 0;
	}
private:
	/**
	 * Performs a recursive exclusive lock on @a block.
	 */
	void exclusiveLock();
	/**
	 * Performs a recursive exclusive unlock on @a block.
	 */
	void exclusiveUnlock();
	/**
	 * Stores a weak reference to the given view for the purpose of informing
	 * the view of any item insertions and deletions.
	 * @param view  The view to store.
	 */
	void addView(const std::shared_ptr<MenuView> &view);
	/**
	 * Calls a function on each view using this menu, and provides the index
	 * of a menu item. This is used to tell the views when an item is added or
	 * removed. Views that no longer exist will be removed from the internal
	 * data structure @a views while iterating over it.
	 * @pre   An exclusive lock on the menu.
	 * @param eventFunc  The function to call on each view object.
	 * @param idx        The index of the menu item in question.
	 */
	void informViews(void (MenuView::* eventFunc)(std::size_t), std::size_t idx);
	/**
	 * Returns an iterator to the MenuItem object at the given position.
	 * This requires either an exclusive or a shared lock on the menu. The lock
	 * will ensure the return value is good while avoiding the need to create
	 * a new shared_ptr object.
	 * @param index            The position of the menu item to return.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	ItemVec::const_iterator iterator(std::size_t index) const;
	/**
	 * Returns the begin iterator for the contained MenuItem objects.
	 */
	ItemVec::const_iterator cbegin() const {
		return items.cbegin();
	}
	/**
	 * Returns the end iterator for the contained MenuItem objects.
	 */
	ItemVec::const_iterator cend() const {
		return items.cend();
	}

	// the following functions must only be invoked by MenuAccess

	/**
	 * Removes all items from the menu.
	 */
	void clear() noexcept;
	/**
	 * Change the title of the menu.
	 * @param newTitle   The new title for the menu.
	 */
	void title(const std::string &newTitle) noexcept;
	/**
	 * Appends a new item to the end of the menu.
	 * @param mi  The menu item to add. It will be moved into an internal vector.
	 * @throw MenuNoItemError  An attempt was made to add nothing.
	 */
	void append(std::shared_ptr<MenuItem> &&mi);
	/**
	 * Appends a new item to the end of the menu.
	 * @param mi  The menu item to add.
	 * @throw MenuNoItemError  An attempt was made to add nothing.
	 */
	void append(const std::shared_ptr<MenuItem> &mi);
	/**
	 * Inserts a new item into the menu.
	 * @param index  The location where the item will go.
	 * @param mi     The menu item to insert. It will be moved into an internal
	 *               vector.
	 * @throw MenuNoItemError  An attempt was made to insert nothing.
	 * @throw MenuBoundsError  The insertion location is beyond the bounds
	 *                         of the menu.
	 */
	void insert(std::size_t index, std::shared_ptr<MenuItem> &&mi);
	/**
	 * Inserts a new item into the menu.
	 * @param index  The location where the item will go.
	 * @param mi     The menu item to insert.
	 * @throw MenuNoItemError  An attempt was made to insert nothing.
	 * @throw MenuBoundsError  The insertion location is beyond the bounds
	 *                         of the menu.
	 */
	void insert(std::size_t index, const std::shared_ptr<MenuItem> &mi);
	/**
	 * Removes an item from the menu.
	 * @param mi     The menu item to remove.
	 * @throw MenuItemDoesNotExist  The MenuItem is not in the menu.
	 */
	void remove(const std::shared_ptr<MenuItem> &mi);
	/**
	 * Removes an item from the menu.
	 * @param index  The position of the menu item to remove.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	void remove(std::size_t index);
};

/**
 * A shared pointer to a Menu.
 */
typedef std::shared_ptr<Menu>  MenuSptr;

/**
 * A weak pointer to a Menu.
 */
typedef std::weak_ptr<Menu>  MenuWptr;

} } }

#endif        //  #ifndef MENU_HPP
