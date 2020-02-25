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

MenuOutputViewAccess::MenuOutputViewAccess(
	MenuOutputView *mov,
	std::size_t newRange
) : outview(mov) {
	if (outview) {
		viewmenu = outview->menu().get();
		outview->lock(newRange);
		seliter = outview->items.cend();
	} else {
		viewmenu = nullptr;
	}
}

void MenuOutputViewAccess::retire() noexcept {
	if (viewmenu) {
		outview->unlock();
		outview = nullptr;
		viewmenu = nullptr;
	}
}

} } }
