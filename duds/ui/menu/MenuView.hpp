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
#include <duds/ui/Page.hpp>
#include <boost/any.hpp>
#include <list>

namespace duds { namespace ui { namespace menu {

class Menu;
class MenuItem;
class MenuOutput;
class MenuOutputAccess;

/**
 * Keeps track of the selected menu item, and updates it based on user input.
 *
 * User input is provided to the backward(), forward(), jump(), and chose()
 * functions. These functions are called in an asynchronous fashion; no
 * access objects are required, and the view may be in use by another thread.
 * The input is evaluated when no other MenuOutputAccess object is using the
 * view and update() is called.
 *
 * Updating the view requires its update() function to have a brief
 * exclusive lock on the menu data. For output, a shared lock on the
 * menu data is maintained by each MenuOutput object while a corresponding
 * MenuOutputAccess object is in use. This prevents altering the menu while
 * it is being output.
 *
 * An optional arbitrary object is stored to assist with writing
 * MenuItem::chose() functions that must deal with being invoked from multiple
 * MenuView objects. It is available through the context() function.
 *
 * @author  Jeff Jackowski
 */
class MenuView : public Page {
private:
	/**
	 * Arbitrary context data that is available to MenuItem::chose().
	 */
	boost::any ctx;
	/**
	 * The parent menu that supplies the MenuItems.
	 */
	std::shared_ptr<Menu> parent;
	/**
	 * Protects this object's data from inappropriate modification when used in
	 * a multithreaded manner.
	 */
	duds::general::Spinlock block;
	/**
	 * The index of the currently selected menu item.
	 */
	int currSel;
	/**
	 * The index of the next menu item to select.
	 */
	int nextSel;
	/**
	 * The number of MenuOutput objects currently using this MenuView.
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
	 * Increments the internal count of subviews currently accessing this
	 * menu view.
	 */
	void incUser();
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
	friend MenuOutput;
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
	 * This value is not changed until after all MenuOutputAccess objects
	 * presently using this view are retired or destructed, and a call to
	 * update() is made. As a result, the index of an item that is not
	 * @ref MenuItem::isSelectable() "selectable" may be returned if the
	 * item was changed and the menu view has not been updated since.
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
	 * MenuOutputAccess objects presently using this view are retired or
	 * destructed, and then a call to update() is made. Calling backward()
	 * and forward() multiple times before the current selection is
	 * re-evaluated is allowed, and will change an internal selection offset
	 * (@a nextSelOff) each time.
	 *
	 * Between a call to jump() or chose() and update(), this function will
	 * have no effect. jump() is intended to select a specific item, so this
	 * behavior will prevent another item from being selected in the case that
	 * backward() or forward() are called shortly after jump(). The behavior
	 * also prevents chosing an item that may not be the intended item by
	 * ignoring selction changes after chosing an item.
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
	void backward(int dist = 1);
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
	 * MenuOutputAccess objects presently using this view are retired or
	 * destructed, and then a call to update() is made. Calling backward()
	 * and forward() multiple times before the current selection is
	 * re-evaluated is allowed, and will change an internal selection offset
	 * (@a nextSelOff) each time.
	 *
	 * Between a call to jump() or chose() and update(), this function will
	 * have no effect. jump() is intended to select a specific item, so this
	 * behavior will prevent another item from being selected in the case that
	 * backward() or forward() are called shortly after jump(). The behavior
	 * also prevents chosing an item that may not be the intended item by
	 * ignoring selction changes after chosing an item.
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
	void forward(int dist = 1) {
		backward(-dist);
	}
	/**
	 * Jump to a particular option by position index. If negative, the size of
	 * the menu will be added so that -1 will jump to the last item, -2 will
	 * jump to the next to last item, and so forth. If the option is not
	 * selectable, no change will occur.
	 *
	 * The currently selected item is not changed until after all
	 * MenuOutputAccess objects presently using this view are retired or
	 * destructed, and update() is called.
	 *
	 * Between a call to this function and update(), calls to backward() and
	 * forward() will have no effect. jump() is intended to select a specific
	 * item, so this behavior will prevent another item from being selected in
	 * the case that backward() or forward() are called shortly after jump().
	 * However, jump() may be called multiple times, but only the last call
	 * before calling chose() or update() will affect the selected item.
	 *
	 * @param pos  The position index to select, but not chose. If the option
	 *             is not @ref MenuItem::isSelectable() "selectable", then the
	 *             current menu item will remain selected. If the current item
	 *             is no longer selectable, then the next option backward
	 *             (toward last item), wrapping to the begining of the menu if
	 *             needed, that is visible and enabled will be selected.
	 */
	void jump(int pos);
	/**
	 * Jumps to the first option in the menu. If the first option is not
	 * selectable, no change will occur.
	 * @sa jump()
	 */
	void jumpToFirst() {
		jump(std::numeric_limits<int>::min());
	}
	/**
	 * Jumps to the last option in the menu. If the last option is not
	 * selectable, no change will occur.
	 * @sa jump()
	 */
	void jumpToLast() {
		jump(-1);
	}
	/**
	 * Queues a request to chose what will be the currently selected menu item
	 * during input processing when a MenuOutputAccess object is created.
	 * Input changing the selection will be processed first, regardless of the
	 * order of function calls to this object. In order to limit any problems
	 * this might cause, changes to a selection are ignored after chose() is
	 * called until after the next MenuOutputAccess has started the next menu
	 * rendering cycle.
	 * @note  If the ordering of keypresses is not known, call chose() after
	 *        any calls to forward(), backward(), or jump(). If the ordering
	 *        is known, call the functions in the same order.
	 * @post  Calls to forward(), backward(), and jump() will do nothing until
	 *        another MenuOutputAccess has started the next menu rendering
	 *        cycle.
	 */
	void chose();
	/**
	 * Returns true if any input for the menu view has been queued and is
	 * awaiting processing. Input is queued by calls to backward(), forward(),
	 * jump(), jumpToFirst(), jumpToLast(), and chose(). It is processed by
	 * calling update().
	 * @note   The menu may need to be output again for reasons other than
	 *         changes from input, such as changes to the underlying menu or
	 *         its items.
	 */
	bool queuedInput();
	/**
	 * Updates the view's selected and chosen menu item if there are no
	 * MenuOutput objects currently rendering this view. This is where the
	 * menu resolves the effects of calls to backward(), forward(), jump(),
	 * and chose(). MenuItem::chose() functions are called here. Not calling
	 * this function will make it appear that the menu is ignoring input.
	 * Resolving the effects of input here allows the program to ensure one
	 * update between multiple render attempts on the same menu view. It also
	 * allows the program to use one MenuOutput object with submenus because
	 * the menu to show may change based on input that is resolved here before
	 * acquiring a MenuOutputAccess object.
	 * @note    This function uses a MenuAccess object internally, so it will
	 *          block until it can get exclusive access to the Menu object.
	 * @return  True if an update could take place, or false if it was delayed
	 *          because another thread was using the view through a
	 *          MenuOutputAccess object. A true value does not mean that an
	 *          update occured because there may not have been a change
	 *          requiring an update.
	 * @throw exception   If an item is chosen, its MenuItem::chose() function
	 *                    is called. If that function throws, the exception will
	 *                    be re-thrown.
	 */
	bool update();
	/**
	 * Returns the arbitrary context object for this view.
	 */
	const boost::any &context() const {
		return ctx;
	}
	/**
	 * Returns the arbitrary context object for this view.
	 */
	boost::any &context() {
		return ctx;
	}
	/**
	 * Helper function that returns a shared pointer to this object from the
	 * base class Page.
	 */
	std::shared_ptr<MenuView> shared_from_this() {
		return std::static_pointer_cast<MenuView>(Page::shared_from_this());
	}
};

/**
 * A shared pointer to a MenuView.
 */
typedef std::shared_ptr<MenuView>  MenuViewSptr;

/**
 * A weak pointer to a MenuView.
 */
typedef std::weak_ptr<MenuView>  MenuViewWptr;

} } }

#endif        //  #ifndef MENUVIEW_HPP
