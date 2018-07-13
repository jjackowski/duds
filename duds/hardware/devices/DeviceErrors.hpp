/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DEVICEERRORS_HPP
#define DEVICEERRORS_HPP

#include <boost/exception/info.hpp>

namespace duds { namespace hardware {

/**
 * The root namespace for various device drivers to support specific hardware
 * items. Code that is not device specific should go elsewhere.
 */
namespace devices {

/**
 * The base type for errors from devices. These errors should be kept separate
 * from errors from methods of communication.
 * @todo  Expand the set of general device errors to handle a wider variety of
 *        errors to limit the need for device specific errors. However, maybe
 *        all errors from a device should also be derived from a single class
 *        using multiple inheritance. 
 */
struct DeviceError : virtual std::exception, virtual boost::exception { };

/**
 * An attempt was made to use a device prior to running a required
 * initialization step.
 */
struct DeviceUninitalized : DeviceError { };

/**
 * An attempt was made to use a device that seems to exist, but the responding
 * device is not the type that was expected.
 */
struct DeviceMisidentified : DeviceError { };

} } }

#endif        //  #ifndef DEVICEERRORS_HPP
