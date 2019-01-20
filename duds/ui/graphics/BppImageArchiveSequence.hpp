/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppImage.hpp>
#include <boost/noncopyable.hpp>
#include <fstream>

namespace duds { namespace ui { namespace graphics {

/**
 * Provides an input iterator to a sequence of named bit-per-pixel images
 * read from an input stream.
 * @author  Jeff Jackowski
 */
class BppImageArchiveSequence : boost::noncopyable {
protected:
	/**
	 * The stream which contains image data. It might not be owned by this
	 * object.
	 */
	std::istream *is;
	/**
	 * Stores the last parsed image data. It is placed here instead of the
	 * iterator to allow the iterator class to have a simple constructor, a
	 * trivial destructor, and only one small member. Without this, the end
	 * iterator cannot be a constexpr object.
	 */
	std::pair<std::string, BppImageSptr> deref;
	/**
	 * Only use when a derived class will set @a is.
	 */
	BppImageArchiveSequence() = default;
public:
	/**
	 * Constructs the sequence parser to use the stream @a str.
	 * @param str  The stream that will provide the image data. It must remain
	 *             valid (useable object, not error-free) during the life span
	 *             of this object.
	 */
	BppImageArchiveSequence(std::istream &str) : is(&str) { }
	/**
	 * An input iterator that will parse and supply image data from the stream.
	 * When incremented, any copies of the iterator must be considered
	 * invalid. The default constructed iterator is always equivalent to the
	 * end iterator, and is a constexpr object.
	 * @author  Jeff Jackowski
	 */
	class iterator {
		/**
		 * The object containing the data required for this iterator to work.
		 */
		BppImageArchiveSequence *bias;
	public:
		/**
		 * Makes a new iterator, but does not prepare the
		 * BppImageArchiveSequence object to use the iterator, and results in
		 * an iterator that is invalid. BppImageArchiveSequence::begin()
		 * implements the remaining steps to make the iterator valid.
		 */
		iterator(BppImageArchiveSequence *parent) : bias(parent) { }
		/**
		 * Equivalent to the end iterator.
		 */
		constexpr iterator() : bias(nullptr) { }
		/**
		 * Equality operator.
		 */
		bool operator==(const iterator &i) const {
			return (bias == i.bias);
		}
		/**
		 * Inequality operator.
		 */
		bool operator!=(const iterator &i) const {
			return (bias != i.bias);
		}
		/**
		 * Pre-increment operator; parses the next image.
		 * @throw ImageArchivePastEndError          The incremented iterator is
		 *                                          the end iterator.
		 * @throw ImageArchiveStreamTruncatedError  The next image has
		 *                                          incomplete data.
		 */
		iterator &operator++();
		/**
		 * Post-increment operator; involves making a copy of this iterator.
		 */
		iterator operator++(int) {
			iterator i(*this);
			operator++();
			return i;
		}
		/**
		 * Provides a std::pair with the name of the image and the image itself.
		 */
		const std::pair<std::string, BppImageSptr> &operator*() const {
			return bias->deref;
		}
		/**
		 * Provides a pointer to a std::pair with the name of the image and
		 * the image itself.
		 */
		const std::pair<std::string, BppImageSptr> *operator->() const {
			return &(bias->deref);
		}
		/**
		 * Returns the name of the image.
		 */
		const std::string &name() const {
			return bias->deref.first;
		}
		/**
		 * Returns the image.
		 */
		const BppImageSptr &image() const {
			return bias->deref.second;
		}
	};
	friend iterator;
	/**
	 * Parses the first image in the stream and returns an iterator to that
	 * data. This function must only be called once on a specific object. It
	 * does @b not parse any headers, like the one used for bit-per-pixel image
	 * archive files. To parse headers, call readHeaders() first.
	 * @pre   begin() has not yet been called on this object.
	 * @post  begin() must not be called again on this object.
	 * @throw ImageArchiveStreamTruncatedError  The first image has
	 *                                          incomplete data.
	 */
	iterator begin();
	/**
	 * Returns the end iterator.
	 */
	static constexpr iterator end() {
		return iterator();
	}
	/**
	 * Parses the headers used in BppImageArchive files. The parsed data
	 * includes a version number that is currently ignored, but could be used
	 * in the future to allow the image data format to change.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	void readHeader();
};

/**
 * Provides an input iterator to a sequence of named bit-per-pixel images
 * read from an archive file. Unlike BppImageArchiveSequence, this requires
 * the presence of a header at the start of the file stream. The readHeader()
 * function should not be called when using this class.
 * @author  Jeff Jackowski
 */
class BppImageArchiveFile : public BppImageArchiveSequence {
	/**
	 * The input file stream.
	 */
	std::ifstream inf;
public:
	/**
	 * Opens the given file and parses headers at the start.
	 * readHeader() is called within this function and must not be called
	 * again.
	 * @post  If an exception wasn't thrown, begin() may be called.
	 * @param path  The path of the image archive file to open.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	BppImageArchiveFile(const std::string &path);
};

} } }
