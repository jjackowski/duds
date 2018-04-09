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
#include <boost/exception/info.hpp>
#include <exception>

#ifdef DUDS_ERRORS_TERSE
#define DUDS_THROW_EXCEPTION(x) ::boost::throw_exception(x)
#elif defined(DUDS_ERRORS_VERBOSE) || defined(DOXYGEN)
/**
 * Requests that addr2line be used to relate addresses on a stack trace to
 * locations in source code.
 */
#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>
/**
 * Works like BOOST_THROW_EXCEPTION, but includes a stack trace if
 * DUDS_ERRORS_VERBOSE is defined. The build normally defines
 * DUDS_ERRORS_VERBOSE for debug builds unless the following bug makes
 * including boost/stacktrace.hpp fail with a compile-time error.
 * @bug  Boost 1.65.0, and possibily earler versions, include push_options.pp
 *       and pop_options.pp, which are not part of the Boost install. The
 *       issue was resolved in Boost 1.65.1.
 * @pre  On Linux and similar systems, the dl library is required. If not
 *       found, DUDS_ERRORS_VERBOSE will not be defined so that the
 *       implementation in BOOST_THROW_EXCEPTION will be used instead.
 */
#define DUDS_THROW_EXCEPTION(x) \
	::boost::throw_exception( ::boost::enable_error_info(x) << \
	::boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) << \
	::boost::throw_file(__FILE__) <<	::boost::throw_line((int)__LINE__) << \
	duds::general::StackTrace(boost::stacktrace::stacktrace()))
#else
#define DUDS_THROW_EXCEPTION(x)  BOOST_THROW_EXCEPTION(x)
#endif

namespace duds { namespace general {

#if defined(DUDS_ERRORS_VERBOSE) || defined(DOXYGEN)
/**
 * Includes stack trace information in an exception. This is included in
 * exceptions that are thrown with ::DUDS_THROW_EXCEPTION when
 * DUDS_ERRORS_VERBOSE is defined. The build normally defines
 * DUDS_ERRORS_VERBOSE for debug builds when the required libraries are
 * available and usable; see ::DUDS_THROW_EXCEPTION for more info.
 * @pre  On Linux and similar systems, the dl library is required.
 * @bug  The captured stack trace shows where this object is created rather
 *       than where the exception is thrown. The function throwing the
 *       exception may not be in the trace. However, the location of the
 *       throw is included in other exception information by
 *       ::DUDS_THROW_EXCEPTION and BOOST_THROW_EXCEPTION.
 */
typedef boost::error_info<
	struct Info_Stacktrace,
	boost::stacktrace::stacktrace
> StackTrace;
#endif

/**
 * An error indicating an attempt to use an already destructed object.
 * This may be thrown by functions that block a thread until some resource
 * becomes available, but the object is destroyed before the resource can be
 * provided.
 */
struct ObjectDestructedError : virtual std::exception, virtual boost::exception { };

/**
 * Indicates that the requested operation or called function is not implemented.
 */
struct UnimplementedError : virtual std::exception, virtual boost::exception { };

/**
 * A general I/O error.
 */
struct IoError : virtual std::exception, virtual boost::exception { };

/**
 * A bad checksum value was found.
 */
struct ChecksumError : IoError { };

/**
 * An incorrect cyclic redundancy code (CRC) value was found.
 */
struct CrcError : ChecksumError { };

/**
 * An I/O error involving a file. Use boost::errinfo_file_name to indicate
 * the file name in the exception (from boost/exception/errinfo_file_name.hpp).
 */
struct FileIoError : IoError { };

} }

#endif        //  #ifndef ERRORS_HPP
