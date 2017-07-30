/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/interface/DigitalPinConfig.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Defines the capabilites of a digital general purpose I/O pin.
 *
 * The reported capabilities do not consider emulating any behaviors because
 * emulation can lead to bad behavior under certain conditions. For instance,
 * configuring a pin as an input to get open collector or high impedance
 * behavior allows the possibility that the input will change its state
 * rapidly wich can cause the input buffer to draw enough current to damage
 * itself. This may not be a problem depending upon the circuit. The
 * application software must have correct information about the capabilities
 * so that if it is a problem it can report an error.
 *
 * DigitalPort implementations may emulate behaviors not directly supported by
 * the hardware.
 *
 * @author  Jeff Jackowski
 */
struct DigitalPinCap {
	/**
	 * A container of flags that control the operation of a digital pin. The
	 * flags are intended to cover a wide range of hardware, so not all options
	 * may be available on specific hardware.
	 */
	typedef duds::general::BitFlags<struct DigitalPinFlags, std::uint16_t>
		Flags;
	/**
	 * Input operation is supported.
	 */
	static constexpr Flags Input = Flags::Bit(0);
	/**
	 * The output can drive the line either low or high. This is distinct from
	 * the combination of the OutputDriveLow and OutputDriveHigh flags since
	 * many outputs are a push-pull type and not an open collector, drain, or
	 * emitter.
	 */
	static constexpr Flags OutputPushPull = Flags::Bit(1);
	/**
	 * The output is or can be an open collector or open drain type. This type
	 * drives the line low, but not high.
	 * @note  There is a difference in the electronics of open collector and
	 *        open drain outputs, but so far as I know there is no difference
	 *        to how software would use them, so I used the same flag for both.
	 */
	static constexpr Flags OutputDriveLow = Flags::Bit(2);
	/**
	 * The output is or can be an open emitter type. This type drives the line
	 * high, but not low.
	 */
	static constexpr Flags OutputDriveHigh = Flags::Bit(3);
	/**
	 * A mask of all output flags that involve driving the line.
	 */
	static constexpr Flags OutputDriveMask =
		OutputPushPull | OutputDriveLow | OutputDriveHigh;
	/**
	 * The pin supports a high impedance state without input.
	 */
	static constexpr Flags OutputHighImpedance = Flags::Bit(4);
	/**
	 * The pin has a pull-down resistor.
	 */
	static constexpr Flags HasPulldown = Flags::Bit(5);
	/**
	 * The use of the pin's pull-down resistor can be controlled by software.
	 * @todo  Support this.
	 */
	static constexpr Flags ControllablePulldown = Flags::Bit(6);
	/**
	 * The pin has a pull-up resistor.
	 */
	static constexpr Flags HasPullup = Flags::Bit(7);
	/**
	 * The use of the pin's pull-up resistor can be controlled by software.
	 * @todo  Support this.
	 */
	static constexpr Flags ControllablePullup = Flags::Bit(8);
	/**
	 * The pin supports setting an event flag on the falling edge.
	 */
	static constexpr Flags EventEdgeFalling = Flags::Bit(9);
	/**
	 * The pin supports setting an event flag on the rising edge.
	 */
	static constexpr Flags EventEdgeRising = Flags::Bit(10);
	/**
	 * The pin supports setting an event flag on an edge change.
	 */
	static constexpr Flags EventEdgeChange = Flags::Bit(11);
	/**
	 * The pin supports setting an event flag on a low level.
	 */
	static constexpr Flags EventLevelLow = Flags::Bit(12);
	/**
	 * The pin supports setting an event flag on a high level.
	 */
	static constexpr Flags EventLevelHigh = Flags::Bit(13);
	/**
	 * The pin supports triggering an interrupt when an event occurs. If set,
	 * at least one of the event flags must also be set:
	 *  - EventEdgeFalling
	 *  - EventEdgeRising
	 *  - EventEdgeChange
	 *  - EventLevelLow
	 *  - EventLevelHigh
	 * This flag may be false even when the hardware supports the interrupt if
	 * the driver works directly with the hardware even though an operating
	 * system is in use that normally operates and limits access to the hardware.
	 */
	static constexpr Flags InterruptOnEvent = Flags::Bit(14);
	/**
	 * The capabilities of a digital pin.
	 */
	Flags capabilities;
	/**
	 * The maximum output current in milliamps (?) the pin can manage, or zero
	 * if unspecified or not applicable.
	 */
	std::uint16_t maxOutputCurrent;
	/**
	 * Construct uninitialized.
	 */
	DigitalPinCap() = default;
	/**
	 * Construct fully initiallized.
	 * @param cap  The capability flags.
	 * @param cur  The maximum output current.
	 */
	constexpr DigitalPinCap(const Flags cap, std::uint16_t cur = 0) :
		capabilities(cap),
		maxOutputCurrent(cur)
	{ }
	/**
	 * Returns true if the pin exists and is usable by this process. If false,
	 * the pin may or may not actually exist, but the difference will be
	 * immaterial to this process. The result does not mean that the pin is
	 * not currently in use.
	 */
	constexpr bool exists() const {
		return (capabilities & (
			Input | OutputPushPull | OutputDriveHigh | OutputDriveLow |
			ControllablePulldown | ControllablePullup | InterruptOnEvent
		)) != Flags::Zero();
	}
	/**
	 * Returns true if the port is capable of output. This does not imply a
	 * specific high impedence state.
	 */
	constexpr bool canOutput() const {
		return (capabilities & OutputDriveMask) != Flags::Zero();
	}
	/**
	 * Checks the flags in OutputDriveMask, starting with OutputPushPull, and
	 * returns the first match found in @a capabilities. If no match is found,
	 * returns Flags::Zero().
	 */
	Flags firstOutputDriveFlag() const;
	/**
	 * Returns the output configuration flags that corresponds to the result of
	 * firstOutputDriveFlag(). If no driving output is available, the result
	 * is DigitalPinConfig::OutputNoChange. Otherwise, the flag
	 * DigitalPinConfig::DirOutput is included along with the flags to specify
	 * driving low and/or high.
	 */
	DigitalPinConfig::Flags firstOutputDriveConfigFlags() const;
	/**
	 * Returns a set of flags that indicate certain incompatible conditions
	 * in the given pin configuration irrespective of any other pin. The only
	 * condition currently checked is whether or not the configuration is
	 * supported by the hardware.
	 * @param cfg  The configuration to check.
	 * @throw DigitalPinConfigurationError  The configuration disagrees with
	 *                                      itself from either having mutually
	 *                                      exclusive flags set, or a minimum
	 *                                      greater than a maximum. This is a
	 *                                      programming mistake that has nothing
	 *                                      to do with the pin capabilities.
	 * @return  Flags that are set for incompatible conditions. If all flags
	 *          are clear, the given configuration is compatible.
	 */
	DigitalPinRejectedConfiguration::Reason compatible(
		const DigitalPinConfig &cfg
	) const;
};

