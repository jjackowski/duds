/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef SINGLELAYOUTPANEL_HPP
#define SINGLELAYOUTPANEL_HPP

#include <duds/ui/graphics/Panel.hpp>
#include <duds/ui/graphics/GridLayoutConfig.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * A variation of a Panel that can only be added to one layout at a time, and
 * that keeps track of its own priority and the layout object. By tracking its
 * layout and priority, the SingleLayoutPanel offers the panelConfig()
 * function that makes it easier to modify its layout configuration.
 * @author  Jeff Jackowski
 */
class SingleLayoutPanel : public Panel {
	/**
	 * The layout object that has added this Panel, or nullptr if not added.
	 */
	PriorityGridLayout *pgl = nullptr;
	/**
	 * The priority value assigned to this Panel when added to a layout.
	 */
	unsigned int pri = 0;
public:
	/**
	 * Returns the layout object that has added this Panel, or nullptr if no
	 * layout has the panel.
	 */
	PriorityGridLayout *owner() const {
		return pgl;
	}
	/**
	 * Returns the priority assigned to this panel when added to a layout, or
	 * zero if not added.
	 */
	int layoutPriority() const {
		return pri;
	}
	/**
	 * Returns the panel's layout configuration.
	 * The configuration may be modified, but not in a thread-safe manner. If
	 * modifications are made, PriorityGridLayout::layout() must be called prior
	 * to rendering again.
	 * @return    The panel's layout configuration. Do not modify this while
	 *            PriorityGridLayout::layout() or PriorityGridLayout::render()
	 *            of owner() are running on another thread.
	 * @throw PanelNotAddedError  The panel has not been added to a layout, or
	 *                            has been removed from a layout, so it has no
	 *                            layout configuration.
	 */
	GridLayoutConfig &panelConfig();
	/**
	 * Records the layout object and priority that has been assigned to this
	 * panel.
	 * @throw PanelAlreadyAddedError  SingleLayoutPanel objects may only be used
	 *                                by one layout at any given time. The
	 *                                exception will prevent the layout from
	 *                                adding this panel.
	 */
	virtual void added(PriorityGridLayout *l, int p);
	/**
	 * Records that the panel has been removed from the layout. After removal,
	 * the panel may be added to a layout.
	 */
	virtual void removing(PriorityGridLayout *l, int p);
};

} } }

#endif        //  #ifndef SINGLELAYOUTPANEL_HPP
