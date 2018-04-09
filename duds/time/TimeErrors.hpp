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

namespace duds { namespace time {

/**
 * The base type of all time related errors.
 */
struct TimeError : virtual std::exception, virtual boost::exception { };

/**
 * A base class for time related range errors. Thrown in cases where the
 * derived classes are not applicable.
 */
struct TimeOutOfRangeError : TimeError { };
/*
/ **
 * The specified year is outside the allowable range.
 * /
struct YearOutOfRangeError : TimeOutOfRangeError { };

/ **
 * The specified month is outside the allowable range.
 * /
struct MonthOutOfRangeError : TimeOutOfRangeError { };
*/

namespace planetary {

/**
 * The specified number of leap seconds is outside the allowable range.
 */
struct LeapOutOfRangeError : TimeOutOfRangeError { };

/**
 * More than one leap second was specified for the same time.
 */
struct DuplicateLeapSecond : TimeError { };

/**
 * An error involving reading a zoneinfo database file.
 */
struct ZoneinfoError : TimeError { };

/**
 * More than one leap second was specified for the same time in a zoneinfo
 * database file.
 */
struct ZoneDuplicateLeap : ZoneinfoError, virtual DuplicateLeapSecond { };

/**
 * The zoneinfo file claimed to have more leap second records than were
 * read from the file. This could be from a truncated file, or an I/O problem
 * that prevented reading the whole file.
 */
struct ZoneTruncated : ZoneinfoError, virtual duds::general::FileIoError { };

/**
 * An I/O error occured while reading a zoneinfo database file.
 */
struct ZoneIoError : ZoneinfoError, virtual duds::general::FileIoError { };
/*
/ **
 * The year value specified as part of something that caused a TimeError
 * to be thrown.
 * /
typedef boost::error_info<struct Info_year, int>  YearInError;

/ **
 * The month value specified as part of something that caused a TimeError
 * to be thrown.
 * /
typedef boost::error_info<struct Info_year, int>  MonthInError;

/ **
 * The leap secons value specified as part of something that caused a
 * TimeError to be thrown.
 * /
typedef boost::error_info<struct Info_year, int>  LeapInError;
*/
} } }
