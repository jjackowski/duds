/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinCap.hpp>

namespace duds { namespace hardware { namespace interface {

constexpr DigitalPinCap::Flags DigitalPinCap::Input;
constexpr DigitalPinCap::Flags DigitalPinCap::OutputPushPull;
constexpr DigitalPinCap::Flags DigitalPinCap::OutputDriveLow;
constexpr DigitalPinCap::Flags DigitalPinCap::OutputDriveHigh;
constexpr DigitalPinCap::Flags DigitalPinCap::OutputDriveMask;
constexpr DigitalPinCap::Flags DigitalPinCap::OutputHighImpedance;
constexpr DigitalPinCap::Flags DigitalPinCap::HasPulldown;
constexpr DigitalPinCap::Flags DigitalPinCap::ControllablePulldown;
constexpr DigitalPinCap::Flags DigitalPinCap::HasPullup;
constexpr DigitalPinCap::Flags DigitalPinCap::ControllablePullup;
constexpr DigitalPinCap::Flags DigitalPinCap::EventEdgeFalling;
constexpr DigitalPinCap::Flags DigitalPinCap::EventEdgeRising;
constexpr DigitalPinCap::Flags DigitalPinCap::EventEdgeChange;
constexpr DigitalPinCap::Flags DigitalPinCap::EventLevelLow;
constexpr DigitalPinCap::Flags DigitalPinCap::EventLevelHigh;
constexpr DigitalPinCap::Flags DigitalPinCap::InterruptOnEvent;

DigitalPinCap::Flags DigitalPinCap::firstOutputDriveFlag() const {
	for (
		Flags::bitsType check = OutputPushPull.flags();
		check != OutputHighImpedance.flags();
		check <<= 1
	) {
		if (check & capabilities.flags()) {
			return Flags(check);
		}
	}
	return Flags::Zero();
}

DigitalPinConfig::Flags DigitalPinCap::firstOutputDriveConfigFlags() const {
	if (capabilities & OutputPushPull) {
		return DigitalPinConfig::DirOutput | DigitalPinConfig::OutputPushPull;
	} else if (capabilities & OutputDriveLow) {
		return DigitalPinConfig::DirOutput | DigitalPinConfig::OutputDriveLow;
	} else if (capabilities & OutputDriveHigh) {
		return DigitalPinConfig::DirOutput | DigitalPinConfig::OutputDriveHigh;
	} else {
		return DigitalPinConfig::OutputNoChange;
	}
}

DigitalPinRejectedConfiguration::Reason DigitalPinCap::compatible(
	const DigitalPinConfig &cfg
) const {
	cfg.checkValidity();
	DigitalPinRejectedConfiguration::Reason err =
		DigitalPinRejectedConfiguration::NotRejected;
	if (  // direction
		((cfg & DigitalPinConfig::DirInput) &&
		(~capabilities & DigitalPinCap::Input)) ||
		((cfg & DigitalPinConfig::DirOutput) &&
		!(capabilities & (
			DigitalPinCap::OutputPushPull | DigitalPinCap::OutputDriveLow |
			DigitalPinCap::OutputDriveHigh | DigitalPinCap::OutputHighImpedance
		))
	)) {
		err |= DigitalPinRejectedConfiguration::UnsupportedDirection;
	}
	if (  // input pull(s) required
		((cfg & DigitalPinConfig::InputNoPull) &&
		(capabilities & (DigitalPinCap::HasPulldown |
			DigitalPinCap::HasPullup)
		) && (~capabilities & (DigitalPinCap::ControllablePulldown |
			DigitalPinCap::ControllablePullup)
		)) ||
		// pulldown
		((cfg & DigitalPinConfig::InputPulldown) &&
		(~capabilities & DigitalPinCap::HasPulldown)) ||
		// pullup
		((cfg & DigitalPinConfig::InputPullup) &&
		(~capabilities & DigitalPinCap::HasPullup))
	) {
		err |= DigitalPinRejectedConfiguration::UnsupportedInputPull;
	}
	if (  // events
		((cfg & DigitalPinConfig::EventEdgeFalling) &&
		(~capabilities & DigitalPinCap::EventEdgeFalling)) ||
		((cfg & DigitalPinConfig::EventEdgeRising) &&
		(~capabilities & DigitalPinCap::EventEdgeRising)) ||
		((cfg & DigitalPinConfig::EventLevelLow) &&
		(~capabilities & DigitalPinCap::EventLevelLow)) ||
		((cfg & DigitalPinConfig::EventLevelHigh) &&
		(~capabilities & DigitalPinCap::EventLevelHigh))
	) {
		err |= DigitalPinRejectedConfiguration::UnsupportedEvent;
	}
	if (  // interrupt
		((cfg & DigitalPinConfig::InterruptOnEvent) &&
		(~capabilities & DigitalPinCap::InterruptOnEvent))
	) {
		err |= DigitalPinRejectedConfiguration::UnsupportedInterrupt;
	}
	if (  // output
		(cfg.options.test(DigitalPinConfig::OutputPushPull) &&
		(~capabilities & DigitalPinCap::OutputPushPull)) ||
		(  // drive low, like open collector or open drain
			cfg.options.test(DigitalPinConfig::OutputDriveLow, DigitalPinConfig::OutputPushPull) &&
			(
				// no emulation
				(~capabilities & DigitalPinCap::OutputDriveLow) ||
				// emulation
				(!capabilities.test(
					DigitalPinCap::OutputPushPull | DigitalPinCap::Input
				))
			)
		) ||
		(  // drive high, like open emitter
			cfg.options.test(DigitalPinConfig::OutputDriveHigh, DigitalPinConfig::OutputPushPull) &&
			(
				// no emulation
				(~capabilities & DigitalPinCap::OutputDriveHigh) ||
				// emulation
				(!capabilities.test(
					DigitalPinCap::OutputPushPull | DigitalPinCap::Input
				))
			)
		) ||
		(  // high impedance
			(cfg & DigitalPinConfig::OutputHighImpedance) &&
			(~capabilities & (
				DigitalPinCap::OutputHighImpedance | DigitalPinCap::Input
			))
		) ||
		(maxOutputCurrent && (
			(cfg.minOutputCurrent &&
				(cfg.minOutputCurrent > maxOutputCurrent)
			) /*  rethink this  ||
			(cfg.maxOutputCurrent &&
				(cfg.maxOutputCurrent > maxOutputCurrent)
			) */
		))
	) {
		err |= DigitalPinRejectedConfiguration::UnsupportedOutput;
	}
	return err;
}

} } }
