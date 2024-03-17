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
#include <duds/general/Spinlock.hpp>
#include <unordered_map>

namespace duds { namespace ui { namespace graphics {

/**
 * An archive of BppImage objects keyed by a string name.
 *
 * This class is thread safe.
 *
 * @author  Jeff Jackowski
 */
class BppImageArchive : boost::noncopyable {
public:
	typedef std::unordered_map<std::string, BppImageSptr>  ImageMap;
private:
	/**
	 * The images keyed by name.
	 */
	ImageMap arc;
	/**
	 * Used for thread safety.
	 */
	mutable duds::general::Spinlock block;
public:
	/**
	 * Makes an empty image archive.
	 */
	BppImageArchive() = default;
	/**
	 * Returns a new empty BppImageArchive object in a shared pointer.
	 */
	static std::shared_ptr<BppImageArchive> make() {
		return std::make_shared<BppImageArchive>();
	}
	/**
	 * @copydoc load(const std::string &)
	 */
	BppImageArchive(const std::string &path) {
		load(path);
	}
	/**
	 * Returns a new BppImageArchive object in a shared pointer, and calls
	 * load(const std::string &).
	 */
	static std::shared_ptr<BppImageArchive> make(const std::string &path) {
		return std::make_shared<BppImageArchive>(path);
	}
	/**
	 * @copydoc load(std::istream &)
	 */
	BppImageArchive(std::istream &is) {
		load(is);
	}
	/**
	 * Returns a new BppImageArchive object in a shared pointer, and calls
	 * load(std::istream &).
	 */
	static std::shared_ptr<BppImageArchive> make(std::istream &is) {
		return std::make_shared<BppImageArchive>(is);
	}
	/**
	 * Loads images from an image archive in the specified file.
	 * If the archive stream has an image with the same name as one already
	 * inside this object, the already loaded image will be replaced by putting
	 * a new BppImage object in a new shared pointer in the old one's place.
	 * This does not modify the previously loaded image of the same name.
	 * @param path  The path of the archive file to load.
	 * @throw ImageArchiveStreamError  Failed to open the file.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	void load(const std::string &path);
	/**
	 * Loads images from the given input stream. Seeking is not performed.
	 * If the archive stream has an image with the same name as one already
	 * inside this object, the already loaded image will be replaced by putting
	 * a new BppImage object in a new shared pointer in the old one's place.
	 * This does not modify the previously loaded image of the same name.
	 * @param is  The input stream that will provide the image archive.
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
	/**
	 * Returns a begining iterator to inspect the images within the archive.
	 * Using the iterator is not a thread-safe operation.
	 */
	ImageMap::const_iterator begin() const {
		return arc.cbegin();
	}
	/**
	 * Returns an end iterator to inspect the images within the archive.
	 * Using the iterator is not a thread-safe operation.
	 */
	ImageMap::const_iterator end() const {
		return arc.cend();
	}
};

typedef std::shared_ptr<BppImageArchive>  BppImageArchiveSptr;
typedef std::weak_ptr<BppImageArchive>  BppImageArchiveWptr;

} } }
