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

namespace duds { namespace ui { namespace menu {

/**
 * Provides access to a MenuOutputView for rendering.
 * Input processing in the MenuView may occur during this object's
 * constructor, which may cause a MenuItem's @ref MenuItem::chose() "chose()"
 * function to be called.
 *
 * This will acquire a shared lock on the associated Menu and MenuView objects
 * that are released when this object is destroyed or retire() is called. It
 * will also get an exclusive lock on the MenuOutputView during the
 * constructor. None of these locks are recursive; a thread must not have
 * multiple MenuOutputViewAccess objects from the same MenuOutputView on the
 * stack at the same time.
 *
 * @author  Jeff Jackowski
 */
class MenuOutputViewAccess {
	/**
	 * The output view being accessed.
	 */
	MenuOutputView *outview;
	/**
	 * The menu used by the view.
	 */
	Menu *viewmenu;
	/**
	 * An iterator to the selected item.
	 */
	MenuVisibleList::const_iterator seliter;
	friend MenuOutputView;
public:
	/**
	 * Creates a new MenuOutputViewAccess object that will provide information
	 * on the visible items from the given output view.
	 * @param mov       The output view to use.
	 * @param newRange  The number of menu items that can be displayed, or -1
	 *                  to not change the range.
	 */
	MenuOutputViewAccess(MenuOutputView *mov, std::size_t newRange = -1);
	/**
	 * Creates a new MenuOutputViewAccess object that will provide information
	 * on the visible items from the given output view.
	 * @param mov       The output view to use.
	 * @param newRange  The number of menu items that can be displayed, or -1
	 *                  to not change the range.
	 */
	MenuOutputViewAccess(MenuOutputView &mov, std::size_t newRange = -1) :
		MenuOutputViewAccess(&mov, newRange) { }
	/**
	 * Creates a new MenuOutputViewAccess object that will provide information
	 * on the visible items from the given output view.
	 * @param mov       The output view to use.
	 * @param newRange  The number of menu items that can be displayed, or -1
	 *                  to not change the range.
	 */
	MenuOutputViewAccess(
		const std::shared_ptr<MenuOutputView> &mov,
		std::size_t newRange = -1
	) : MenuOutputViewAccess(mov.get(), newRange) { }
	/**
	 * Relinquishes access to the outview's data.
	 */
	~MenuOutputViewAccess() {
		retire();
	}
	/**
	 * Relinquishes access to the outview's data.
	 */
	void retire() noexcept;
	/**
	 * Returns the Menu in use.
	 */
	const Menu *menu() const {
		return viewmenu;
	}
	/**
	 * Returns the MenuOutputView being accessed.
	 */
	const MenuOutputView *outputView() const {
		return outview;
	}
	/**
	 * Returns the currently set maximum number of visible menu items.
	 */
	std::size_t maxVisible() const {
		return outview->range;
	}
	/**
	 * Changes the maximum number of visible menu items and causes the visible
	 * list to be regenerated. Since the visible list is generated as part of
	 * constructing this object, it is best to tell the constructor of any
	 * change in the number of visible items. If the number of visible items
	 * does not change, the list will not be regenerated.
	 * @warning  Any visible list iterators obtained from other member
	 *           functions must be considered invalid immediately following
	 *           a call to this function.
	 * @param newRange   The number of menu items that can be displayed.
	 */
	void maxVisible(std::size_t newRange) const {
		outview->maxVisible(newRange);
	}
	/**
	 * True if the view has changed since the last access.
	 */
	bool changed() const {
		return outview->changed();
	}
	/**
	 * Returns the MenuItem object at the given position on the menu, not the
	 * position of visible items. Items that are not visible may be requested.
	 * @param index            The position of the menu item to return.
	 * @throw MenuBoundsError  The index is beyond the bounds of this menu.
	 */
	const std::shared_ptr<MenuItem> &item(std::size_t index) const {
		return *(viewmenu->iterator(index));
	}
	/**
	 * True if the menu has at least one MenuItem that is a toggle, even if
	 * none of the toggles are visible. Room should be left on the rendered
	 * menu for toggles if the menu has toggles.
	 */
	bool haveToggles() const {
		return viewmenu->haveToggles();
	}
	/**
	 * Returns the number of visible menu items. This may be smaller than the
	 * maximum number of items that can be displayed.
	 */
	std::size_t size() const {
		return outview->items.size();
	}
	/**
	 * Returns true if there are no visible menu items. This may be true even
	 * when the menu has items.
	 */
	bool empty() const {
		return outview->items.empty();
	}
	/**
	 * Returns an iterator to the start of the visible menu items.
	 * @warning  The iterator must be considered invalid after this access
	 *           object has been retired or destructed. Further use of the
	 *           iterator may appear to work, but will introduce race
	 *           conditions that may cause spurious failures.
	 */
	MenuVisibleList::const_iterator begin() const {
		return outview->items.cbegin();
	}
	/**
	 * Returns an iterator to the end of the visible menu items. As with
	 * regular C++ containers, this iterator cannot be dereferenced.
	 * @warning  The iterator must be considered invalid after this access
	 *           object has been retired or destructed. Further use of the
	 *           iterator may appear to work, but will introduce race
	 *           conditions that may cause spurious failures.
	 */
	MenuVisibleList::const_iterator end() const {
		return outview->items.cend();
	}
	/**
	 * Returns an iterator to the selected menu item. The iterator may be the
	 * same as end() if there is no selected item, but this should only be
	 * true for empty menus.
	 * @warning  The iterator must be considered invalid after this access
	 *           object has been retired or destructed. Further use of the
	 *           iterator may appear to work, but will introduce race
	 *           conditions that may cause spurious failures.
	 */
	const MenuVisibleList::const_iterator &selectedIter() const {
		return outview->seliter;
	}
	/**
	 * Returns the index of the currently selected MenuItem from Menu's
	 * container. This is @b not the position within the visible items; it
	 * is the position within all items for the menu. If the menu is empty,
	 * this value should not be used.
	 */
	std::size_t selected() const {
		return outview->selected;
	}
	/**
	 * Returns the index of the currently selected MenuItem within the list of
	 * currently visible items. This must only be used by rendering code; menu
	 * operations requiring the selected index must use selected() instead.
	 * If the menu has no visible items (empty() is true), this value should
	 * not be used.
	 */
	std::size_t selectedVisible() const {
		return outview->selectedVis;
	}
	/**
	 * True if the menu's first visible item is one of the currently visible
	 * items.
	 */
	bool showingFirst() const {
		return outview->showFirst;
	}
	/**
	 * True if the menu's last visible item is one of the currently visible
	 * items.
	 */
	bool showingLast() const {
		return outview->showLast;
	}
};

} } }
