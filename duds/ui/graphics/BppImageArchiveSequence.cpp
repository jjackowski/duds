/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppImageErrors.hpp>
#include <duds/ui/graphics/BppImageArchiveSequence.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

BppImageArchiveSequence::iterator BppImageArchiveSequence::begin() {
	// prepare the iterator
	iterator iter(this);
	return ++iter;
}

BppImageArchiveSequence::iterator &BppImageArchiveSequence::iterator::operator++() {
	// past end check
	if (!bias) {
		DUDS_THROW_EXCEPTION(ImageArchivePastEndError());
	}
	std::istream *is = bias->is;
	// read in name length
	unsigned char nlen;
	is->read((char*)&nlen, 1);
	// read in the name
	std::vector<char> buff(nlen);
	is->read(&(buff[0]), nlen);
	std::string name(buff.begin(), buff.end());
	// EOF is only set after attempting to read more than the file holds, so
	// it may be set now, but is not an error condition
	if (is->eof()) {
		// clear out data
		bias->deref.first.clear();
		bias->deref.second.reset();
		bias = nullptr;
		return *this;
	}
	// read in the dimensions
	if (nlen < 4) {
		buff.resize(4);
	}
	is->read(&(buff[0]), 4);
	if (!is->good()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError() <<
			ImageArchiveImageName(name)
		);
	}
	std::int16_t w = buff[0] | (buff[1] << 8);
	std::int16_t h = buff[2] | (buff[3] << 8);
	// calculate size
	std::size_t imgwidth = w / 8 + ((w % 8) ? 1 : 0);
	std::size_t length = imgwidth * h + 4;
	// allocate space
	buff.resize(length);
	// read rest of image
	is->read(&(buff[4]), length - 4);
	if (!is->good() && !is->eof()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamTruncatedError() <<
			ImageArchiveImageName(name)
		);
	}
	bias->deref.first = std::move(name);
	// create image object
	bias->deref.second = std::make_shared<BppImage>(buff);
	return *this;
}

void BppImageArchiveSequence::readHeader() {
	// check for header
	std::string str;
	// do not read into the version value that follows the header
	is->width(4);
	(*is) >> str;
	is->width(0);
	if (str != "BPPI") {
		DUDS_THROW_EXCEPTION(ImageNotArchiveStreamError());
	}
	// read version
	char buff[4];
	is->read(buff, 4);
	if (!is->good()) {
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
}

BppImageArchiveFile::BppImageArchiveFile(const std::string &path) {
	inf.open(path);
	if (!inf.good()) {
		DUDS_THROW_EXCEPTION(ImageArchiveStreamError() <<
			ImageArchiveFileName(path)
		);
	}
	is = &inf;
	readHeader();
}

} } }
