/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
/**
 * @file
 * Header for Conversarion; includes ConversationVector.hpp and
 * ConversationExternal.hpp. If the extractor is needed, then just include
 * ConversationExtractor.hpp.
 */
#ifndef CONVERSATION_HPP
#define CONVERSATION_HPP

#include <memory>
#include <duds/hardware/interface/ConversationVector.hpp>
#include <duds/hardware/interface/ConversationExternal.hpp>

namespace duds { namespace hardware { namespace interface {

class ConversationExtractor;

/**
 * An attribute for errors when using Conversation objects that references the
 * ConversationPart by index.
 */
typedef boost::error_info<struct Info_ConversationPartIndex, int>
	ConversationPartIndex;

/**
 * Represents a two-way conversation with a device. Output data is written
 * prior to transmission, and input space is pre-allocated. The data is held
 * in ConversationPart objects; there is no hard limit on the number of parts
 * that can be added.
 *
 * A Conversation can be used multiple times. Each time, input data will be
 * overwritten. Output data can be reused rather than rewritten.
 *
 * After using a Conversation for a transmission, the input is read using a
 * ConversationExtractor object. It references the data held in the
 * Conversation object rather than copy it, so the source Conversation must
 * not be modified while the ConversationExtractor object is in use. The
 * ConversationExtractor can outlive the Conversation, but it must not be used
 * to read data from a destructed Conversation.
 *
 * @author  Jeff Jackowski
 */
class Conversation {
public:
	/**
	 * The storage type for the ConversationPart objects.
	 */
	typedef std::vector< std::unique_ptr<ConversationPart> >  PartVector;
protected:
	/**
	 * The container of the parts that make up the full conversation.
	 */
	PartVector parts;  // the clonus horror
public:
	//struct CopyAll { };
	//struct CopyExtractible { };
	//struct MoveExtractible { };
	Conversation() = default;
	//Conversation(const Conversation &msg);
	//Conversation(const Conversation &msg, CopyAll) : Conversation(msg) { }
	/*
	 * Creates a copy of the given Conversation that only includes the
	 * ConversationPart objects flagged as extractible.
	 * @sa ConversationPart::MpfExtract
	 * @param msg  The source of the copy.
	 */
	//Conversation(const Conversation &msg, CopyExtractible);
	/*
	 * Creates a new Conversation that only includes ConversationPart objects
	 * flagged as extractible. The parts are moved from the original. They
	 * are replaced in @a msg by new ConversationVector objects with identical
	 * flags and lengths. This can be used to extract data on one thread while
	 * reusing the original Conversation to gather more data.
	 * @sa ConversationPart::MpfExtract
	 * @param msg  The source of the copy.
	 */
	//Conversation(Conversation &msg, MoveExtractible);
	/**
	 * An iterator to the first ConversationPart that allows modification.
	 */
	PartVector::iterator begin() {
		return parts.begin();
	}
	/**
	 * An iterator to the end of ConversationPart vector.
	 */
	PartVector::iterator end() {
		return parts.end();
	}
	/**
	 * An iterator to the first ConversationPart that does not
	 * allow modification.
	 */
	PartVector::const_iterator cbegin() const {
		return parts.cbegin();
	}
	/**
	 * An iterator to the end of ConversationPart vector.
	 */
	PartVector::const_iterator cend() const {
		return parts.cend();
	}
	/**
	 * Returns the number of parts within this conversation.
	 */
	std::size_t size() const {
		return parts.size();
	}
	/**
	 * Returns true if the conversation has no parts.
	 */
	bool empty() const {
		return parts.empty();
	}
	/**
	 * Makes the conversation empty.
	 */
	void clear() {
		parts.clear();
	}
	/**
	 * Adds an already constructed conversation part to the end of the
	 * conversation.
	 * @tparam CP  The class of the conversation part.
	 * @param  cp  The conversation part to add. The Conversation object will
	 *             assume responsibility for the part.
	 * @post       @a cp will no longer hold the part object.
	 */
	template <class CP>
	void add(std::unique_ptr<CP> &cp) {
		parts.emplace_back(std::move(cp));
	}
	/**
	 * Adds a copy of an existing conversation part to the end of the
	 * conversation.
	 * @tparam CP  The class of the conversation part. It must be derived from
	 *             ConversationPart.
	 * @param  cp  The conversation part to copy.
	 * @return     The added conversation part.
	 */
	template <class CP>
	CP &add(const CP &cp) {
		CP *np = new CP(cp);
		parts.emplace_back(np);
		return *np;
	}
	/**
	 * Creates a new ConversationVector for output and returns it for
	 * modification.
	 */
	ConversationVector &addOutputVector();
	/**
	 * Creates a new ConversationVector for fixed length input and initializes
	 * it with the given length.
	 * @param len  The length of the input in bytes.
	 */
	ConversationVector &addInputVector(std::size_t len);
	/**
	 * Creates a new ConversationVector for fixed or variable length input and
	 * initializes it with the given length.
	 * @tparam LengthType  Either ConversationVector::FixedLength or
	 *                     ConversationVector::VaribleLength.
	 * @param len  The length of the input in bytes.
	 * @param lt   The length type object.
	 */
	template <class LengthType>
	ConversationVector &addInputVector(std::size_t len, LengthType lt) {
		ConversationVector *cv = new ConversationVector(len, lt);
		parts.emplace_back(cv);
		return *cv;
	}
	/**
	 * Adds a conversation part that will use the given buffer for output.
	 * @param a    The start of the buffer.
	 * @param len  The length of the buffer in bytes.
	 */
	ConversationExternal &addOutputBuffer(const char *a, std::size_t len);
	/**
	 * Adds a conversation part that will use the given array for output.
	 * @tparam T  The element type of the array.
	 * @tparam N  The length of the array in elements of @a T.
	 * @param  a  The array.
	 */
	template <typename T, std::size_t N>
	ConversationExternal &addOutputBuffer(const T (&a)[N]) {
		ConversationExternal *ce = new ConversationExternal(a);
		parts.emplace_back(ce);
		return *ce;
	}
	/**
	 * Adds a conversation part that will write input into the given buffer.
	 * @param a    The start of the buffer.
	 * @param len  The length of the buffer in bytes. The current
	 *             implementation requires this length to be fixed rather than
	 *             the maximum amount of a variable length input.
	 */
	ConversationExternal &addInputBuffer(char *a, std::size_t len);
	/**
	 * Adds a conversation part that will write input into the given array.
	 * @tparam T  The element type of the array.
	 * @tparam N  The length of the buffer in elements of @a T. The current
	 *            implementation requires this length to be fixed rather than
	 *            the maximum amount of a variable length input.
	 * @param a   The array.
	 */
	template <typename T, std::size_t N>
	ConversationExternal &addInputBuffer(T (&a)[N]) {
		ConversationExternal *ce = new ConversationExternal(a);
		parts.emplace_back(ce);
		return *ce;
	}
	/**
	 * Returns an extraction object that can be used to read all the
	 * conversation data marked extractible.
	 * @post  This Conversation object is not changed until the returned object
	 *        is no longer used to read from this conversation.
	 */
	ConversationExtractor extract() const;
};

} } }

#endif        //  #ifndef CONVERSATION_HPP
