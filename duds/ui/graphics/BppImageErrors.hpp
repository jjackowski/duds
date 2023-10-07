/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef BPPIMAGEERRORS_HPP
#define BPPIMAGEERRORS_HPP

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <exception>
#include <memory>
#include <cstdint>

namespace duds { namespace ui { namespace graphics {

/**
 * The base class for errors related to the use of images.
 */
struct ImageError : virtual std::exception, virtual boost::exception { };

/**
 * Data with an image to parse was too short to hold the image.
 */
struct ImageTruncatedError : ImageError { };

/**
 * A problem with image bounds, such as the use of a location beyond the
 * image's dimensions.
 */
struct ImageBoundsError : ImageError { };

/**
 * Indicates the image has zero size when an operation requires some image
 * data.
 */
struct ImageZeroSizeError : ImageBoundsError { };

/**
 * The Pixel or ConstPixel iterator object was dereferenced when in at the end.
 */
struct ImageIteratorEndError : ImageBoundsError { };

/**
 * The base class for errors related to the use of image archives.
 */
struct ImageArchiveError : virtual std::exception, virtual boost::exception { };

/**
 * An image was requested that the archive does not contain.
 */
struct ImageNotFoundError : ImageArchiveError { };

/**
 * The base class for errors resulting from the attempt to read an image
 * archive stream or file.
 */
struct ImageArchiveStreamError : ImageArchiveError { };

/**
 * The stream appears to not be an image archive.
 */
struct ImageNotArchiveStreamError : ImageArchiveStreamError { };

/**
 * The archive is in an unsupported version of the format.
 */
struct ImageArchiveUnsupportedVersionError : ImageArchiveStreamError { };

/**
 * The stream appears to end early.
 */
struct ImageArchiveStreamTruncatedError : ImageArchiveStreamError { };

/**
 * An attempt was made to advance past the end of a archive stream.
 */
struct ImageArchivePastEndError : ImageArchiveStreamError { };

/**
 * The name of the image involved in an ImageArchiveError.
 */
typedef boost::error_info<struct Info_ArchiveImageName, std::string>
	ImageArchiveImageName;

/**
 * The name of the image archive file involved in an ImageArchiveStreamError.
 * This is only added if a file name is known; it is not added if a std::istream
 * is used.
 */
typedef boost::error_info<struct Info_ImageArcFileName, std::string>
	ImageArchiveFileName;

/**
 * The name of the image involved in an ImageArchiveError.
 */
typedef boost::error_info<struct Info_ImageArcName, std::uint32_t>
	ImageArchiveVersion;

/**
 * A glyph required to render a string is not availble in the font.
 */
struct GlyphNotFoundError : ImageError { };

/**
 * A string, like one requested for rendering in a specific font.
 */
typedef boost::error_info<struct Info_String, std::string> String;

/**
 * A character, like one requested for rendering in a specific font.
 */
typedef boost::error_info<struct Info_Character, char32_t> Character;

/**
 * The maximum size of a BppStringCache is zero. If allowed, rendered text
 * would be destroyed before it could be used.
 */
struct StringCacheZeroSize : ImageError { };

/**
 * A given string cache does not correspond to the given font.
 */
struct FontStringCacheMismatchError : ImageError { };

/**
 * A specified font is not present in the font pool.
 */
struct FontNotFoundError : ImageError { };

/**
 * The name of the font and is not present in the font pool.
 */
typedef boost::error_info<struct Info_FontName, std::string> FontName;

} } }

#endif        //  #ifndef BPPIMAGEERRORS_HPP
