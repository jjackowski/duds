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

constexpr DigitalPinConfig::Flags DigitalPinConfig::DirInput;
constexpr DigitalPinConfig::Flags DigitalPinConfig::DirOutput;
constexpr DigitalPinConfig::Flags DigitalPinConfig::DirImmaterial;
constexpr DigitalPinConfig::Flags DigitalPinConfig::DirNoChange;
constexpr DigitalPinConfig::Flags DigitalPinConfig::DirMask;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputNoPull;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputPulldown;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputPullup;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputPullImmaterial;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputPullNoChange;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InputPullMask;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventNone;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventEdgeFalling;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventEdgeRising;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventEdge;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventLevelLow;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventLevelHigh;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventImmaterial;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventNoChange;
constexpr DigitalPinConfig::Flags DigitalPinConfig::EventMask;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InterruptNone;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InterruptOnEvent;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InterruptImmaterial;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InterruptNoChange;
constexpr DigitalPinConfig::Flags DigitalPinConfig::InterruptMask;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputDriveLow;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputDriveHigh;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputPushPull;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputHighImpedance;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputImmaterial;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputNoChange;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OutputMask;
constexpr DigitalPinConfig::Flags DigitalPinConfig::OperationNoChange;

void DigitalPinConfig::checkValidity() const {
	int chk = 0;
	if (options & DirInput) ++chk;
	if (options & DirOutput) ++chk;
	if (options & DirImmaterial) ++chk;
	if (chk > 1) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingDirectionError());
	}
	chk = 0;
	if (options & InputNoPull) ++chk;
	if (options & InputPulldown) ++chk;
	if (options & InputPullup) ++chk;
	if (options & InputPullImmaterial) ++chk;
	if (chk > 1) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingPullError());
	}
	chk = 0;
	if (options & EventNone) ++chk;
	if (options & EventEdge) ++chk;
	if (options & EventLevelLow) ++chk;
	if (options & EventLevelHigh) ++chk;
	if (options & EventImmaterial) ++chk;
	if (chk > 1) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingEventError());
	}
	chk = 0;
	if (options & InterruptNone) ++chk;
	if (options & InterruptOnEvent) ++chk;
	if (options & InterruptImmaterial) ++chk;
	if (chk > 1) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingInterruptError());
	}
	chk = 0;
	if (options & OutputPushPull) ++chk;
	if (options & OutputHighImpedance) ++chk;
	if (options & OutputImmaterial) ++chk;
	if (chk > 1) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingOutputError());
	}
	if ((minOutputCurrent > 0) && (maxOutputCurrent > 0) &&
	(minOutputCurrent > maxOutputCurrent)) {
		BOOST_THROW_EXCEPTION(DigitalPinConflictingCurrentError());
	}
}

void DigitalPinConfig::combine(const DigitalPinConfig &newCfg) {
	// change all immaterial bits at once
	options.setMasked(newCfg.options,
		DirImmaterial | InputPullImmaterial | EventImmaterial | OutputImmaterial
	);
	// pin direction options
	if (!(newCfg.options & DirImmaterial) &&
	((newCfg.options & DirMask) != DirNoChange)) {
		options.setMasked(newCfg.options, DirMask);
	}
	// input pull up/down options
	if (!(newCfg.options & InputPullImmaterial) &&
	((newCfg.options & InputPullMask) != InputPullNoChange)) {
		options.setMasked(newCfg.options, InputPullMask);
	}
	// event options
	if (!(newCfg.options & InterruptImmaterial) &&
	((newCfg.options & InterruptMask) != InterruptNoChange)) {
		options.setMasked(newCfg.options, InterruptMask);
	}
	// interrupt options
	if (!(newCfg.options & EventImmaterial) &&
	((newCfg.options & EventMask) != EventNoChange)) {
		options.setMasked(newCfg.options, EventMask);
	}
	// output options
	if (!(newCfg.options & OutputImmaterial) &&
	((newCfg.options & OutputMask) != OutputNoChange)) {
		options.setMasked(newCfg.options, OutputMask);
	}
	// current requests
	if (newCfg.minOutputCurrent) {
		minOutputCurrent = newCfg.minOutputCurrent;
	}
	if (newCfg.maxOutputCurrent) {
		maxOutputCurrent = newCfg.maxOutputCurrent;
	}
}

DigitalPinConfig DigitalPinConfig::combine(
	const DigitalPinConfig &oldCfg,
	const DigitalPinConfig &newCfg
) {
	DigitalPinConfig res(oldCfg);
	res.combine(newCfg);
	return res;
}

DigitalPinRejectedConfiguration::Reason DigitalPinConfig::compatible(
	const DigitalPinCap &cap
) const {
	return cap.compatible(*this);
}

} } }
