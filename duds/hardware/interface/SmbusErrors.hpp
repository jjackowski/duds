/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Base class for all errors specific to SMBus communications and used
 * for very general errors. Errors not derived from this class might still be
 * thrown. For example, an implementation using DigitalPin objects may throw
 * exceptions related to their operation.
 */
struct SmbusError : virtual std::exception, virtual boost::exception { };

/**
 * The received message included a bad checksum and Packet Error Checking is
 * in use.
 */
struct SmbusErrorPec : virtual SmbusError, virtual duds::general::ChecksumError { };

/**
 * A message of an invalid length was specified, or a message too big for a
 * buffer was received. With SMBus, block messages must be no more than 32
 * bytes.
 */
struct SmbusErrorMessageLength : SmbusError { };

/**
 * An attempt was made to use a non-existent SMBus bus.
 */
struct SmbusErrorNoBus : SmbusError { };

/**
 * The device did not respond to its address. It could be a transient error,
 * or there may not be a device at the address.
 */
struct SmbusErrorNoDevice : SmbusError { };

/**
 * The attempted operation is not supported by the bus master.
 */
struct SmbusErrorUnsupported : SmbusError { };

/**
 * The device has failed to conform to the protocol.
 */
struct SmbusErrorProtocol : SmbusError { };

/**
 * The operation took too long. This can be caused by a device taking too long,
 * maybe stretching a clock for example, and going past a limit imposed by
 * either the SMBus protocol or the system's device driver. This is not caused
 * by scheduling on the computer; if multiple processes attempt to use the bus
 * simultaneously, the resulting delay some of them will see does not count
 * toward the timeout.
 */
struct SmbusErrorTimeout : SmbusError { };

/**
 * A timeout occured while waiting to use the bus. This error can be generated
 * by an SMBus master after the bus has been in use for longer than the
 * protocol allows. It can be caused by another master, an errant device, or
 * a fault on the SMBus master whose use caused this error. This is not caused
 * by scheduling on the computer; if multiple processes attempt to use the bus
 * simultaneously, the resulting delay some of them will see does not count
 * toward the timeout. Only use of the bus itself counts.
 */
struct SmbusErrorBusy : SmbusErrorTimeout { };

/**
 * Provides the device (slave) address along with an error.
 */
typedef boost::error_info<struct Info_smbusdevaddr, int>  SmbusDeviceAddr;

} } }