/**
 * The capabilities of a non-existent pin. The object's exists() function
 * will return false.
 */
constexpr DigitalPinCap NonexistentDigitalPin =
	DigitalPinCap(DigitalPinCap::Flags::Zero(), 0);


typedef boost::error_info<
	struct Info_DigitalPinCap, DigitalPinCap
> DigitalPinCapInfo;

inline std::ostream &operator<<(std::ostream &os, const DigitalPinCap &c) {
	return os << '(' << c.capabilities.flags() << ',' <<
	c.maxOutputCurrent << ')';
}

constexpr DigitalPinCap::Flags operator & (
	const DigitalPinCap &cap,
	const DigitalPinCap::Flags &flg
) {
	return cap.capabilities & flg;
}

constexpr DigitalPinCap::Flags operator & (
	const DigitalPinCap::Flags &flg,
	const DigitalPinCap &cap
) {
	return flg & cap.capabilities;
}

constexpr DigitalPinCap::Flags operator | (
	const DigitalPinCap &cap,
	const DigitalPinCap::Flags &flg
) {
	return cap.capabilities | flg;
}

constexpr DigitalPinCap::Flags operator | (
	const DigitalPinCap::Flags &flg,
	const DigitalPinCap &cap
) {
	return flg | cap.capabilities;
}

constexpr DigitalPinCap::Flags operator ^ (
	const DigitalPinCap &cap,
	const DigitalPinCap::Flags &flg
) {
	return cap.capabilities ^ flg;
}

constexpr DigitalPinCap::Flags operator ^ (
	const DigitalPinCap::Flags &flg,
	const DigitalPinCap &cap
) {
	return flg ^ cap.capabilities;
}

constexpr bool operator == (
	const DigitalPinCap &cap,
	const DigitalPinCap::Flags &flg
) {
	return cap.capabilities == flg;
}

constexpr bool operator == (
	const DigitalPinCap::Flags &flg,
	const DigitalPinCap &cap
) {
	return flg == cap.capabilities;
}

constexpr bool operator != (
	const DigitalPinCap &cap,
	const DigitalPinCap::Flags &flg
) {
	return cap.capabilities != flg;
}

constexpr bool operator != (
	const DigitalPinCap::Flags &flg,
	const DigitalPinCap &cap
) {
	return flg != cap.capabilities;
}

} } }
