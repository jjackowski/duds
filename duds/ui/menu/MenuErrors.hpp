/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <exception>
#include <memory>

namespace duds { namespace ui { namespace menu {

class Menu;
class MenuItem;

/**
 * The base exception type for all errors about menus.
 */
struct MenuError : virtual std::exception, virtual boost::exception { };

/**
 * A required MenuItem object is missing.
 */
struct MenuNoItemError : MenuError { };

/**
 * An attempt was made to inset a MenuItem beyond the bounds of the menu.
 */
struct MenuBoundsError : MenuError { };

/**
 * The specified MenuItem does not exist in the Menu.
 */
struct MenuItemDoesNotExist : MenuError { };

/**
 * An operation was attempted on an invisible MenuItem that requires a
 * visible item.
 */
struct MenuItemNotVisible : MenuError { };

/**
 * An operation was attempted on a disabled MenuItem that requires a
 * non-disabled item.
 */
struct MenuItemDisabled : MenuError { };

/**
 * A request was made to change the toggle state of a MenuItem that is not
 * a toggle item.
 */
struct MenuItemNotAToggle : MenuError { };

/**
 * A request was made to change the value of a MenuItem that does not have
 * a value.
 */
struct MenuItemLacksValue : MenuError { };

/**
 * MenuView::attach() was called more than once on a MenuView object.
 */
struct MenuViewAlreadyAttached : MenuError { };

/**
 * The index (position) of the MenuItem involved in an error.
 */
typedef boost::error_info<struct Info_MenuItemIndex, std::size_t>
	MenuItemIndex;

// may need to add functions to output names for menus and items to ostream
// for the following types to work well. also include menu size.

/**
 * The MenuItem object that is involved in an error.
 */
typedef boost::error_info< struct Info_MenuItem, std::shared_ptr<const MenuItem> >
	MenuItemObject;

/**
 * The Menu object that is involved in an error.
 */
typedef boost::error_info< struct Info_Menu, std::shared_ptr<const Menu> >
	MenuObject;

} } }
