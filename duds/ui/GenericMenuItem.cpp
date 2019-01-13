/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/GenericMenuItem.hpp>

namespace duds { namespace ui {

void GenericMenuItem::chose(MenuView &invokingView, const MenuAccess &access) {
	choseSig(invokingView, access, *this);
}

void GenericMenuItem::select(MenuView &invokingView, const MenuAccess &access) {
	selSig(invokingView, access, *this);
}

void GenericMenuItem::deselect(MenuView &invokingView, const MenuAccess &access) {
	deselSig(invokingView, access, *this);
}

} }
