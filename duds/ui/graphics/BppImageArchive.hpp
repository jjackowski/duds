/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/ui/graphics/BppImage.hpp>
#include <boost/noncopyable.hpp>
#include <unordered_map>

namespace duds { namespace ui { namespace graphics {

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
 * An archive of BppImage objects keyed by a string name.
 *
 * This class is not thread safe. That may change in the future.
 *
 * @author  Jeff Jackowski
 */
class BppImageArchive : boost::noncopyable {
	/**
	 * The images keyed by name.
	 */
	std::unordered_map<std::string, BppImageSptr>  arc;
public:
	/**
	 * Loads images from an image archive in the specified file.
	 * If the archive stream has an image with the same name as one already
	 * inside this object, the already loaded image will be replaced by putting
	 * a new BppImage object in a new shared pointer in the old one's place.
	 * This does not modify the previously loaded image of the same name.
	 * @throw ImageArchiveStreamError  Failed to open the file.
	 */
	void load(const std::string &path);
	/**
	 * Loads images from the given input stream. Seeking is not performed.
	 * If the archive stream has an image with the same name as one already
	 * inside this object, the already loaded image will be replaced by putting
	 * a new BppImage object in a new shared pointer in the old one's place.
	 * This does not modify the previously loaded image of the same name.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	void load(std::istream &is);
	/**
	 * Adds an image to the archive.
	 * @param name  The name to give the image. If there is already an image
	 *              by that name in this archive, it will be replaced, but
	 *              the image object will not be modified.
	 * @param img   The image to store.
	 */
	void add(const std::string &name, const BppImageSptr &img);
	/**
	 * Moves an image into the archive.
	 * @param name  The name to give the image. If there is already an image
	 *              by that name in this archive, it will be replaced, but
	 *              the image object will not be modified.
	 * @param img   The image to store. The shared pointer will be moved.
	 */
	void add(const std::string &name, BppImageSptr &&img);
	/**
	 * Returns the image with the given name.
	 * If there is no image for the name, an exception will be thrown. The
	 * advantage is that the shared pointer to the image is not copied
	 * (better performance if an error is unlikely) and there is no need for 
	 * a conditional to check for the lack of an image (possibly cleaner code).
	 * @param name  The name of the image to find.
	 * @return      A shared pointer to the requested image.
	 * @throw       ImageNotFoundError  The image @a name is not in the archive.
	 */
	const BppImageSptr &get(const std::string &name) const;
	/**
	 * Returns the image with the given name.
	 * If there is no image for the name, an empty shared pointer is returned.
	 * The advantage is that no exception handling is required, which can
	 * provide better performance when the lack of an image is a common
	 * occurance.
	 * @param name  The name of the image to find.
	 * @return      A shared pointer to the requested image.
	 */
	BppImageSptr tryGet(const std::string &name) const;
};

} } }
