/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef MENUITEM_HPP
#define MENUITEM_HPP

#include <duds/general/BitFlags.hpp>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

namespace duds { namespace ui { namespace menu {

class Menu;
class MenuView;
class MenuAccess;

/**
 * Represents an option that a user can chose from a menu of options.
 * An item may only be added to one Menu.
 *
 * This is an abstract class to allow implementations to bind an item to
 * whatever the program needs. This can be used to minimize overhead.
 * GenericMenuItem can be used for many cases, but may be cumbersome in cases
 * where it would be helpful for the item to retain additional arbitrary data
 * beyond what this class holds.
 *
 * Items have the following attributes:
 * - Label: short text presented on the menu as the option.
 * - Description: optional longer text, like a helpful description, that
 *   normally is only shown when the item is selected, if shown at all.
 * - Disabled flag: prevents the item from being selected, but does not make
 *   it invisible.
 * - Invisible flag: hides the item from view.
 * - Toggle: items can have a toggle state. This is enabled with the Toggle
 *   flag.
 * - Value: items can have a string value stored with them. This is intended
 *   as an arbitrary setting for the menu item that should be displayed to
 *   the user along with the menu. This is enabled with the HasValue flag.
 *
 * Functions are provided to modify the item's attributes, but the flags, save
 * for the current toggle state (ToggledOn), cannot be changed after
 * construction. Once the item has been added to a menu, these modifications
 * require an exclusive lock on the owning menu. The modification functions
 * will automatically acquire the lock if needed, and the release it afterwards.
 *
 * If an item is removed from a menu, it may be further modified and added to
 * another menu.
 *
 * @author  Jeff Jackowski
 */
class MenuItem : public std::enable_shared_from_this<MenuItem>, boost::noncopyable {
public:
	/**
	 * A set of option and state flags for menu items.
	 */
	typedef duds::general::BitFlags<struct MenuItemFlags>  Flags;
	/**
	 * Indicates that the item may not be chosen by the user. Items that are
	 * disabled and visible should be rendered in a way that tells the user they
	 * exist but are disabled.
	 */
	static constexpr Flags Disabled = Flags::Bit(0);
	/**
	 * Indicates that the item will not be rendered. This will also prevent the
	 * item from being chosen.
	 */
	static constexpr Flags Invisible = Flags::Bit(1);
	/**
	 * Indicates that the item has a value, or setting, that should be shown
	 * in the menu if possible.
	 */
	static constexpr Flags HasValue = Flags::Bit(2);
	/**
	 * Denotes that the item is a toggle, and that the toggle state should be
	 * visible on the menu.
	 */
	static constexpr Flags Toggle = Flags::Bit(3);
	/**
	 * The toggle state; true when the state is on, when the item is true.
	 * Only valid if @a toggle is also true. This value is not automatically
	 * changed by choosing the item.
	 */
	static constexpr Flags ToggledOn = Flags::Bit(4);
private:
	/**
	 * The text shown to represent the item.
	 */
	std::string lbl;
	/**
	 * Additional text that may be shown to provide users with a better idea
	 * of what the option does.
	 */
	std::string descr;
	/**
	 * An optional string for the current setting of the item.
	 */
	std::string val;
	/**
	 * The owning Menu object.
	 */
	Menu *parent = nullptr;
	/**
	 * The item's option flags.
	 */
	Flags flgs;
	friend Menu;
public:
	/**
	 * Constructs a new MenuItem.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	MenuItem(
		const std::string &label,
		Flags flags = Flags::Zero()
	) : lbl(label), flgs(flags) { }
	/**
	 * Constructs a new MenuItem.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	MenuItem(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : lbl(label), descr(description), flgs(flags) { }
	/**
	 * Constructs a new MenuItem with an associated value.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the value given to this constructor so it is implicit
	 *                     when an item's value string is provided.
	 */
	MenuItem(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : lbl(label), descr(description), val(value), flgs(flags | HasValue) { }
	/**
	 * Copy constructs a new MenuItem. The new item will contain the same data
	 * as the original, except that it is not yet part of any menu.
	 */
	MenuItem(const MenuItem &mi) : lbl(mi.lbl), descr(mi.descr), val(mi.val),
	flgs(mi.flgs) { }
	/**
	 * Returns the label text for this item.
	 */
	const std::string &label() const {
		return lbl;
	}
	/**
	 * Changes the label text for this item.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param l  The new label text.
	 */
	void label(const std::string &l);
	/**
	 * Returns the optional description text for this item.
	 */
	const std::string &description() const {
		return descr;
	}
	/**
	 * Changes the optional description text for this item.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param d  The new description text.
	 */
	void description(const std::string &d);
	/**
	 * Returns the optional value text for the item. If the item is not flagged
	 * to have a value (MenuItem::HasValue), the string will be empty. If the
	 * item is flagged as having a value, an empty string is valid.
	 */
	const std::string &value() const {
		return val;
	}
	/**
	 * Changes the optional value text for the item. The item must be flagged
	 * as having a value (MenuItem::HasValue).
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param v  The new value string.
	 * @throw MenuItemLacksValue   The item is not flagged as having a value.
	 */
	void value(const std::string &v);
	/**
	 * Changes the state of the item to either enabled or disabled.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param state  True to enable the item, or false to disable.
	 */
	void changeEnabledState(bool state);
	/**
	 * Makes the item disabled.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 */
	void disable() {
		changeEnabledState(false);
	}
	/**
	 * Makes the item enabled.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 */
	void enable() {
		changeEnabledState(true);
	}
	/**
	 * Changes the visibility of the item.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param vis  True to make the item visible, or false for invisible.
	 */
	void changeVisibility(bool vis);
	/**
	 * Makes the item invisible.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 */
	void hide() {
		changeVisibility(false);
	}
	/**
	 * Makes the item visible.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 */
	void show() {
		changeVisibility(true);
	}
	/**
	 * Toggles the toggle state of the item and returns the new toggle state.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	bool toggle();
	/**
	 * Changes the toggle state of the item to the indicated state.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @param state  The new toggle state. If this is the same as the current
	 *               state, the menu's update index will not change.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void changeToggle(bool state);
	/**
	 * Clears the toggle state of the item.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void clearToggle() {
		changeToggle(false);
	}
	/**
	 * Sets the toggle state of the item.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 * @throw MenuItemNotAToggle    The MenuItem is not a toggle.
	 */
	void setToggle() {
		changeToggle(true);
	}
	/**
	 * Returns the menu object that owns this item.
	 */
	Menu *menu() const {
		return parent;
	}
	/**
	 * Returns the option flags for the item.
	 */
	Flags flags() const {
		return flgs;
	}
	/**
	 * True if the item is flagged as disabled.
	 */
	bool isDisabled() const {
		return flgs & Disabled;
	}
	/**
	 * True if the item is flagged as enabled.
	 */
	bool isEnabled() const {
		return !(flgs & Disabled);
	}
	/**
	 * True if the item is flagged as invisible.
	 */
	bool isInvisible() const {
		return flgs & Invisible;
	}
	/**
	 * True if the item is flagged as visible.
	 */
	bool isVisible() const {
		return !(flgs & Invisible);
	}
	/**
	 * True if the item is flagged as having a value.
	 */
	bool hasValue() const {
		return flgs & HasValue;
	}
	/**
	 * True if the item is flagged as being a toggle.
	 */
	bool isToggle() const {
		return flgs & Toggle;
	}
	/**
	 * True if the item is in the toggled on (true) state. If the item is not
	 * a toggle, the result will be false.
	 */
	bool isToggledOn() const {
		return flgs & ToggledOn;
	}
	/**
	 * Returns true if the item is both visible and enabled.
	 */
	bool isSelectable() const {
		return !(flgs & (Disabled | Invisible));
	}
	/**
	 * Removes the item from its parent menu. If the item has not been added to
	 * a menu, this function has no effect.
	 * @note  If the item has been added to a menu, this operation requires
	 *        an exclusive lock on the parent Menu. The lock is recursive,
	 *        and this function will acquire and release that lock if needed.
	 */
	void remove();
	/**
	 * Invoked when the user has selected, but has not chosen, the item.
	 * The implementation in MenuItem does nothing.
	 */
	virtual void select(MenuView &invokingView, const MenuAccess &access);
	/**
	 * Invoked when the user has selected another item, and before the select()
	 * function for that item is invoked.
	 * The implementation in MenuItem does nothing.
	 * @note   This function will not be called when the item is currently
	 *         selected and then removed from the menu. Issues with locking
	 *         resources prevents a simple solution.
	 */
	virtual void deselect(MenuView &invokingView, const MenuAccess &access);
	/**
	 * Called by MenuView when the user choses this MenuItem. The call occurs
	 * during a MenuView::update() call. This thread will have an exclusive
	 * lock on the menu. If this function throws an exception, the caller of
	 * MenuView::update() can catch that exception.
	 * @param invokingView  The view used to chose this item.
	 * @param access        An access object for the menu that may be used
	 *                      to modify the menu.
	 */
	virtual void chose(MenuView &invokingView, const MenuAccess &access) = 0;
};

/**
 * A shared pointer to a MenuItem.
 */
typedef std::shared_ptr<MenuItem>  MenuItemSptr;

/**
 * A weak pointer to a MenuItem.
 */
typedef std::weak_ptr<MenuItem>  MenuItemWptr;

} } }

#endif        //  #ifndef MENUITEM_HPP
