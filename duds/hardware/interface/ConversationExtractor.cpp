/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ConversationExtractor.hpp>
#include <cstring>  // for std::memcpy()

namespace duds { namespace hardware { namespace interface {

void ConversationExtractor::set() {
	if (!c) {
		DUDS_THROW_EXCEPTION(ConversationNotSet());
	}
	piter = c->cbegin();
	// first part for extraction
	while ((piter != c->cend() && !(*piter)->extract())) {
		++piter;
	}
	if (piter == c->cend()) {
		// set conversation end condition
		pos = nullptr;
		remain = 0;
	} else {
		// start at first part to extract
		pos = (*piter)->start();
		remain = (*piter)->length();
	}
}

void ConversationExtractor::advance(std::size_t bytes) {
	// too few bytes?
	if (bytes > remain) {
		DUDS_THROW_EXCEPTION(ConversationReadPastEnd());
	}
	remain -= bytes;
	// no data left in part?
	if (!remain) {
		// if used with a part rather than whole conversation . . .
		if (!c) {
			// set conversation end condition
			pos = nullptr;
			return;
		}
		// find next part for extraction
		do {
			++piter;
		} while ((piter != c->cend() && !(*piter)->extract()));
		// no more?
		if (piter == c->cend()) {
			// set conversation end condition
			pos = nullptr;
		} else {
			// start at next part
			pos = (*piter)->start();
			remain = (*piter)->length();
		}
	} else {
		// advance within the part
		pos += bytes;
	}
}

void ConversationExtractor::nextPart() {
	if (!c) {
		DUDS_THROW_EXCEPTION(ConversationNotSet());
	}
	if (!pos) {
		DUDS_THROW_EXCEPTION(ConversationReadPastEnd());
	}
	// find next part for extraction
	while ((piter != c->cend() && !(*piter)->extract())) {
		++piter;
	}
	if (piter == c->cend()) {
		// set conversation end condition
		pos = nullptr;
		remain = 0;
	}
	// start at next part
	pos = (*piter)->start();
	remain = (*piter)->length();
}

void ConversationExtractor::read(char *dest, std::size_t len) {
	if (remain < len) {
		DUDS_THROW_EXCEPTION(ConversationReadPastEnd());
	}
	std::memcpy(dest, pos, len);
	advance(len);
}

} } }
