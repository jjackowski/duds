/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/exception/info.hpp>  // other includes may be needed to throw/inspect

namespace duds { namespace hardware { namespace interface {

/**
 * The base type of all chip select related errors.
 */
struct ChipSelectError : virtual std::exception, virtual boost::exception { };

/**
 * Indicates an attempt to select a non-existant chip.
 */
struct ChipSelectInvalidChip : ChipSelectError { };

/**
 * An attempt was made to retire a ChipAccess object that was not the active
 * access object for the manager.
 */
struct ChipSelectInvalidAccess : ChipSelectError { };

/**
 * An attempt was made to change the set of valid chips or exactly how a
 * particluar chip might be selected while a ChipAccess object made by the
 * chip select manager currently exists.
 */
struct ChipSelectInUse : ChipSelectError { };

/**
 * A ChipSelectManager is required for the operation but is not set.
 */
struct ChipSelectBadManager : ChipSelectError { };

/**
 * A ChipAccess object was given to ChipSelectManager::access(ChipAccess &, int)
 * that is already providing access.
 */
struct ChipSelectAccessInUse : ChipSelectError { };

/**
 * A ChipSelectManager was given more pins to use than the implementation
 * supports.
 */
struct ChipSelectTooManyPins : ChipSelectError { };

/**
 * The chip select ID relavent to the error.
 * @note  This is not an error code. Error is used in the name to distinguish
 *        this type from other types that do not deal with errors.
 */
typedef boost::error_info<struct Info_ChipId, int>  ChipSelectIdError;

} } }

