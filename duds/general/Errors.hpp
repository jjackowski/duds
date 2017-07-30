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
 * General errors.
 */

#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <boost/exception/exception.hpp>
#include <exception>

namespace duds { namespace general {

/**
 * An error indicating an attempt to use an already destructed object.
 * This may be thrown by functions that block a thread until some resource
 * becomes available, but the object is destroyed before the resource can be
 * provided.
 */
struct ObjectDestructed : virtual std::exception, virtual boost::exception { };

/**
 * Indicates that the requested operation or called function is not implemented.
 */
struct Unimplemented : virtual std::exception, virtual boost::exception { };

/**
 * A general I/O error.
 */
struct IoError : virtual std::exception, virtual boost::exception { };

/**
 * An I/O error involving a file. Use boost::errinfo_file_name to indicate
 * the file name in the exception (from boost/exception/errinfo_file_name.hpp).
 */
struct FileIoError : IoError { };

} }

#endif        //  #ifndef ERRORS_HPP
