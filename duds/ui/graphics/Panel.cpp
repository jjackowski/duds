/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/graphics/Panel.hpp>

namespace duds { namespace ui { namespace graphics {

void Panel::added(PriorityGridLayout *, unsigned int) { }

void Panel::removing(PriorityGridLayout *, unsigned int) { }

const BppImage *EmptyPanel::render(
		ImageLocation &,
		ImageDimensions &,
		PanelMargins &,
		int
) {
	return nullptr;
}

} } }
