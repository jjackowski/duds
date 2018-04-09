/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * Various errors involving the use of GPIO hardware.
 * @author  Jeff Jackowski
 */

#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Base exception type for all errors about pins.
 * While these errors are not intended to be specific to digital pins and
 * ports, those are the only kinds currently supported. The comments only
 * mention what exists.
 */
struct PinError : virtual std::exception, virtual boost::exception { };

/**
 * An I/O error prevented the operation on the pin from succeeding. This may
 * be thrown to indicate that the pin cannot be accessed, or that there was
 * an error during an access.
 */
struct PinIoError : PinError, virtual duds::general::IoError { };

/**
 * The requested operation is not supported by the specific pin.
 */
struct PinUnsupportedOperation : PinError { };

/**
 * The requested operation requires the use of the wrong, or not the current,
 * I/O direction.
 */
struct PinWrongDirection : PinError { };

/**
 * The operation has too few or much data to work on the pins, which can
 * alternately be stated as having too few or too many pins for the operation.
 */
struct PinRangeError : PinError { };

/**
 * A DigitalPinAccess or DigitalPinSetAccess object cannot be obtained because
 * one already exists with access to the pin. This can only occur when a call
 * to request one or more access object(s) that specifies multiple pins
 * includes a pin more than once.
 */
struct PinInUse : PinError { };

/**
 * A request to add a pin to a DigitalPort cannot be fullfilled because the
 * port already has a pin with the same ID.
 */
struct PinAlreadyExists : PinError { };

/**
 * A pin required for the operation does not exist or is unavailable to the
 * process.
 */
struct PinDoesNotExist : PinError { };

/**
 * An attempt was made to use a DigitalPinSetAccess object with two different
 * DigitalPort objects.
 */
struct PinSetWrongPort : PinError { };

/**
 * A request was made to access zero pins.
 */
struct PinEmptyAccessRequest : PinError { };

// Old stuff that may be useful again in the future after more refactoring
/*

/ **
 * The requested PinIndex does not exist. These typically come from a
 * PinDirectory.
 * /
struct PinIndexDoesNotExist : PinError { };

/ **
 * An operation that requires a PinStore was attempted before a PinStore was in
 * place.
 * /
struct PinStoreRequired : PinError { };

/ **
 * A PinIndex was directed to attach to a PinStore after it had already been
 * attached to a PinStore without first detaching.
 * /
struct PinStoreAlreadyAttached : PinError { };

/ **
 * An operation that requires a DigitalPin to be attached to a particular
 * PinStore was given a DigitalPin that is either unattached or attached to
 * another PinStore.
 * /
struct PinStoreMismatch : PinError { };

/ **
 * The name of a PinIndex; added by PinDirectory.
 * /
typedef boost::error_info<struct Info_indexname, std::string>  PinErrorIndex;

*/

/**
 * A name or function associated with the pin(s).
 * @note  This is not an error description string. Error is used in the name
 *        to distinguish this type from other types that do not deal with
 *        errors.
 */
typedef boost::error_info<struct Info_PinName, std::string>  PinErrorName;

/**
 * The pin global ID involved in the error. It will always be a global ID,
 * even from functions that take a local ID.
 * @note  This is not an error code. Error is being used in the name to
 *        distinguish this type from other types that do not deal with errors.
 */
typedef boost::error_info<struct Info_PinId, unsigned int>  PinErrorId;

} } } // namespaces

