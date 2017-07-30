/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <boost/uuid/uuid.hpp>

namespace duds { namespace hardware {

/**
 * Represents a hardware part; typically an item that can be purchased and
 * added to a circut. It does not represnt a specific tangible item. Instead,
 * it represents a model of an item that can be found from a supplier.
 *
 * @todo  One day, read these from a database on demand.
 *
 * @todo  Support adding information, maybe in derived classes, that describe
 *        the part. Things like common factory calibration, accuracy, and
 *        precision data for various sensors.
 */
class PartModel {
	boost::uuids::uuid pid;
	std::string partname;
protected:
	PartModel() = default;  // serialization
	PartModel(const boost::uuids::uuid &id, const std::string &name) :
		pid(id), partname(name) { }
public:
	const boost::uuids::uuid id() const {
		return pid;
	}
	const std::string &name() const {
		return partname;
	}
};

} }

// the rest of this file suggests the direction of future development

#if 0

/**
 * Represents something that is a specific tangible part. This is intended to
 * be added to the hierarchy of a class that derives from Something. It allows
 * the history of a particular component to be tracked, and provides a way to
 * get the PartModel object for general information and for initial calibration
 * data.
 */
class PartInstance {
	boost::uuids::uuid partid;
public:
	const boost::uuids::uuid &partId() const {
		return partid;
	}
};

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>

struct PartIndex_UUID { };
struct PartIndex_Name { };

typedef boost::multi_index::multi_index_container<
	PartModel,
	boost::multi_index::indexed_by<
		boost::multi_index::hashed_unique<
			boost::multi_index::tag<PartIndex_UUID>,
			boost::multi_index::const_mem_fun<
				PartModel, boost::uuids::uuid, &PartModel::id
			>
		>,
		boost::multi_index::hashed_non_unique<
			boost::multi_index::tag<PartIndex_Name>,
			boost::multi_index::const_mem_fun<
				PartModel, std::string, &PartModel::name
			>
		>
	>
> PartModelContainer;

/**
 * A searchable library of parts.
 * @todo  Consider making this an interface with a derived class that does
 *        everything, and another that uses an external database.
 */
class PartLibrary {
	PartModelContainer parts;  // clonus horror
public:
	const PartModel& part(const boost::uuids::uuid &id) const;
};

#endif

