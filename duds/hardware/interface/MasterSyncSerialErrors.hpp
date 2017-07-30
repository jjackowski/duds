/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/exception/info.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Base class for all errors specific to MasterSyncSerial and its derivatives.
 * Errors not derived from this class might still be thrown. For example,
 * DigitalIoMasterSyncSerial throws some errors derived from PinError when
 * the problem has to do with a DigitalPin.
 */
struct SyncSerialError : virtual std::exception, virtual boost::exception { };

/**
 * The requested mode of operation is unsupported; intended for use by
 * implementing classes that must impose limitations like transfering whole
 * bytes rather than an arbitrary number of bits.
 */
struct SyncSerialUnsupported : SyncSerialError { };

/**
 * The requested operation requires communication to be in the ready state.
 */
struct SyncSerialNotReady : SyncSerialError { };

/**
 * The requested operation requires communication to be in the open state.
 */
struct SyncSerialNotOpen : SyncSerialError { };

/**
 * A transfer operation was requested when not in the communicating state.
 */
struct SyncSerialNotCommunicating : SyncSerialError { };

/**
 * The requested operation requires communication to not already be in use.
 */
struct SyncSerialInUse : SyncSerialError { };

/**
 * A full-duplex configuration or data transfer operation was requested on
 * a half-duplex serial interface.
 */
struct SyncSerialNotFullDuplex : SyncSerialError { };

/**
 * A half-duplex configuration was requested on a full-duplex serial interface.
 */
struct SyncSerialNotHalfDuplex : SyncSerialError { };

/**
 * A chip select object was specified on a serial interface that does not use
 * a chip select.
 */
struct SyncSerialSelectNotUsed : SyncSerialError { };

/**
 * An attempt was made to retire a MasterSyncSerialAccess object that does not
 * belong to the serial interface.
 */
struct SyncSerialInvalidAccess : SyncSerialError { };

/**
 * The given MasterSyncSerialAccess object already provides access to a
 * MasterSyncSerial object. The MasterSyncSerial may be different or the same
 * as the one which throws this exception.
 */
struct SyncSerialAccessInUse : SyncSerialError { };

/**
 * General I/O error.
 */
struct SyncSerialIoError : SyncSerialError { };

} } }

