/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/graphics/GridLayoutConfig.hpp>

namespace duds { namespace ui { namespace graphics {

constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelHidden;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelWidthExpand;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelHeightExpand;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelExpand;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelJustifyLeft;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelJustifyRight;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelJustifyUp;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelJustifyDown;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelCenterHoriz;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelCenterVert;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelCenter;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelPositionHorizMask;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelPositionVertMask;
constexpr GridLayoutConfig::Flags GridLayoutConfig::PanelPositionMask;


GridLayoutConfig::GridLayoutConfig(const GridSizeStep &step) : flags(step.flags) {
	sizes.push_back(step);
}

GridLayoutConfig::GridLayoutConfig(GridSizeStep &&step) : flags(step.flags) {
	sizes.emplace_back(std::move(step));
}


} } }
