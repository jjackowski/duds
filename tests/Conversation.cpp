/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * Test of the duds::hardware::interface::Conversation and releated classes.
 */
#include <duds/hardware/interface/ConversationExtractor.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

namespace dhi = duds::hardware::interface;

BOOST_AUTO_TEST_SUITE(Conversation)

BOOST_AUTO_TEST_CASE(Conversation_Vec) {
	dhi::Conversation con;
	dhi::ConversationVector &cvo = con.addOutputVector();
	BOOST_CHECK(cvo.output());
	BOOST_CHECK(!cvo.extract());
	std::size_t i = 4;
	cvo.addBe(i);
	BOOST_CHECK_EQUAL(cvo.length(), sizeof(i));
	// must be big endian
	BOOST_CHECK_EQUAL(*cvo.start(), 0);
	BOOST_CHECK_EQUAL(*(cvo.start() + sizeof(i) - 1), 4);
	cvo << i;
	BOOST_CHECK_EQUAL(cvo.length(), sizeof(i) * 2);
	// must be little endian
	BOOST_CHECK_EQUAL(*(cvo.start() + sizeof(i)), 4);
	BOOST_CHECK_EQUAL(*(cvo.start() + sizeof(i) * 2 - 1), 0);
	// add an input
	dhi::ConversationVector &cvi = con.addInputVector(sizeof(i) * 2);
	BOOST_CHECK(cvi.input());
	BOOST_CHECK(cvi.extract());
	BOOST_CHECK_EQUAL(cvi.length(), sizeof(i) * 2);
	// cannot add to input
	BOOST_CHECK_THROW(cvi.addBe(i), dhi::ConversationBadAdd);
	BOOST_CHECK_THROW(cvi << i, dhi::ConversationBadAdd);
	BOOST_CHECK_EQUAL(cvi.length(), sizeof(i) * 2);  // length unchanged
	// set input to output
	std::memcpy(cvi.start(), cvo.start(), cvi.length());
	// extract input
	dhi::ConversationExtractor ce = con.extract();
	ce.readBe(i);
	BOOST_CHECK_EQUAL(i, 4);
	ce >> i;
	BOOST_CHECK_EQUAL(i, 4);
	BOOST_CHECK_EQUAL(ce.end(), true);
	BOOST_CHECK_EQUAL(ce.remaining(), 0);
	BOOST_CHECK_THROW(ce.readLe(i), dhi::ConversationReadPastEnd);
	// internal condition checked for past end different from above
	BOOST_CHECK_THROW(ce.read(i), dhi::ConversationReadPastEnd);
	BOOST_CHECK_THROW(ce.nextPart(), dhi::ConversationReadPastEnd);
}

BOOST_AUTO_TEST_CASE(Conversation_Ext) {
	// configure a couple of buffers
	const char buffout[16] = { 0, 1, (char)-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	char buffin[8];
	dhi::Conversation con;
	BOOST_CHECK(con.empty());
	std::unique_ptr<dhi::ConversationExternal> cout(
		new dhi::ConversationExternal(buffout)
	);
	BOOST_CHECK(cout->output());
	BOOST_CHECK(!cout->extract());
	std::unique_ptr<dhi::ConversationExternal> cin(
		new dhi::ConversationExternal(buffin)
	);
	BOOST_CHECK(cin->input());
	BOOST_CHECK(!cin->extract()); // externals are not normally extracted
	con.add(cout);
	BOOST_CHECK(!cout);
	con.add(cin);
	BOOST_CHECK(!cin);
	BOOST_CHECK_EQUAL(con.size(), 2);
	// iterate over the parts
	dhi::Conversation::PartVector::iterator iter = con.begin();
	BOOST_CHECK_EQUAL((*iter)->start(), buffout);
	BOOST_CHECK_EQUAL((*iter)->length(), 16);
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), buffin);
	BOOST_CHECK_EQUAL((*iter)->length(), 8);
}

