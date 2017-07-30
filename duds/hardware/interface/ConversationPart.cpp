/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/ConversationPart.hpp>

namespace duds { namespace hardware { namespace interface {

constexpr ConversationPart::Flags ConversationPart::MpfInput;
constexpr ConversationPart::Flags ConversationPart::MpfExtract;
constexpr ConversationPart::Flags ConversationPart::MpfVarlen;
constexpr ConversationPart::Flags ConversationPart::MpfBigendian;
constexpr ConversationPart::Flags ConversationPart::MpfBreak;

ConversationPart::~ConversationPart() { }

} } }
