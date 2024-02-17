/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef LAYOUTERRORS_HPP
#define LAYOUTERRORS_HPP

#include <duds/ui/graphics/BppImageErrors.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * The base class for errors from layout managers.
 */
struct LayoutError : virtual std::exception, virtual boost::exception { };

/**
 * An attempt was made to add a panel using an invalid priority value.
 * Currently, zero is the only invalid value. It is used internally to denote
 * the absence of a panel.
 */
struct LayoutPriorityInvalidError : LayoutError { };

/**
 * Thrown by Panels during rendering if the panel discovers the size required
 * for its image will not fit into the layout.
 */
struct PanelImageTooLargeError : ImageError, LayoutError { };

/**
 * A panel identified by priority key could not be found.
 */
struct PanelNotFoundError : LayoutError { };

/**
 * A panel that can only be used in one layout was added to another layout.
 */
struct PanelAlreadyAddedError : LayoutError { };

/**
 * An operation that requires a panel to be added to a layout was attempted
 * on a panel that is not in any layout.
 */
struct PanelNotAddedError : LayoutError { };

/**
 * Identifies a Panel object's layout priority in errors.
 */
typedef boost::error_info<struct Info_PanelPriority, int> PanelPriority;

/**
 * Identifies a Panel's selected size-step in errors.
 */
typedef boost::error_info<struct Info_PanelSizeStep, int> PanelSizeStep;

} } }

#endif        //  #ifndef LAYOUTERRORS_HPP
