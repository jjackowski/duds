/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef GRIDLAYOUTCONFIG_HPP
#define GRIDLAYOUTCONFIG_HPP

#include <duds/general/BitFlags.hpp>
#include <duds/ui/graphics/BppImage.hpp>

namespace duds { namespace ui { namespace graphics {

struct GridSizeStep;

/**
 * A vector of size-step information for a Panel in a PriorityGridLayout.
 */
typedef std::vector<GridSizeStep>  GridSizeSteps;

/**
 * Informs a PriorityGridLayout object where to place and how large to make
 * Panel objects.
 * @author  Jeff Jackowski
 */
struct GridLayoutConfig {
	/**
	 * The size-steps for the panel. The size-steps are given in order of
	 * precedence; the layout will use the first size-step, starting at index
	 * 0, that fits. A size-step does not fit if it is too large
	 * for the remaining area, or if its grid location is already taken by a
	 * higher priority Panel.
	 *
	 * If no size-steps are present (sizes.empty() is true), the corresponding
	 * Panel will not be placed.
	 */
	GridSizeSteps sizes;
	/**
	 * The type for configuration flags that adjust how the panel is placed.
	 */
	typedef duds::general::BitFlags<struct GridLayoutConfigFlags>  Flags;
	/**
	 * The panel is not showm; this prevents it from being placed in the layout.
	 * Use this to temporarily make a panel disappear without having to remove
	 * configuration data from vectors.
	 */
	static constexpr Flags PanelHidden          = Flags::Bit(0);
	/**
	 * The panel is shown. This is the default.
	 */
	static constexpr Flags PanelShown           = Flags::Zero();
	/**
	 * Request that this panel's width be expanded past the requested minumum if
	 * there is extra space available on the row. If multiple panels on the row
	 * make this request, extra width will be provided about evenly among the
	 * panels.
	 */
	static constexpr Flags PanelWidthExpand     = Flags::Bit(1);
	/**
	 * Request that this panel's height be expanded past the requested minumum
	 * if there is extra space available on the output image. All panels in the
	 * same row will be given the same height. If multiple panels on the row
	 * make this request, the result is the same as one panel making the
	 * request. If multiple panels in different rows make this request, extra
	 * height will be provided about evenly among the rows.
	 */
	static constexpr Flags PanelHeightExpand    = Flags::Bit(2);
	/**
	 * Request both width and height expansion.
	 */
	static constexpr Flags PanelExpand = PanelWidthExpand | PanelHeightExpand;
	/**
	 * Place the panel's left edge to the far left in its grid spot. This is
	 * the default.
	 */
	static constexpr Flags PanelJustifyLeft     = Flags::Zero();
	/**
	 * Place the panel's right edge to the far right in its grid spot.
	 */
	static constexpr Flags PanelJustifyRight    = Flags::Bit(3);
	/**
	 * Place the panel's top edge to the far top in its grid spot. This is
	 * the default.
	 */
	static constexpr Flags PanelJustifyUp       = Flags::Zero();
	/**
	 * Place the panel's bottom edge to the far bottom in its grid spot.
	 */
	static constexpr Flags PanelJustifyDown     = Flags::Bit(4);
	/**
	 * Center the panel horizontally within its grid spot.
	 */
	static constexpr Flags PanelCenterHoriz     = Flags::Bit(5);
	/**
	 * Center the panel vertically within its grid spot.
	 */
	static constexpr Flags PanelCenterVert      = Flags::Bit(6);
	/**
	 * Center the panel horizontally and vertically within its grid spot.
	 */
	static constexpr Flags PanelCenter = PanelCenterHoriz | PanelCenterVert;
	/**
	 * Mask of all configuration flags affecting a panel's horizontal
	 * positioning.
	 */
	static constexpr Flags PanelPositionHorizMask =
		PanelJustifyRight | PanelCenterHoriz;
	/**
	 * Mask of all configuration flags affecting a panel's vertical
	 * positioning.
	 */
	static constexpr Flags PanelPositionVertMask =
		PanelJustifyDown | PanelCenterVert;
	/**
	 * Mask of all configuration flags affecting panel position.
	 */
	static constexpr Flags PanelPositionMask =
		PanelPositionHorizMask | PanelPositionVertMask;
	/**
	 * The configuration flags used for the panel for all of its size-steps.
	 * The actual flags used will be the result of OR'ing these flags with the
	 * ones of the selected size-step.
	 */
	Flags flags = Flags::Zero();
	/**
	 * Makes a new configuration that lacks any size-steps and has the default
	 * flags.
	 */
	GridLayoutConfig() = default;
	/**
	 * Constructs a new grid layout.
	 * @param gss  The size-steps for a panel.
	 * @param flg  The configuration flags.
	 */
	GridLayoutConfig(const GridSizeSteps &gss, Flags flg) :
	sizes(gss), flags(flg) { }
	/**
	 * Constructs a new grid layout.
	 * @param gss  The size-steps for a panel; these are moved into this new
	 *             object.
	 * @param flg  The configuration flags.
	 */
	GridLayoutConfig(GridSizeSteps &&gss, Flags flg) :
	sizes(std::move(gss)), flags(flg) { }
	/**
	 * Constructs a new grid layout.
	 * @param gss  The size-steps for a panel.
	 */
	GridLayoutConfig(const GridSizeSteps &gss) : sizes(gss) { }
	/**
	 * Constructs a new grid layout.
	 * @param gss  The size-steps for a panel; these are moved into this new
	 *             object.
	 */
	GridLayoutConfig(GridSizeSteps &&gss) : sizes(std::move(gss)) { }
	/**
	 * Generates a configuration using the given size-step as a template. The
	 * size-step is copied to be the only item in @a sizes, and the
	 * configuration flags are also copied into @a flags.
	 */
	GridLayoutConfig(const GridSizeStep &step);
	/**
	 * Generates a configuration using the given size-step as a template. The
	 * size-step is moved into @a sizes, and the configuration flags are also
	 * copied into @a flags.
	 */
	GridLayoutConfig(GridSizeStep &&step);
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * justified to the left edge. This is the default configuration.
	 */
	void justifyLeft() {
		flags &= ~PanelPositionHorizMask;
	}
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * justified to the right edge.
	 */
	void justifyRight() {
		flags.setMasked(PanelJustifyRight, PanelPositionHorizMask);
	}
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * centered.
	 */
	void centerHoriz() {
		flags.setMasked(PanelCenterHoriz, PanelPositionHorizMask);
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * justified to the top edge. This is the default configuration.
	 */
	void justifyUp() {
		flags &= ~PanelPositionVertMask;
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * justified to the bottom edge.
	 */
	void justifyDown() {
		flags.setMasked(PanelJustifyDown, PanelPositionVertMask);
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * centered.
	 */
	void centerVert() {
		flags.setMasked(PanelCenterVert, PanelPositionVertMask);
	}
	/**
	 * Sets all the positioning flags to indicate the panel should be
	 * centered horizontally and vertically.
	 */
	void center() {
		flags.setMasked(PanelCenter, PanelPositionMask);
	}
	/**
	 * Sets the flag to hide the panel.
	 */
	void hide() {
		flags |= PanelHidden;
	}
	/**
	 * Clear the flag to show the panel.
	 */
	void show() {
		flags &= ~PanelHidden;
	}
};

/**
 * The location of a panel in a PriorityGridLayout.
 * @author  Jeff Jackowski
 */
struct GridLocation {
	/**
	 * The column position. Because each row is independent, columns may @b not
	 * line up between rows.
	 */
	std::uint16_t c;
	/**
	 * The row position. Each row is independent of every other row.
	 */
	std::uint16_t r;
	/**
	 * Construct uninitialized.
	 */
	GridLocation() = default;
	/**
	 * Construct with the given location.
	 * The template avoids warnings when the integer type is not std::int16_t.
	 * If a value is the result of a computation, it will likely be an int
	 * unless explicitly made otherwise, which is annoying when it comes with a
	 * warning.
	 * @param col  The column.
	 * @param row  The row.
	 */
	template <
		typename Int0,
		typename Int1,
		std::enable_if_t<std::is_integral<Int0>::value, bool> = true,
		std::enable_if_t<std::is_integral<Int1>::value, bool> = true
	>
	constexpr GridLocation(Int0 col, Int1 row) :
	c((std::int16_t)col), r((std::int16_t)row) { }
	/**
	 * Obvious equality operator.
	 */
	constexpr bool operator == (const GridLocation &gl) const {
		return (gl.r == r) && (gl.c == c);
	}
	/**
	 * Obvious inequality operator.
	 */
	constexpr bool operator != (const GridLocation &gl) const {
		return (gl.r != r) || (gl.c != c);
	}
};

/**
 * A single size-step used by a PriorityGridLayout to place a Panel.
 * @author  Jeff Jackowski
 */
struct GridSizeStep {
	/**
	 * The minimum size required for the Panel for this size-step. A larger
	 * area may be assigned than the minimum.
	 */
	ImageDimensions minDim;
	/**
	 * The location within the grid to place the Panel. The rows are handled
	 * independently of each other, so columns may not line up between rows.
	 * Unused locations take up no space; rows with no columns do not appear,
	 * and columns with no panels do not cause a gap.
	 */
	GridLocation loc;
	/**
	 * Configuration flags that are OR'd with the flags in GridLayoutConfig to
	 * produce the final configuration options. If the flag
	 * @ref GridLayoutConfig::PanelHidden "PanelHidden" is set, this size-step
	 * will not be considered, but other size-steps for the panel will still
	 * be considered.
	 */
	GridLayoutConfig::Flags flags;
	/**
	 * Default constructor; does nothing.
	 */
	GridSizeStep() = default;
	/**
	 * Constructs a new GridSizeStep with the given values.
	 * @param id   The minimum dimensions for the size-step.
	 * @param gl   The grid location for the size-step.
	 * @param flg  The configuration flags.
	 */
	constexpr GridSizeStep(
		const ImageDimensions &id,
		const GridLocation &gl,
		const GridLayoutConfig::Flags flg
	) : minDim(id), loc(gl), flags(flg) { }
	/**
	 * Constructs a new GridSizeStep with the given values, and cleared (0)
	 * configuration flags.
	 * @param id   The minimum dimensions for the size-step.
	 * @param gl   The grid location for the size-step.
	 */
	constexpr GridSizeStep(
		const ImageDimensions &id,
		const GridLocation &gl
	) : minDim(id), loc(gl), flags(GridLayoutConfig::Flags::Zero()) { }
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * justified to the left edge. This is the default configuration.
	 */
	void justifyLeft() {
		flags &= ~GridLayoutConfig::PanelPositionHorizMask;
	}
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * justified to the right edge.
	 */
	void justifyRight() {
		flags.setMasked(
			GridLayoutConfig::PanelJustifyRight,
			GridLayoutConfig::PanelPositionHorizMask
		);
	}
	/**
	 * Sets the horizontal positioning flags to indicate the panel should be
	 * centered.
	 */
	void centerHoriz() {
		flags.setMasked(
			GridLayoutConfig::PanelCenterHoriz,
			GridLayoutConfig::PanelPositionHorizMask
		);
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * justified to the top edge. This is the default configuration.
	 */
	void justifyUp() {
		flags &= ~GridLayoutConfig::PanelPositionVertMask;
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * justified to the bottom edge.
	 */
	void justifyDown() {
		flags.setMasked(
			GridLayoutConfig::PanelJustifyDown,
			GridLayoutConfig::PanelPositionVertMask
		);
	}
	/**
	 * Sets the vertcal positioning flags to indicate the panel should be
	 * centered.
	 */
	void centerVert() {
		flags.setMasked(
			GridLayoutConfig::PanelCenterVert,
			GridLayoutConfig::PanelPositionVertMask
		);
	}
	/**
	 * Sets all the positioning flags to indicate the panel should be
	 * centered horizontally and vertically.
	 */
	void center() {
		flags.setMasked(
			GridLayoutConfig::PanelCenter,
			GridLayoutConfig::PanelPositionMask
		);
	}
	/**
	 * Sets the flag to hide the panel.
	 */
	void hide() {
		flags |= GridLayoutConfig::PanelHidden;
	}
	/**
	 * Clear the flag to show the panel.
	 */
	void show() {
		flags &= ~GridLayoutConfig::PanelHidden;
	}
};

} } }

#endif        //  #ifndef GRIDLAYOUTCONFIG_HPP
