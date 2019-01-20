/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/ui/graphics/BppImageArchive.hpp>
#include <duds/ui/graphics/BppImageErrors.hpp>
#include <duds/ui/graphics/BppImageArchiveSequence.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

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
	BppImageArchiveSequence bias(is);
	bias.readHeader();
	BppImageArchiveSequence::iterator iter = bias.begin();
	for (; iter != bias.end(); ++iter) {
		std::lock_guard<duds::general::Spinlock> lock(block);
		arc[iter->first] = iter->second;
	}
}

void BppImageArchive::add(const std::string &name, const BppImageSptr &img) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	arc[name] = img;
}

void BppImageArchive::add(const std::string &name, BppImageSptr &&img) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	arc[name] = std::move(img);
}

const BppImageSptr &BppImageArchive::get(const std::string &name) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	ImageMap::const_iterator
		iter = arc.find(name);
	if (iter != arc.end()) {
		return iter->second;
	}
	DUDS_THROW_EXCEPTION(ImageNotFoundError() << ImageArchiveImageName(name));
}

BppImageSptr BppImageArchive::tryGet(const std::string &name) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	ImageMap::const_iterator
		iter = arc.find(name);
	if (iter != arc.end()) {
		return iter->second;
	}
	return BppImageSptr();
}

} } }
