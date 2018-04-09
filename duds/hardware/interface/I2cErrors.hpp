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
 * Base class for all errors specific to I2C communications and used
 * for very general errors. Errors not derived from this class might still be
 * thrown. For example, an implementation using DigitalPin objects may throw
 * exceptions related to their operation.
 */
struct I2cError : virtual std::exception, virtual boost::exception { };

/**
 * A ConversationPart of an invalid length was specified. An input part with
 * varying length should be at least as long as the maximum amount that can
 * be received, including the length value. Implementations may impose further
 * restrictions.
 */
struct I2cErrorPartLength : I2cError { };

/**
 * The number of conversation parts is too great for the implementation to
 * handle.
 */
struct I2cErrorConversationLength : I2cError { };

/**
 * The device did not respond to its address (NACK). It could be a transient
 * error, or there may not be a device at the address. Devices that support a
 * reset by I2C command will normally result in this error after the reset
 * command is sent.
 */
struct I2cErrorNoDevice : I2cError { };

/**
 * The attempted operation is not supported by the bus master.
 */
struct I2cErrorUnsupported : I2cError { };

/**
 * The device has failed to conform to the protocol.
 */
struct I2cErrorProtocol : I2cError { };

/**
 * The operation took too long. This can be caused by a device taking too long,
 * maybe stretching a clock for example, and going past a limit imposed by
 * either the SMBus protocol or the system's device driver. This is not caused
 * by scheduling on the computer; if multiple processes attempt to use the bus
 * simultaneously, the resulting delay some of them will see does not count
 * toward the timeout.
 */
struct I2cErrorTimeout : I2cError { };

/**
 * A timeout occured while waiting to use the bus. This error can be generated
 * by an SMBus master after the bus has been in use for longer than the
 * protocol allows. It can be caused by another master, an errant device, or
 * a fault on the SMBus master whose use caused this error. This is not caused
 * by scheduling on the computer; if multiple processes attempt to use the bus
 * simultaneously, the resulting delay some of them will see does not count
 * toward the timeout. Only use of the bus itself counts.
 */
struct I2cErrorBusy : I2cErrorTimeout { };

/**
 * Provides the device (slave) address along with an error.
 */
typedef boost::error_info<struct Info_i2cdevaddr, int>  I2cDeviceAddr;

} } }
