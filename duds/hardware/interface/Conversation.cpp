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

namespace duds { namespace hardware { namespace interface {

/*
Conversation::Conversation(const Conversation &msg) {
	PartVector::const_iterator iter = msg.parts.cbegin();
	std::for_each(iter, msg.parts.cend(), [this](auto uptr) {
		// copy() makes copy of part; but how with the types?
		data.push_back(copy(uptr));
	}
}

Conversation::Conversation(const Conversation &msg, CopyExtractible) {
}

Conversation::Conversation(Conversation &msg, MoveExtractible) {
}
*/

ConversationVector &Conversation::addOutputVector() {
	ConversationVector *cv =
		new ConversationVector(ConversationVector::Output());
	parts.emplace_back(cv);
	return *cv;
}

ConversationVector &Conversation::addInputVector(std::size_t len) {
	ConversationVector *cv =
		new ConversationVector(len, ConversationVector::FixedLength());
	parts.emplace_back(cv);
	return *cv;
}

ConversationExternal &Conversation::addOutputBuffer(
	const char *a,
	std::size_t len
) {
	ConversationExternal *ce = new ConversationExternal(a, len);
	parts.emplace_back(ce);
	return *ce;
}

ConversationExternal &Conversation::addInputBuffer(char *a, std::size_t len) {
	ConversationExternal *ce = new ConversationExternal(a, len);
	parts.emplace_back(ce);
	return *ce;
}

ConversationExtractor Conversation::extract() const {
	return ConversationExtractor(*this);
}

} } }