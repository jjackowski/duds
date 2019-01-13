/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <duds/general/Spinlock.hpp>
#include <list>

namespace duds { namespace ui {

class Menu;
class MenuItem;
class MenuOutputView;
class MenuOutputViewAccess;

/**
 * Keeps track of the selected menu item, and updates it based on user input.
 *
 * User input is provided to the backward(), forward(), jump(), and chose()
 * functions. These functions are called in an asynchronous fashion; no
 * access objects are required, and the view may be in use by another thread.
 * The input is evaluated when a new MenuOutputViewAccess object begins to use
 * the MenuView, and no other MenuOutputViewAccess object is using the view.
 *
 * Updating the view and output views requires the MenuView to have a brief
 * exclusive lock on the menu data. After the update, a shared lock on the
 * menu data is maintained by the MenuOutputView while it is in use. This
 * prevents a MenuView object from being used at the same time by multiple
 * MenuOutputView objects on the same thread. The deadlock can be avoided by
 * having only one MenuOutputViewAccess object on the stack at the same time.
 * Technically, two on the satck will work if they use different MenuView
 * objects even if they use the same menu, but this shouldn't be required to
 * make anything work. 
 *
 * @author  Jeff Jackowski
 */
class MenuView : public std::enable_shared_from_this<MenuView> {
private:
	/**
	 * The parent menu that supplies the MenuItems.
	 */
	std::shared_ptr<Menu> parent;
	/**
	 * The index of the currently selected menu item.
	 */
	std::size_t currSel;
	/**
	 * The index of the next menu item to select.
	 */
	std::size_t nextSel;
	/**
	 * Protects this object's data from inappropriate modification when used in
	 * a multithreaded manner.
	 */
	duds::general::Spinlock block;
	/**
	 * The number of MenuOutputView objects currently using this MenuView.
	 */
	int outvUsers;
	/**
	 * Offset from the next selection.
	 */
	int nextSelOff;
	/**
	 * The menu's update index value when this subview was last rendered.
	 */
	int updateIdx;
	/**
	 * Flag used to queue an input request to chose the selected item.
	 */
	bool choseItem;
	/**
	 * Updates the view's selected and chosen menu item if there are no
	 * MenuOutputView objects currently rendering this view. Also increments
	 * the internal count of subviews currently accessing this
	 * menu view. This is needed to prevent update() from making changes
	 * while the view is in use.
	 */
	void update();
	/**
	 * Decrements the internal count of subviews currently accessing this
	 * menu view.
	 */
	void decUser();
	/**
	 * Find the first menu item that is selectable, starting at and including
	 * @a pos, and advancing toward the end of the menu. If nothing is
	 * selectable in that direction, the first item in the opposite direction.
	 * If nothing at all is selectable, then the first menu item.
	 * @param pos  The postiion of the first menu item to inspect.
	 * @return     The position of a menu item that is selectable, or is the
	 *             first item in the menu.
	 */
	int adv(int pos);
	/**
	 * Find the first menu item that is selectable, starting at and including
	 * @a pos, and advancing toward the start of the menu. If nothing is
	 * selectable in that direction, the first item in the opposite direction.
	 * If nothing at all is selectable, then returns @a pos.
	 * @param pos  The postiion of the first menu item to inspect.
	 * @return     The position of a menu item that is selectable, or @a pos.
	 */
	int retr(int pos);
	/**
	 * Responds to the menu inserting a menu item to a spot other than the
	 * end of the menu.
	 * @pre  The thread has an exclusive lock on the menu data, so update()
	 *       cannot be using member data while this function runs.
	 * @bug  Can run simultaneously with jump(), but cannot lock @a block
	 *       because that would cause a deadlock with update() if a menu item
	 *       responded to MenuItem::chose() by inserting or removing an item.
	 */
	void insertion(std::size_t idx);
	/**
	 * Responds to the menu removing a menu item.
	 * @pre  The thread has an exclusive lock on the menu data, so update()
	 *       cannot be using member data while this function runs.
	 * @bug  Can run simultaneously with jump(), but cannot lock @a block
	 *       because that would cause a deadlock with update() if a menu item
	 *       responded to MenuItem::chose() by inserting or removing an item.
	 */
	void removal(std::size_t idx);
	friend Menu;
	friend MenuOutputView;
public:
	/**
	 * Constructs a new MenuView without attaching it to a Menu. It must be
	 * attached to a Menu prior to use, but this cannot be done within the
	 * constructor because it requires the std::shared_ptr that manages the
	 * view. The attachment is done by attach().
	 * @note  All MenuView objects must be managed by std::shared_ptr.
	 * @sa make()
	 */
	MenuView();
	/**
	 * Attaches the view to a menu so that the view can operate on the menu's
	 * data, and the menu can inform the view of changes to the menu's items.
	 * @pre   The view is not already attached to a menu.
	 * @post  The view is useful.
	 * @param menu  The menu whose items will be viewed.
	 * @throw MenuViewAlreadyAttached  The view was already attached to a menu.
	 * @sa make(const std::shared_ptr<Menu> &)
	 */
	void attach(const std::shared_ptr<Menu> &menu);
	/**
	 * Constructs a new MenuView, attaches it to a Menu, and returns the
	 * std::shared_ptr that manages the view.
	 * @param menu  The menu whose items will be viewed.
	 */
	static std::shared_ptr<MenuView> make(const std::shared_ptr<Menu> &menu) {
		std::shared_ptr<MenuView> mv = std::make_shared<MenuView>();
		mv->attach(menu);
		return mv;
	}
	/**
	 * Returns the Menu used by this view.
	 */
	const std::shared_ptr<Menu> &menu() const {
		return parent;
	}
	/**
	 * Returns the index of the currently selected item.
	 *
	 * This value is not changed until after all MenuOutputViewAccess objects
	 * presently using this view are retired or destructed, and another
	 * MenuOutputViewAccess object using this view is created. As a result, the
	 * index of an item that is not @ref MenuItem::isSelectable() "selectable"
	 * may be returned if the item was changed and a new MenuOutputViewAccess
	 * object has not been created since.
	 */
	int selectedIndex() const {
		return currSel;
	}
	/**
	 * Changes the selection toward the back (last item) of the menu.
	 *
	 * The direction may seem to be the reverse of what is expected because
	 * the front and back is defined by the container holding the MenuItem
	 * objects, either a vector or a list, and the initially selected item is
	 * the first, or the front of the container. To call this function
	 * "advance" would abstract its result from the ordering of the items,
	 * and to call it "forward" would use two definitions of front and back
	 * with the same data structure.
	 *
	 * Wrapping of the selection between the front and back of the
	 * @ref MenuItem::isSelectable() "selectable" items is implemented. The
	 * implementation only wraps at the first and last selectable item. If the
	 * currently selected item is not the first or last selectable item, then
	 * the selection change will stop at the first or last item. This is to
	 * prevent a sudden wrapping of the selection that could seem odd,
	 * confusing, or erroneous to the user.
	 *
	 * The currently selected item is not changed until after all
	 * MenuOutputViewAccess objects presently using this view are retired or
	 * destructed, and another MenuOutputViewAccess object using this view is
	 * created. Calling backward() and forward() multiple times before the
	 * current selection is re-evaluated is allowed, and will change an
	 * internal selection offset (@a nextSelOff) each time.
	 *
	 * Between a call to jump() or chose() and the creation of a new
	 * MenuOutputViewAccess object, this function will have no effect. jump() is
	 * intended to select a specific item, so this behavior will prevent
	 * another item from being selected in the case that backward() or
	 * forward() are called shortly after jump(). The behavior also prevents
	 * chosing an item that may not be the intended item by ignoring selction
	 * changes after chosing an item.
	 *
	 * @param dist  The number of @ref MenuItem::isSelectable() "selectable"
	 *              items to move toward the back of the item list. If the
	 *              result would move past the end of the selectable items,
	 *              the item selected will depend on the currently selected
	 *              item:
	 *              - If the current selected item is the last selectable
	 *                item, then the item selected will be the first selectable
	 *                item.
	 *              - If the current selected item is not the last selectable
	 *                item, then the item selected will be the last selectable
	 *                item.
	 */
	void backward(int dist);
	/**
	 * Changes the selection toward the front (first item) of the menu.
	 *
	 * The direction may seem to be the reverse of what is expected because
	 * the front and back is defined by the container holding the MenuItem
	 * objects, either a vector or a list, and the initially selected item is
	 * the first, or the front of the container. To call this function
	 * "backward" would use two definitions of front and back with the same
	 * data structure.
	 *
	 * Wrapping of the selection between the front and back of the
	 * @ref MenuItem::isSelectable() "selectable" items is implemented. The
	 * implementation only wraps at the first and last selectable item. If the
	 * currently selected item is not the first or last selectable item, then
	 * the selection change will stop at the first or last item. This is to
	 * prevent a sudden wrapping of the selection that could seem odd,
	 * confusing, or erroneous to the user.
	 *
	 * The currently selected item is not changed until after all
	 * MenuOutputViewAccess objects presently using this view are retired or
	 * destructed, and another MenuOutputViewAccess object using this view is
	 * created. Calling backward() and forward() multiple times before the
	 * current selection is re-evaluated is allowed, and will change an
	 * internal selection offset (@a nextSelOff) each time.
	 *
	 * Between a call to jump() or chose() and the creation of a new
	 * MenuOutputViewAccess object, this function will have no effect. jump() is
	 * intended to select a specific item, so this behavior will prevent
	 * another item from being selected in the case that backward() or
	 * forward() are called shortly after jump(). The behavior also prevents
	 * chosing an item that may not be the intended item by ignoring selction
	 * changes after chosing an item.
	 *
	 * @param dist  The number of @ref MenuItem::isSelectable() "selectable"
	 *              items to move toward the front of the item list. If the
	 *              result would move past the start of the selectable items,
	 *              the item selected will depend on the currently selected
	 *              item:
	 *              - If the current selected item is the first selectable
	 *                item, then the item selected will be the last selectable
	 *                item.
	 *              - If the current selected item is not the first selectable
	 *                item, then the item selected will be the first selectable
	 *                item.
	 */
	void forward(int dist);
	/**
	 * Jump to a particular option by position index.
	 *
	 * The currently selected item is not changed until after all
	 * MenuOutputViewAccess objects presently using this view are retired or
	 * destructed, and another MenuOutputViewAccess object using this view is
	 * created.
	 *
	 * Between a call to this function and the creation of a new
	 * MenuOutputViewAccess object, calls to backward() and forward() will have no
	 * effect. jump() is intended to select a specific item, so this behavior
	 * will prevent another item from being selected in the case that backward()
	 * or forward() are called shortly after jump(). However, jump() may be
	 * called multiple times, but only the last call before calling chose() or
	 * creating a new MenuOutputViewAccess object will affect the selected item.
	 *
	 * @param pos  The position index to select, but not chose. If the option
	 *             is not @ref MenuItem::isSelectable() "selectable", then the
	 *             next option backward (toward last item),
	 *             wrapping to the begining of the menu if needed, that is
	 *             visible and enabled will be selected.
	 */
	void jump(int pos);
	/**
	 * Queues a request to chose what will be the currently selected menu item
	 * during input processing when a MenuOutputViewAccess object is created.
	 * Input changing the selection will be processed first, regardless of the
	 * order of function calls to this object. In order to limit any problems
	 * this might cause, changes to a selection are ignored after chose() is
	 * called until after the next MenuOutputViewAccess has started the next menu
	 * rendering cycle.
	 * @note  If the ordering of keypresses is not known, call chose() after
	 *        any calls to forward(), backward(), or jump(). If the ordering
	 *        is known, call the functions in the same order.
	 * @post  Calls to forward(), backward(), and jump() will do nothing until
	 *        another MenuOutputViewAccess has started the next menu rendering
	 *        cycle.
	 */
	void chose();
};

/**
 * A shared pointer to a MenuView.
 */
typedef std::shared_ptr<MenuView>  MenuViewSptr;

/**
 * A weak pointer to a MenuView.
 */
typedef std::weak_ptr<MenuView>  MenuViewWptr;

} }

#endif        //  #ifndef MENUVIEW_HPP
