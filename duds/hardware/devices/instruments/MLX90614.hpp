/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/Instrument.hpp>

namespace duds { namespace hardware { namespace devices {

// namespace for the device?

// something for making/managing Instrument objects?
// maybe some Device object to represent hardware? it could manage the
// instruments and adapters

class MLX90614_Instrument : public Instrument {
	// needed?
};

// other files?

// one driver; two or three instruments
class MLX90614_Driver : public InstrumentDriver {
	int nir;
public:
	/**
	 * @param numIr   The number of IR temperature sensors. Only the values
	 *                1 and 2 are valid.  Can this be queried from the device?
	 */
	MLX90614_Driver(int numIr);
	void setAdapter(const std::shared_ptr<Adapter> &adp);
	void sample(ClockDriver &clock);
};

class MLX90614 : public Device {
public:
	MLX90614();
	/**
	 * @param numIr   The number of IR temperature sensors. Only the values
	 *                1 and 2 are valid.  Can this be queried from the device?
	 */
	MLX90614(int numIr);
};

} } }

