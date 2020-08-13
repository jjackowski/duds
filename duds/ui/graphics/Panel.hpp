/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef PANEL_HPP
#define PANEL_HPP

#include <duds/ui/graphics/BppImage.hpp>

namespace duds { namespace ui { namespace graphics {

class PriorityGridLayout;

/**
 * Defines the size of a margin bordering a panel; the size of the margin is
 * included in computations for centering and edge justifications.
 */
struct PanelMargins {
	/**
	 * Left margin size.
	 */
	std::uint16_t l;
	/**
	 * Right margin size.
	 */
	std::uint16_t r;
	/**
	 * Top margin size.
	 */
	std::uint16_t t;
	/**
	 * Bottom margin size.
	 */
	std::uint16_t b;
};

/**
 * Represents something being drawn in a rectangular region defined by a
 * PriorityGridLayout object. A panel may be added to any number of layout
 * objects, and may be added to one layout multiple times. Each layout
 * identifies a panel by a priority value unique for the layout.
 *
 * Panels must be managed by a std::shared_ptr.
 *
 * @author  Jeff Jackowski
 */
class Panel {
public:
	/**
	 * Informs the panel that it is being added to a layout.
	 * The default implementation does nothing.
	 * @param pgl  The layout object that is adding the panel.
	 * @param pri  The priority assigned to the panel. This priority is unique
	 *             for the layout instance, but is not unique between different
	 *             layout instances.
	 * @throw object   Any exception will prevent the panel from being added to
	 *                 the layout. The exception will be rethrown by the layout.
	 */
	virtual void added(PriorityGridLayout *pgl, unsigned int pri);
	/**
	 * Informs the panel that it is being removed from a layout.
	 * The default implementation does nothing.
	 * @param pgl  The layout object that is removing the panel.
	 * @param pri  The priority assigned to the panel that is being removed.
	 * @throw object   Any exception will prevent the panel from being removed
	 *                 from the layout. The exception will be rethrown by the
	 *                 layout.
	 */
	virtual void removing(PriorityGridLayout *pgl, unsigned int pri);
	/**
	 * Returns the image of the rendered panel.
	 * @param offset    The location within the returned image that will be
	 *                  the upper left corner of the visible panel. This is
	 *                  initialized to (0,0).
	 * @param dim       The dimensions of the panel to render. This is
	 *                  initialized to the maximum dimensions alloted to the
	 *                  panel and must not be made larger. It needs to be set
	 *                  to the dimensions of the returned image to show.
	 * @param margin    The size of a clear (0) border around the panel's
	 *                  image. This is initialized to all zeros. The margin
	 *                  must fit within the panel's dimensions, so if a margin
	 *                  is used, @a dim must be made smaller. Any edge
	 *                  justification of the panel, or centering, will include
	 *                  the size of the margin.
	 * @param sizeStep  The index of the panel's size-step that was chosen
	 *                  during layout.
	 * @return          The panel's image, or nullptr to leave the panel clear.
	 *                  The portion of the image shown will be defined by the
	 *                  values in @a offset and @a dim.
	 */
	virtual const BppImage *render(
		ImageLocation &offset,
		ImageDimensions &dim,
		PanelMargins &margin,
		int sizeStep
	) = 0;
};

/**
 * A shared pointer to a Panel.
 */
typedef std::shared_ptr<Panel>  PanelSptr;

/**
 * An empty panel; useful for taking up space according to a layout
 * configuration. All layout configurations must have a Panel object. An
 * instance of EmptyPanel can be used for all empty spots.
 * @note  The PriorityGridLayout will not render anything to the area assigned
 *        to this panel. It will effectively be transparent.
 * @author  Jeff Jackowski
 */
class EmptyPanel : public Panel {
public:
	/**
	 * Just returns nullptr.
	 */
	virtual const BppImage *render(
		ImageLocation &,
		ImageDimensions &,
		PanelMargins &,
		int
	);
};



/*
 * A Panel that provides an image based on the size-step selected by the
 * layout.
 * @todo  Implement.
 * /
class SteppedImagePanel : public Panel {
protected:
	std::vector<ConstBppImageSptr> images;
public:
	virtual const BppImage *render(
		ImageLocation &,
		ImageDimensions &,
		PanelMargins &,
		int sizeStep
	) {
		return images[sizeStep];
	}
};
*/

} } }

#endif        //  #ifndef PANEL_HPP
