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
 * The root for various device drivers, and the place for common support code.
 */
namespace devices {

/**
 * The base type for errors from devices. These errors should be kept separate
 * from errors from methods of communication.
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
