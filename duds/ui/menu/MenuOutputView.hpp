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
#include <duds/ui/menu/MenuView.hpp>
#include <duds/ui/Page.hpp>

namespace duds { namespace ui { namespace menu {

class MenuOutputViewAccess;

/**
 * Type used by MenuOutputView and MenuOutputViewAccess for the list of visible
 * menu items.
 */
typedef std::list<MenuItem*>  MenuVisibleList;

/**
 * Compiles a list of visible menu items based on the selected item of a
 * MenuView. The visible items are queried from a MenuOutputViewAccess
 * object. MenuOutputViewAccess handles locking and unlocking the required
 * data. This class holds more persistent data, and allows reuse of the
 * visible list when no changes have occurred.
 *
 * Updating a MenuView and output views requires the MenuView to have a brief
 * exclusive lock on the menu data. After the update, a shared lock on the
 * menu data is maintained by the output view while it is in use. This
 * prevents a MenuView object from being used at the same time by multiple
 * MenuOutputView objects on the same thread. The deadlock can be avoided by
 * having only one MenuOutputViewAccess object on the stack at the same time.
 * Technically, two on the satck will work if they use different MenuView
 * objects even if they use the same menu, but this shouldn't be required to
 * make anything work. 
 *
 * @author  Jeff Jackowski
 */
class MenuOutputView : public Page {
private:
	/**
	 * The menu view handling the selected menu item.
	 */
	std::shared_ptr<MenuView> mview;
	/**
	 * The currently visible menu items. This data is only guarenteed to be
	 * valid while a MenuOutputViewAccess is acting upon this MenuOutputView. The
	 * data may or may not remain valid between accesses. The locks caused by
	 * the access object include a shared lock on the menu, which is why shared
	 * pointers are not needed here.
	 */
	MenuVisibleList items;
	/**
	 * Iterator to the currently selected item. It will be the end iterator only
	 * if no item is selected. A view of a menu with at least one item will
	 * have a selected item when a MenuOutputViewAccess object is operating on
	 * the view.
	 */
	MenuVisibleList::const_iterator seliter;
	/**
	 * The maximum number of visible items.
	 */
	std::size_t range;
	/**
	 * Index of the selected item. When a MenuOutputViewAccess is operating upon
	 * this object, it is the currently selected item. Otherwise it is the
	 * previously selected item.
	 */
	std::size_t selected;
	/**
	 * The menu's update index value when this subview was last rendered.
	 */
	int updateIdx;
	/**
	 * True when the view has changed since the last access, and false
	 * otherwise. Changed in updateVisible().
	 */
	bool vchg;
	/**
	 * Handles several tasks to lock and prepare menu data.
	 * -# Calls MenuView::update() to conditionally update the view with any
	 *    changes to the menu. If the view does update with new data, it
	 *    requires an exclusive lock from the Menu. The lock is released by the
	 *    end of the function.
	 * -# Gets a shared lock on the menu data.
	 * -# Produce a list of the MenuItem objects that are visible.
	 */
	void lock();
	/**
	 * Informs the MenuView that it has one fewer MenuOutputView objects acting
	 * upon it, and releases the shared lock on the menu data.
	 */
	void unlock();
	/**
	 * Changes the maximum number of visible menu items and causes the visible
	 * list to be regenerated.
	 * @warning  Any visible list iterators obtained from other member
	 *           functions must be considered invalid immediately following
	 *           a call to this function.
	 * @param newRange   The number of menu items that can be displayed.
	 */
	void maxVisible(std::size_t newRange);
	/**
	 * A helper function for update() that moves @a iter towards the front of
	 * the menu until it finds a visible menu item, and then adds that item
	 * to the list of visible items.
	 * @param iter  The location to start the search.
	 * @return      True if an item was added, or false if there isn't an item
	 *              in the forward direction that is visble.
	 */
	bool fore(Menu::ItemVec::const_iterator &iter);
	/**
	 * A helper function for update() that moves @a iter towards the back of
	 * the menu until it finds a visible menu item, and then adds that item
	 * to the list of visible items.
	 * @param iter  The location to start the search.
	 * @return      True if an item was added, or false if there isn't an item
	 *              in the backward direction that is visble.
	 */
	bool revr(Menu::ItemVec::const_iterator &iter);
	/**
	 * Updates which items should be visible.
	 * If an even number of items are visible, an additional item will be in
	 * the direction of the last change from the selected item so that the user
	 * will see more items ahead of the change.
	 * @pre   @a mview's MenuView::block is locked by the calling thread.
	 * @post  The values in @a first and @a last will be updated, and
	 *        @a updateIdx will match the value from the parent Menu.
	 */
	void updateVisible();
	/**
	 * True if the view has changed.
	 * @pre  A MenuOutputViewAccess object is acting upon this object. This will
	 *       cause the selected item to be updated, and updateVisible() will
	 *       record if a change occurred.
	 */
	bool changed() const {
		return vchg;
	}
	friend MenuOutputViewAccess;
public:
	/**
	 * Constructs a new output view for a given menu that will start with a
	 * specified maximum number of visible items.
	 * @note  MenuOutputView objects must be managed by std::shared_ptr.
	 * @param view  The MenuView that will feed this view.
	 * @param vis   The initial maximum number of visible menu items.
	 * @sa make()
	 */
	MenuOutputView(const std::shared_ptr<MenuView> &view, int vis);
	/**
	 * Makes a new output view for a given menu that will start with a
	 * specified maximum number of visible items, and returns a std::shared_ptr
	 * that will manage its memory.
	 * @param view  The MenuView that will feed this view.
	 * @param vis   The initial maximum number of visible menu items.
	 */
	static std::shared_ptr<MenuOutputView> make(
		const std::shared_ptr<MenuView> &view,
		int vis
	) {
		return std::make_shared<MenuOutputView>(view, vis);
	}
	/**
	 * Returns the MenuView used by this output view.
	 */
	const std::shared_ptr<MenuView> &view() const {
		return mview;
	}
	/**
	 * Returns the MenuV used by this output view.
	 */
	const std::shared_ptr<Menu> &menu() const {
		return mview->menu();
	}
	/**
	 * Helper function that returns a shared pointer to this object from the
	 * base class Page.
	 */
	std::shared_ptr<MenuOutputView> shared_from_this() {
		return std::static_pointer_cast<MenuOutputView>(Page::shared_from_this());
	}
};

/**
 * A shared pointer to a MenuOutputView.
 */
typedef std::shared_ptr<MenuOutputView>  MenuOutputViewSptr;

/**
 * A weak pointer to a MenuOutputView.
 */
typedef std::weak_ptr<MenuOutputView>  MenuOutputViewWptr;

} } }
