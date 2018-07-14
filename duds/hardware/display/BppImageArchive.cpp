/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/display/BppImageArchive.hpp>
#include <duds/general/Errors.hpp>
#include <fstream>
#include <map>

#include <iostream>

namespace duds { namespace hardware { namespace display {

void BppImageArchive::load(const std::string &path) {
	std::ifstream is(path);
	if (!is.good()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamError() <<
			ImageArchiveFileName(path)
		);
	}
	try {
		load(is);
	} catch (boost::exception &be) {
		// add the file name to the error metadata
		be << ImageArchiveFileName(path);
		throw;
	}
}

void BppImageArchive::load(std::istream &is) {
	// check for header
	std::string str;
	// do not read into the version value that follows the header
	is.width(4);
	is >> str;
	is.width(0);
	if (str != "BPPI") {
		DUDS_THROW_EXCEPTION(ImageNotArchiveStreamError());
	}
	// read version
	std::vector<char> buff(4);
	is.read(&(buff[0]), 4);
	if (!is.good()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError());
	}
	std::uint32_t ver = buff[0] | (buff[1] << 8) | (buff[2] << 16) |
		(buff[3] << 24);
	// version check; only 0 is supported
	if (ver != 0) {
		DUDS_THROW_EXCEPTION(ImageArchiveUnsupportedVersionError() <<
			ImageArchiveVersion(ver)
		);
	}
	// loop for each image
	std::int16_t w, h;
	while (is.good()) {
		// read in the name
		is >> str;
		// EOF is only set after attempting to read more than the file holds, so
		// it may be set now, but is not an error condition
		if (is.eof()) {
			break;
		}
		int ex = is.get();
		if ((ex != ' ') || !is.good()) {
			DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError());
		}
		// read in the dimensions
		buff.resize(4);
		is.read(&(buff[0]), 4);
		if (!is.good()) {
			DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError() <<
				ImageArchiveImageName(str)
			);
		}
		w = buff[0] | (buff[1] << 8);
		h = buff[2] | (buff[3] << 8);
		// calculate size
		std::size_t imgwidth = w / 8 + ((w % 8) ? 1 : 0);
		std::size_t length = imgwidth * h + 4;
		// allocate space
		buff.resize(length);
		// read rest of image
		is.read(&(buff[4]), length - 4);
		if (!is.good() && !is.eof()) {
			DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError() <<
				ImageArchiveImageName(str)
			);
		}
		// create and store image object
		arc[str] = std::make_shared<BppImage>(buff);
	}
}

void BppImageArchive::add(
	const std::string &name,
	const std::shared_ptr<BppImage> &img
) {
	arc[name] = img;
}

void BppImageArchive::add(
	const std::string &name,
	std::shared_ptr<BppImage> &&img
) {
	arc[name] = std::move(img);
}

const std::shared_ptr<BppImage> &BppImageArchive::get(
	const std::string &name
) const {
	std::unordered_map< std::string, std::shared_ptr< BppImage > >::const_iterator
		iter = arc.find(name);
	if (iter != arc.end()) {
		return iter->second;
	}
	DUDS_THROW_EXCEPTION(ImageNotFoundError() << ImageArchiveImageName(name));
}

std::shared_ptr<BppImage> BppImageArchive::tryGet(
	const std::string &name
) const {
	std::unordered_map< std::string, std::shared_ptr< BppImage > >::const_iterator
		iter = arc.find(name);
	if (iter != arc.end()) {
		return iter->second;
	}
	return std::shared_ptr<BppImage>();
}

} } }
