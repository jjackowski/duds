/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef PRIORITYGRIDLAYOUT_HPP
#define PRIORITYGRIDLAYOUT_HPP

#include <duds/ui/graphics/GridLayoutConfig.hpp>
#include <duds/ui/graphics/Panel.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * A way to place Panel objects dynamically with a priority mechanism to allow
 * panels to be resized and moved more automatically to support panels changing
 * in importance on a user interface.
 *
 * Panels are added with a priority key. This key is a positive integer where
 * lower values represent a higher priority. The panels are placed in the
 * layout in order of their priority. Lower priority panels might not be given
 * their first choice of location and size, and might not even be placed at
 * all.
 *
 * Each panel's configuration includes a series of
 * @ref GridSizeStep "size-steps" in GridLayoutConfig::sizes. These size-steps
 * include a @ref GridSizeStep::loc "grid location" that places the panel
 * relative to other panels, and its @ref GridSizeStep::minDim "minimum size".
 *
 * The grid location includes a row and column. The layout organizes the panels
 * first by row and then by column. There is no attempt to make the columns
 * between rows line up; the column location only affects where a panel will be
 * with respect to other panels in the same row. All panels in the same row
 * will have available to them the same height. Any unused grid locations do
 * not take up space in the final result.
 *
 * Only one panel will be placed into a given grid location. If a panel has a
 * size-step that requests a location used by a higher priority panel, then
 * the next size-step will be tried. If a size-step requests a minimum size
 * that cannot be fullfilled with the remaining space, the next size-step will
 * be tried. Any size-steps flagged as hidden will be skipped. If no size-step
 * can be used to place a panel, the panel will not be rendered. The index of
 * the size-step selected during layout is passed to Panel::render() during
 * rendering so that the rendering code can use it as a hint for how the panel
 * should be rendered.
 *
 * After panels are added, removed, their configurations changed, or the fill
 * dimensions (renderFill()) are changed, layout() must be called prior to
 * calling render() again. None of these operations are thread-safe.
 *
 * To render, a destination image must be provided. The panel images will be
 * written into the destination. The area of the destination used by the layout
 * is defined by an offset location (renderOffset()) and a fill dimension
 * (renderFill()). Each panel will provide its image when its Panel::render()
 * function is called. If the image does not cover the whole area allocated
 * to the panel, the unused area in the destination image will remain
 * unchanged.
 *
 * @author  Jeff Jackowski
 */