/* Make 4-part conversation, 2 each way, with one variable length input.
   Simulate using it twice without modifying output data. */
BOOST_AUTO_TEST_CASE(Conversation_I2Clike) {
	dhi::Conversation con;
	// fill with 4 parts, alternate between output and input, start with output
	dhi::ConversationVector &cvo = con.addOutputVector();
	BOOST_CHECK(cvo.output());
	BOOST_CHECK(!cvo.extract());
	BOOST_CHECK(!cvo.varyingLength());
	BOOST_CHECK_THROW(cvo.setStartOffset(2), dhi::ConversationFixedLength);
	cvo << std::uint16_t(0xF012) << std::uint16_t(1);
	BOOST_CHECK_EQUAL(cvo.length(), 4);
	std::uint16_t buffin[2];
	dhi::ConversationExternal &ain = con.addInputBuffer(buffin);
	BOOST_CHECK(ain.input());
	BOOST_CHECK(!ain.extract());
	BOOST_CHECK(!ain.varyingLength());
	const std::uint32_t buffout[2] = { 0xAA550011, 0x12345678 };
	dhi::ConversationExternal &aout = con.addOutputBuffer(buffout);
	BOOST_CHECK(aout.output());
	BOOST_CHECK(!aout.extract());
	BOOST_CHECK(!aout.varyingLength());
	dhi::ConversationVector &cvi = con.addInputVector(
		32, dhi::ConversationVector::VaribleLength()
	);
	BOOST_CHECK_EQUAL(cvi.length(), 32);
	BOOST_CHECK(cvi.input());
	BOOST_CHECK(cvi.extract());
	BOOST_CHECK(cvi.varyingLength());
	BOOST_CHECK_EQUAL(con.size(), 4);
	// inspect each part and write out test input data
	dhi::Conversation::PartVector::iterator iter = con.begin();
	// 1st item
	BOOST_CHECK_EQUAL((*iter)->start(), cvo.start());
	BOOST_CHECK_EQUAL((*iter)->length(), cvo.length());
	dhi::ConversationExtractor extr(*(*iter));
	BOOST_CHECK(!extr.end());
	BOOST_CHECK_EQUAL(cvo.length(), extr.remaining());
	std::uint16_t out16;
	BOOST_CHECK_NO_THROW(extr.readLe(out16));
	BOOST_CHECK_EQUAL(out16, 0xF012);
	BOOST_CHECK_NO_THROW(extr.readLe(out16));
	BOOST_CHECK_EQUAL(out16, 1);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
	// 2nd item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), ain.start());
	BOOST_CHECK_EQUAL((*iter)->start(), (char*)buffin);
	BOOST_CHECK_EQUAL((*iter)->length(), ain.length());
	// 3rd item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), aout.start());
	BOOST_CHECK_EQUAL((*iter)->length(), aout.length());
	extr.reset(*(*iter));
	BOOST_CHECK(!extr.end());
	BOOST_CHECK_EQUAL(aout.length(), extr.remaining());
	BOOST_CHECK_EQUAL(extr.remaining(), sizeof(buffout));
	std::uint32_t out32;
	BOOST_CHECK_NO_THROW(extr.readLe(out32));
	BOOST_CHECK_EQUAL(out32, buffout[0]);
	BOOST_CHECK_NO_THROW(extr.readLe(out32));
	BOOST_CHECK_EQUAL(out32, buffout[1]);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
	// 4th item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), cvi.start());
	BOOST_CHECK_EQUAL((*iter)->length(), cvi.length());
	BOOST_CHECK((*iter)->varyingLength());
	dhi::ConversationVector *cv = dynamic_cast<dhi::ConversationVector*>(iter->get());
	BOOST_CHECK(cv->varyingLength());
	BOOST_CHECK_THROW(cv->setStartOffset(64), dhi::ConversationBadOffset);
	// simulate write
	BOOST_CHECK_NO_THROW(cv->setStartOffset(2));
	char *outp = cv->start() - 2;
	outp[0] = (char)sizeof(buffout);
	outp[1] = 0;
	int c = 0;
	for (char *r = (char*)buffout; c < sizeof(buffout); ++c, ++r) {
		outp[c+2] = *r;
	}
	BOOST_CHECK_NO_THROW(cv->setLength(sizeof(buffout) + 2));
	// end
	++iter;
	BOOST_CHECK(iter == con.end());
	// attempt to read back data
	extr.reset(con);
	BOOST_CHECK_EQUAL(extr.remaining(), sizeof(buffout));
	extr >> out32;
	BOOST_CHECK_EQUAL(out32, buffout[0]);
	extr >> out32;
	BOOST_CHECK_EQUAL(out32, buffout[1]);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
	// re-use conversation; re-check contents, but do not write until last part
	iter = con.begin();
	// 1st item
	BOOST_CHECK_EQUAL((*iter)->start(), cvo.start());
	BOOST_CHECK_EQUAL((*iter)->length(), cvo.length());
	extr.reset(*(*iter));
	BOOST_CHECK(!extr.end());
	BOOST_CHECK_EQUAL(cvo.length(), extr.remaining());
	BOOST_CHECK_NO_THROW(extr.readLe(out16));
	BOOST_CHECK_EQUAL(out16, 0xF012);
	BOOST_CHECK_NO_THROW(extr.readLe(out16));
	BOOST_CHECK_EQUAL(out16, 1);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
	// 2nd item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), ain.start());
	BOOST_CHECK_EQUAL((*iter)->start(), (char*)buffin);
	BOOST_CHECK_EQUAL((*iter)->length(), ain.length());
	// 3rd item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), aout.start());
	BOOST_CHECK_EQUAL((*iter)->length(), aout.length());
	extr.reset(*(*iter));
	BOOST_CHECK(!extr.end());
	BOOST_CHECK_EQUAL(aout.length(), extr.remaining());
	BOOST_CHECK_EQUAL(extr.remaining(), sizeof(buffout));
	BOOST_CHECK_NO_THROW(extr.readLe(out32));
	BOOST_CHECK_EQUAL(out32, buffout[0]);
	BOOST_CHECK_NO_THROW(extr.readLe(out32));
	BOOST_CHECK_EQUAL(out32, buffout[1]);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
	// 4th item
	++iter;
	BOOST_CHECK_EQUAL((*iter)->start(), cvi.start());
	BOOST_CHECK_EQUAL((*iter)->length(), cvi.length());
	BOOST_CHECK((*iter)->varyingLength());
	cv = dynamic_cast<dhi::ConversationVector*>(iter->get());
	BOOST_CHECK(cv->varyingLength());
	BOOST_CHECK_THROW(cv->setStartOffset(64), dhi::ConversationBadOffset);
	// simulate second write
	BOOST_CHECK_NO_THROW(cv->setStartOffset(2));
	outp = cv->start() - 2;
	outp[0] = (char)sizeof(buffout);
	outp[1] = 0;
	c = 0;
	// copy in reverse order from previous time
	for (char *r = ((char*)buffout) + sizeof(buffout) - 1; c < sizeof(buffout); ++c, --r) {
		outp[c+2] = *r;
	}
	BOOST_CHECK_NO_THROW(cv->setLength(sizeof(buffout) + 2));
	// end
	++iter;
	BOOST_CHECK(iter == con.end());
	// attempt to read back data
	extr.reset(con);
	BOOST_CHECK_EQUAL(extr.remaining(), sizeof(buffout));
	// in reverse, so elements backwards and endianess is different
	extr.readBe(out32);
	BOOST_CHECK_EQUAL(out32, buffout[1]);
	extr.readBe(out32);
	BOOST_CHECK_EQUAL(out32, buffout[0]);
	BOOST_CHECK_EQUAL(extr.remaining(), 0);
	BOOST_CHECK(extr.end());
}

BOOST_AUTO_TEST_SUITE_END()
