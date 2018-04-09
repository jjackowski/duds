/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ConversationVector.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace interface {

void ConversationVector::add(char i) {
	if (input()) {
		throw ConversationBadAdd();
	}
	data.push_back(i);
}

void ConversationVector::add(const std::string &str) {
	if (input()) {
		throw ConversationBadAdd();
	}
	data.insert(data.end(), str.begin(), str.end());
}

void ConversationVector::add(std::int8_t *a, std::size_t l) {
	if (input()) {
		throw ConversationBadAdd();
	}
	data.insert(data.end(), a, a + l);
}

void ConversationVector::setStartOffset(std::int16_t offset) {
	if (!varyingLength()) {
		DUDS_THROW_EXCEPTION(ConversationFixedLength());
	}
	if ((offset < 0) || (offset > data.size())) {
		DUDS_THROW_EXCEPTION(ConversationBadOffset());
	}
	val16 = offset;
}

void ConversationVector::setLength(std::size_t len) {
	if (varyingLength()) {
		data.resize(len);
	} else {
		DUDS_THROW_EXCEPTION(ConversationFixedLength());
	}
}

void ConversationVector::reserve(std::size_t len) {
	data.reserve(len + val16);
}

char *ConversationVector::start() const {
	return const_cast<char*>(&(data[val16]));
}

std::size_t ConversationVector::length() const {
	return data.size() - val16;
}

} } }