class PriorityGridLayout {
	/**
	 * Internal data structure used to store a Panel, its configuration, and
	 * current layout status.
	 */
	struct PanelStatus {
		/**
		 * The Panel object.
		 */
		PanelSptr panel;
		/**
		 * The panel's layout configuration.
		 */
		GridLayoutConfig config;
		/**
		 * The maximum dimensions allocated to the panel by layout().
		 */
		ImageDimensions dim;
		/**
		 * The location on the target image as determined by layout().
		 */
		ImageLocation loc;
		/**
		 * The index of the size-step to use for rendering the panel as selected
		 * by layout().
		 */
		int sizeStep;
		/**
		 * True to indicate that the panel will not be rendered. This can be
		 * from a hidden flag, or from exhausting all size-steps.
		 */
		bool hidden;
		/**
		 * Returns the size-step selected by layout().
		 * @pre  The panel will be rendered; @a hidden is false.
		 */
		const GridSizeStep &currentStep() const {
			return config.sizes[sizeStep];
		}
		/**
		 * Returns the configuration flags to use for rendering. The flags are
		 * formed by OR'ing the panel config flags and the current size-step
		 * flags.
		 * @pre  The panel will be rendered; @a hidden is false.
		 */
		GridLayoutConfig::Flags flags() const {
			return config.flags | config.sizes[sizeStep].flags;
		}
		PanelStatus() = default;
		PanelStatus(const PanelSptr &pspt, const GridLayoutConfig &conf);
		PanelStatus(const PanelSptr &pspt, GridLayoutConfig &&conf);
	};
	/**
	 * The type that maps panel priorities to panel data.
	 */
	typedef std::map<unsigned int, PanelStatus>  GridConfig;
	/**
	 * The mapping of Panel objects by priority key. The priority must be
	 * positive; zero is used to denote the absence of a panel.
	 */
	GridConfig configs;
	/**
	 * Internal data structure used by layout() to store information on each
	 * row that is only needed to place all the panels.
	 */
	struct RowData {
		/**
		 * Stores a panel priority key and a pointer to the associated
		 * PanelStatus object inside @a configs.
		 */
		typedef std::pair<unsigned int, PanelStatus*>  KeyPanel;
		/**
		 * The panels in column order, left to right. A priority key of zero
		 * indicates an unused column; no space is given to such columns.
		 */
		std::vector<KeyPanel> panels;
		/**
		 * Minimum used area of the panels.
		 */
		ImageDimensions used = ImageDimensions(0, 0);
		/**
		 * Number of panels requesting width expansion.
		 */
		int widthExpand = 0;
		/**
		 * True when a panel requests height expansion.
		 */
		bool heightExpand = false;
		/**
		 * Ensures the minimum number of allocated columns includes the given
		 * index.
		 * @param mc  The minimum column number.
		 * @post      The @a panels vector will have enough allocated space to
		 *            include the index @a mc.
		 * @return    True if no allocation was required, or false if the
		 *            column did not previously exist.
		 */
		bool minCols(int mc) noexcept;
		/**
		*/
		KeyPanel &operator [] (int c);
	};
	/**
	 * Type used inside layout() to store data on all the rows.
	 */
	typedef std::vector<RowData> RowVec;
	/**
	 * Maximum heights for rows.
	 */
	std::vector<std::int16_t> rowMaxHeight;
	/**
	 * The upper right location of the destination image where the topmost row
	 * and leftmost column will be placed.
	 */
	ImageLocation offset = { 0, 0 };
	/**
	 * The area to fill in the destination image.
	 */
	ImageDimensions fill;
	/**
	 * Ensures a minimum number of rows in @a rv, and returns true if at least
	 * that many rows already existed.
	 * @param rv  The vector of rows.
	 * @param ms  The minimum number of rows.
	 * @return    True if the minimum already existed, false if the vector
	 *            @a rv was expanded.
	 */
	static bool minRows(RowVec &rv, int ms);
	/**
	 * Returns the priority key for the panel at the given location, or zero if
	 * there is no panel.
	 * @param rv  The vector of row information.
	 * @param gl  The location to check.
	 */
	static unsigned int panelAt(RowVec &rv, const GridLocation &gl) noexcept;
public:
	/**
	 * Change the upper left corner of the destination image that will receive
	 * the panel images rendered through the layout.
	 * @param off  The new offset.
	 */
	void renderOffset(const ImageLocation &off) {
		offset = off;
	}
	/**
	 * Returns the upper left corner of the destination image that will receive
	 * the panel images rendered through the layout.
	 */
	const ImageLocation &renderOffset() const {
		return offset;
	}
	/**
	 * Changes the area filled by the layout.
	 * @param dim  The dimensions of the layout area.
	 * @post  layout() must be called before the next call to render().
	 */
	void renderFill(const ImageDimensions &dim) {
		fill = dim;
	}
	/**
	 * Returns the area filled by the layout.
	 */
	const ImageDimensions &renderFill() const {
		return fill;
	}
	/**
	 * Sets the maximum height of a row.
	 * @note  The values are stored in a std::vector. To minimize how many times
	 *        the vector is reallocated, start with the row in the bottom
	 *        position.
	 * @param row     The row to change. Zero is the topmost row.
	 * @param height  The maximum height for the row. There is always a maximum
	 *                height, so removing the restriction is done by making the
	 *                restriction huge, like 0x7FFF.
	 * @post  layout() must be called before the next call to render().
	 */
	void maxRowHeight(int row, std::int16_t height);
	/**
	 * Returns the maximum height of a row. There is always a maximum height.
	 * If one has not been specified, the returned value will be 0x7FFF.
	 * @param row     The row to query. Zero is the topmost row.
	 */
	std::int16_t maxRowHeight(int row) const;
	/**
	 * Adds a panel in an unused priority spot, or fail to add if the spot is
	 * already used.
	 * @param panel   The panel to add.
	 * @param config  The layout configuration used to place the panel.
	 * @param pri     The unique priority for the panel. It must be positive.
	 * @return        True if the panel was added. False if there already is a
	 *                panel with priority @a pri.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw LayoutPriorityInvalidError
	 *                The provided priority value is zero.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	bool add(
		const PanelSptr &panel,
		const GridLayoutConfig &config,
		unsigned int pri
	);
	/**
	 * Adds a panel in an unused priority spot, or fail to add if the spot is
	 * already used.
	 * @param panel   The panel to add.
	 * @param config  A size-stpe layout configuration used to place the panel.
	 *                A GridLayoutConfig object will be created based on the
	 *                size-step.
	 * @param pri     The unique priority for the panel. It must be positive.
	 * @return        True if the panel was added. False if there already is a
	 *                panel with priority @a pri.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw LayoutPriorityInvalidError
	 *                The provided priority value is zero.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	bool add(
		const PanelSptr &panel,
		const GridSizeStep &config,
		unsigned int pri
	);
	/**
	 * Adds a panel to the next lowest priority spot.
	 * @param panel   The panel to add.
	 * @param config  The layout configuration used to place the panel.
	 * @return        The unique priority assigned to the panel.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	unsigned int add(const PanelSptr &panel, const GridLayoutConfig &config);
	/**
	 * Adds a panel to the next lowest priority spot.
	 * @param panel   The panel to add.
	 * @param config  A size-stpe layout configuration used to place the panel.
	 *                A GridLayoutConfig object will be created based on the
	 *                size-step.
	 * @return        The unique priority assigned to the panel.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	unsigned int add(const PanelSptr &panel, const GridSizeStep &config);
	/**
	 * Adds a panel or replaces an existing panel at the given priority spot.
	 * @param panel   The panel to add.
	 * @param config  The layout configuration used to place the panel.
	 * @param pri     The unique priority for the panel. It must be positive.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw LayoutPriorityInvalidError
	 *                The provided priority value is zero.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	void addOrReplace(
		const PanelSptr &panel,
		const GridLayoutConfig &config,
		unsigned int pri
	);
	/**
	 * Adds a panel or replaces an existing panel at the given priority spot.
	 * @param panel   The panel to add.
	 * @param config  A size-stpe layout configuration used to place the panel.
	 *                A GridLayoutConfig object will be created based on the
	 *                size-step.
	 * @param pri     The unique priority for the panel. It must be positive.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw LayoutPriorityInvalidError
	 *                The provided priority value is zero.
	 * @throw object  Anything thrown by Panel::added() of @a panel. If this
	 *                occurs, the panel will not be added.
	 */
	void addOrReplace(
		const PanelSptr &panel,
		const GridSizeStep &config,
		unsigned int pri
	);
	/**
	 * Removes the panel in the given priority spot.
	 * @param pri     The priority of the panel to remove.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw object  Anything thrown by Panel::removing() of the panel being
	 *                removed. If this occurs, the panel will not be removed.
	 */
	void remove(unsigned int pri);
	/**
	 * Removes the given panel if it is present. If the panel has been added
	 * multiple times, only the highest priority entry will be removed.
	 * This requires searching for the panel, so this function is slower
	 * than remove(unsigned int).
	 * @param panel   The panel to remove.
	 * @post  layout() must be called before the next call to render() if no
	 *        exception is thrown.
	 * @throw object  Anything thrown by Panel::removing() of the panel being
	 *                removed. If this occurs, the panel will not be removed.
	 */
	void remove(const PanelSptr &panel);
	/**
	 * Returns the layout configuration for the specified panel.
	 * The configuration may be modified, but not in a thread-safe manner. If
	 * modifications are made, layout() must be called prior to rendering
	 * again.
	 * @param pri    The unique priority for the panel.
	 * @return       The panel's layout configuration. Do not modify this while
	 *               layout() or render() are running on this object on another
	 *               thread.
	 * @post         If the configuration is altered, layout() must be called
	 *               before the next call to render().
	 * @throw        PanelNotFoundError  There is no panel at the given priority.
	 */
	GridLayoutConfig &panelConfig(unsigned int pri);
	/**
	 * Returns the layout configuration for the specified panel.
	 * @param pri    The unique priority for the panel.
	 * @throw        PanelNotFoundError  There is no panel at the given priority.
	 */
	const GridLayoutConfig &panelConfig(unsigned int pri) const;
	/**
	 * Places all panels into general positions. After any changes to layout
	 * configurations, this function must be called prior to render().
	 * @return  The number of panels that have been allocated space on the
	 *          grid layout.
	 */
	int layout();
	/**
	 * Renders all visible panels to the provided image. If a panel does not
	 * use all the area allocated to it, the corresponding unused area of
	 * @a dest will remain unchanged.
	 * @param dest  The destination image. The area defined by renderFill(),
	 *              starting at renderOffset(), may be overwritten with images
	 *              from the panels. The rest of the image will not be changed.
	 * @throw ImageBoundsError
	 *              The destination image isn't large enough to hold the layout.
	 *              The layout size is renderFill(), starting at the location
	 *              renderOffset().
	 * @throw PanelImageTooLargeError
	 *              A panel provided an image larger than the dimensions it
	 *              was allocated by the layout. If this is thrown, the
	 *              destination image may be altered by other panels, but not
	 *              all of the panels will be rendered. This could be changed
	 *              by obtaining all panel images first and then rendering them
	 *              if no exceptions are thrown.
	 */
	void render(BppImage *dest);
	/**
	 * @copydoc render(BppImage *)
	 */
	void render(BppImage &dest) {
		render(&dest);
	}
	/**
	 * @copydoc render(BppImage *)
	 */
	void render(const BppImageSptr &dest) {
		render(dest.get());
	}
	/**
	 * Returns the dimensions assigned to the panel at priority @a pri by
	 * layout(), or { 0, 0 }.
	 * @param pri    The unique priority for the panel.
	 */
	ImageDimensions layoutDimensions(unsigned int pri) const;
	/**
	 * Returns the location assigned to the panel at priority @a pri by
	 * layout(), or { 0, 0 }.
	 * @param pri    The unique priority for the panel.
	 */
	ImageLocation layoutLocation(unsigned int pri) const;
	/**
	 * Provides the dimensions and location assigned to the panel at priority
	 * @a pri by layout()
	 * @param dim    The destination for the assigned panel dimensions.
	 * @param loc    The destination for the assigned panel location.
	 * @param pri    The unique priority for the panel.
	 * @return       True if the layout data was found and copied, or false
	 *               and no data was changed.
	 */
	bool layoutPosition(
		ImageDimensions &dim,
		ImageLocation &loc,
		unsigned int pri
	) const;
};

} } }

#endif        //  #ifndef PRIORITYGRIDLAYOUT_HPP
