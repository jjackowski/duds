/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/menu/MenuOutputViewAccess.hpp>

namespace duds { namespace ui { namespace menu {

MenuOutputViewAccess::MenuOutputViewAccess(MenuOutputView *mov) : outview(mov) {
	if (outview) {
		menu = outview->menu().get();
		outview->lock();
		seliter = outview->items.cend();
	} else {
		menu = nullptr;
	}
}
/*
MenuOutputViewAccess::MenuOutputViewAccess(MenuOutputView &sv) :
MenuOutputViewAccess(sv.shared_from_this()) { }
*/

void MenuOutputViewAccess::retire() noexcept {
	if (menu) {
		outview->unlock();
		outview = nullptr;
		menu = nullptr;
	}
}

} } }
