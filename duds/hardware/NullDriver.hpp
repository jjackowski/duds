/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/InstrumentDriver.hpp>
#include <vector>

namespace duds { namespace hardware {

class NullDriver : public InstrumentDriver { };  // functions do nothing

class NullMultiDriver : public InstrumentDriver {
	std::vector< shared_ptr< InstrumentAdapter > >  adapters;
public:
	NullMultiDriver() = default;
	NullMultiDriver(size_t numadapt) {
		adapters.reserve(numadapt);
	}
	virtual void setAdapter(const shared_ptr<InstrumentAdapter> &adp) {
		adapters.push(adp);
	}
	void clearAdapters() {
		adapters.clear();
	}
};

class RemoteDriverClient : public InstrumentDriver { };
class RemoteDriverServer : public InstrumentDriver { };

} }
