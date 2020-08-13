/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/graphics/SingleLayoutPanel.hpp>
#include <duds/ui/graphics/PriorityGridLayout.hpp>
#include <duds/ui/graphics/LayoutErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

GridLayoutConfig &SingleLayoutPanel::panelConfig() {
	if (!pgl) {
		DUDS_THROW_EXCEPTION(PanelNotAddedError());
	}
	return pgl->panelConfig(pri);
}

void SingleLayoutPanel::added(PriorityGridLayout *l, int p) {
	if (pgl) {
		DUDS_THROW_EXCEPTION(PanelAlreadyAddedError() <<
			PanelPriority(p)
		);
	}
	pgl = l;
	pri = p;
}

void SingleLayoutPanel::removing(PriorityGridLayout *l, int p) {
	assert((pgl == l) && (pri == p));
	pgl = nullptr;
	pri = 0;
}

} } }
