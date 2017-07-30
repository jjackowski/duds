/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <cstdint>
#include <duds/hardware/interface/DigitalPinErrors.hpp>
#include <iostream>

namespace duds { namespace hardware { namespace interface {

struct DigitalPinCap;

/**
 * Defines the configuration for a digital general purpose I/O pin.
 * @author  Jeff Jackowski
 */
struct DigitalPinConfig {
	/**
	 * A container of flags that control the operation of a digital pin. The
	 * flags are intended to cover a wide range of hardware, so not all options
	 * may be available on specific hardware.
	 */
	typedef duds::general::BitFlags<struct DigitalPinConfigFlags, std::uint32_t>
		Flags;
	// direction
	/**
	 * Configure the pin for input. The output flags, masked by @a OutputMask,
	 * must be clear to use input.
	 */
	static constexpr Flags DirInput = Flags::Bit(0);
	/**
	 * Configure the pin for output.
	 *
	 * The input pull flags, masked by @a InputPullMask, must be clear to use
	 * output, save for @a InputNoPull. The @a InputNoPull flag is set for
	 * output, and included in this flag to simplify program logic.
	 *
	 * The event settings are independent of the direction setting. However,
	 * this flag implies the @a EventNone flag unless another event flag
	 * is specified. This is for ease of use as it is the most common use-case.
	 */
	static constexpr Flags DirOutput = Flags::Bit(1); // normally implies bit 7
	/**
	 * Do not care about the pin's direction. This allows the direction to be
	 * changed if a request to alter the configuration of several pins must
	 * change this pin's direction to succeed with the given hardware. If the
	 * request can be honored without changing the pin's direction, then the
	 * direction will not change.
	 */
	static constexpr Flags DirImmaterial = Flags::Bit(2);
	/**
	 * Do not change the pin's direction.
	 */
	static constexpr Flags DirNoChange = Flags::Zero();
	/**
	 * A bit mask for all direction flags.
	 */
	static constexpr Flags DirMask =
		DirInput | DirOutput;
	// input options - pull up/down
	/**
	 * Configure the pin to not use a pull-up or pull-down resistor.
	 */
	static constexpr Flags InputNoPull = Flags::Bit(3);
	/**
	 * Configure the pin to have a pull-down resistor.
	 */
	static constexpr Flags InputPulldown = Flags::Bit(4);
	/**
	 * Configure the pin to have a pull-up resistor.
	 */
	static constexpr Flags InputPullup = Flags::Bit(5);
	/**
	 * Do not care about the pin's use of a pull-up or pull-down resistor.
	 * This allows the pull-up or pull-down to be changed if a request to
	 * alter the configuration of several pins must change this pin's
	 * pull-up or pull-down resistor useage to succeed with the given hardware.
	 * If the request can be honored without making such a change, then it will
	 * not be changed.
	 */
	static constexpr Flags InputPullImmaterial = Flags::Bit(6);
	/**
	 * Do not change the use or non-use of a pull-up or pull-down resistor.
	 */
	static constexpr Flags InputPullNoChange = Flags::Zero();
	/**
	 * A bit mask for the pull-up and pull-down resistor flags.
	 */
	static constexpr Flags InputPullMask =
		InputNoPull | InputPullup | InputPulldown;
	// input options - event
	/**
	 * Configure the pin to not flag an event when the input state changes.
	 */
	static constexpr Flags EventNone = Flags::Bit(7);
	/**
	 * Configure the pin to flag an event on the falling edge.
	 */
	static constexpr Flags EventEdgeFalling = Flags::Bit(8);
	/**
	 * Configure the pin to flag an event on the rising edge.
	 */
	static constexpr Flags EventEdgeRising = Flags::Bit(9);
	/**
	 * Configure the pin to flag an event on any edge.
	 */
	static constexpr Flags EventEdge = EventEdgeFalling | EventEdgeRising;
	/**
	 * Configure the pin to flag an event on a low level input.
	 */
	static constexpr Flags EventLevelLow = Flags::Bit(10);
	/**
	 * Configure the pin to flag an event on a high level input.
	 */
	static constexpr Flags EventLevelHigh = Flags::Bit(11);
	/**
	 * Do not care about the pin's event configuration. This allows the
	 * event configuration to be changed if a request to alter the
	 * configuration of several pins must change this pin's event
	 * configuration to succeed with the given hardware. If the request can
	 * be honored without making such a change, then the event
	 * configuration will not be changed.
	 */
	static constexpr Flags EventImmaterial = Flags::Bit(12);
	/**
	 * Do not change the pin's event configuration.
	 */
	static constexpr Flags EventNoChange = Flags::Zero();
	/**
	 * A bit mask for the event configuration flags.
	 */
	static constexpr Flags EventMask =
		EventNone | EventEdgeRising | EventEdgeFalling |
		EventLevelLow | EventLevelHigh;
	// input options - interrupt
	/**
	 * Configure the pin to not trigger an interrupt
	 */
	static constexpr Flags InterruptNone = Flags::Bit(13);
	/**
	 * Configure the pin to trigger an interrupt when an event occurs.
	 */
	static constexpr Flags InterruptOnEvent = Flags::Bit(14);
	/**
	 * Do not care about the pin's interrupt configuration. This allows the
	 * interrupt configuration to be changed if a request to alter the
	 * configuration of several pins must change this pin's interrupt
	 * configuration to succeed with the given hardware. If the request can
	 * be honored without making such a change, then the interrupt
	 * configuration will not be changed.
	 */
	static constexpr Flags InterruptImmaterial = Flags::Bit(15);
	/**
	 * Do not change the pin's interrupt configuration.
	 */
	static constexpr Flags InterruptNoChange = Flags::Zero();
	/**
	 * A bit mask for the interrupt configuration flags.
	 */
	static constexpr Flags InterruptMask = InterruptNone | InterruptOnEvent;
	// output options
	/**
	 * Configure the pin to be able to drive the output low. If not configured
	 * to also drive the output high, then operation like an open collector or
	 * open drain output is requested, and may be emulated by configuring the
	 * pin as an input when the output state is changed to high. Emulation
	 * using input will only be done if no other pin can be affected.
	 */
	static constexpr Flags OutputDriveLow = Flags::Bit(16);
	/**
	 * Configure the pin to be able to drive the output high. If not configured
	 * to also drive the output low, then operation like an open emitter is
	 * requested, and may be emulated by configuring the pin as an input when
	 * the output state is changed to low. Emulation using input will only be
	 * done if no other pin can be affected.
	 */
	static constexpr Flags OutputDriveHigh = Flags::Bit(17);
	/**
	 * Configure the pin to drive output both high and low.
	 */
	static constexpr Flags OutputPushPull = OutputDriveLow | OutputDriveHigh;
	/**
	 * Configure the pin to have a high impedance or floating output.
	 */
	static constexpr Flags OutputHighImpedance = Flags::Bit(18);
	/**
	 * Do not care about the pin's output configuration. This allows the
	 * output configuration to be changed if a request to alter the
	 * configuration of several pins must change this pin's output
	 * configuration to succeed with the given hardware. If the request can
	 * be honored without making such a change, then the output
	 * configuration will not be changed.
	 */
	static constexpr Flags OutputImmaterial = Flags::Bit(19);
	/**
	 * Do not change the pin's output configuration.
	 */
	static constexpr Flags OutputNoChange = Flags::Zero();
	/**
	 * A bit mask for the output options.
	 */
	static constexpr Flags OutputMask =
		OutputDriveLow | OutputDriveHigh | OutputPushPull |
		OutputHighImpedance;
	/**
	 * No change to any pin operation.
	 */
	static constexpr Flags OperationNoChange = Flags::Zero();
	/**
	 * The last known input state from the pin. This flag is updated on any
	 * single pin read operation for the pin. It is handled independently of
	 * the output state.
	 */
	static constexpr Flags InputState = Flags::Bit(20);
	/**
	 * The set output state for the pin. This flag is updated on any single
	 * pin write operation for the pin. It is handled independently of
	 * the input state.
	 */
	static constexpr Flags OutputState = Flags::Bit(21);
	/**
	 * The control options requested for a digital pin.
	 */
	Flags options;
	/**
	 * The selected minimum output current in milliamps (?) for the pin, or
	 * zero for no change and immaterial. This will have no effect for inputs.
	 * If the pin capabilities ommits an output current limit, this value will
	 * not be used.
	 */
	std::uint16_t minOutputCurrent;
	/**
	 * The selected maximum output current in milliamps (?) for the pin, or
	 * zero for no change and immaterial. This will have no effect for inputs.
	 * If the pin capabilities ommits an output current limit, this value will
	 * not be used.
	 * @todo  Rethink usage; may be useless; not currently used.
	 */
	std::uint16_t maxOutputCurrent;
	/**
	 * Construct uninitialized.
	 */
	DigitalPinConfig() = default;
	/**
	 * Construct with initial flags and current values.
	 * @param opt     The initial configuration flags.
	 * @param minOut  The initial minimum output current. The default value 0
	 *                indicates that a minimum value is not set.
	 * @param maxOut  The initial maximum output current. The default value 0
	 *                indicates that a maximum value is not set.
	 */
	constexpr DigitalPinConfig(
		const Flags opt,
		const std::uint16_t minOut = 0,
		const std::uint16_t maxOut = 0
	) :
		options(opt),
		minOutputCurrent(minOut),
		maxOutputCurrent(maxOut)
	{ }
	/**
	 * Construction option for initializing all fields to defaul values.
	 */
	struct ClearAll { };
	/**
	 * Construct with all values initialized to zero.
	 */
	constexpr DigitalPinConfig(const ClearAll) :
		options(Flags::Zero()),
		minOutputCurrent(0),
		maxOutputCurrent(0)
	{ }
	/**
	 * Checks for the use of obviously invalid data, such as the use of
	 * mutually exclusive options. This is intended as a debugging check.
	 * Any invalid data found is a programming error, so the error is reported
	 * as an exception.
	 * @throw DigitalPinConfigurationError  An exception indicating the
	 *                                      first problem found. The  actual
	 *                                      object will be derived from this
	 *                                      class.
	 */
	void checkValidity() const;
	/**
	 * Combines this configuration with a newer configuration taking into
	 * account requests to not change certain options. If an immaterial flag
	 * is set in @a newCfg, the corresponding configuration flags will be
	 * taken from this object and the immaterial flag will be set.
	 * @post  This configuration object is changed to the result of the
	 *        combination.
	 * @param newCfg  The new configuration. Anything set to a value indicating
	 *                no change will result in the current configuration
	 *                (this object) being used for that option.
	 */
	void combine(const DigitalPinConfig &newCfg);
	/**
	 * Combines a configuration with a newer configuration taking into
	 * account requests to not change certain options, and produces a new
	 * configuration object. If an immaterial flag is set in @a newCfg, the
	 * corresponding configuration flags will be taken from @a oldCfg and the
	 * immaterial flag will be set.
	 * @param oldCfg  The starting configuration.
	 * @param newCfg  The new configuration. Anything set to a value indicating
	 *                no change will result in the starting configuration,
	 *                @a oldCfg, being used for that option.
	 * @return        The resulting configuration.
	 */
	static DigitalPinConfig combine(
		const DigitalPinConfig &oldCfg,
		const DigitalPinConfig &newCfg
	);
	/**
	 * Combines an old (initial) configuration with a new configuration in this
	 * object and stores the result in this object.
	 * @todo  Finish this comment.
	 */
	void reverseCombine(const DigitalPinConfig &oldCfg) {
		DigitalPinConfig oc(oldCfg);
		oc.combine(*this);
		*this = oc;
	}
	/**
	 * Returns a set of flags that indicate certain incompatible conditions
	 * in the given pin configuration irrespective of any other pin. The only
	 * condition currently checked is whether or not the configuration is
	 * supported by the hardware.
	 * @param cap  The capabilities to check against.
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
		const DigitalPinCap &cap
	) const;
};

typedef boost::error_info<
	struct Info_DigitalPinConfig, DigitalPinConfig
> DigitalPinConfigInfo;

inline std::ostream &operator<<(std::ostream &os, const DigitalPinConfig &c) {
	return os << '(' << c.options.flags() << ',' << c.minOutputCurrent << ',' <<
	c.maxOutputCurrent << ')';
}

constexpr DigitalPinConfig::Flags operator & (
	const DigitalPinConfig &cap,
	const DigitalPinConfig::Flags &flg
) {
	return cap.options & flg;
}

constexpr DigitalPinConfig::Flags operator & (
	const DigitalPinConfig::Flags &flg,
	const DigitalPinConfig &cap
) {
	return flg & cap.options;
}

constexpr DigitalPinConfig::Flags operator | (
	const DigitalPinConfig &cap,
	const DigitalPinConfig::Flags &flg
) {
	return cap.options | flg;
}

constexpr DigitalPinConfig::Flags operator | (
	const DigitalPinConfig::Flags &flg,
	const DigitalPinConfig &cap
) {
	return flg | cap.options;
}

constexpr DigitalPinConfig::Flags operator ^ (
	const DigitalPinConfig &cap,
	const DigitalPinConfig::Flags &flg
) {
	return cap.options ^ flg;
}

constexpr DigitalPinConfig::Flags operator ^ (
	const DigitalPinConfig::Flags &flg,
	const DigitalPinConfig &cap
) {
	return flg ^ cap.options;
}

constexpr bool operator == (
	const DigitalPinConfig &cap,
	const DigitalPinConfig::Flags &flg
) {
	return cap.options == flg;
}

constexpr bool operator == (
	const DigitalPinConfig::Flags &flg,
	const DigitalPinConfig &cap
) {
	return flg == cap.options;
}

constexpr bool operator != (
	const DigitalPinConfig &cap,
	const DigitalPinConfig::Flags &flg
) {
	return cap.options != flg;
}

constexpr bool operator != (
	const DigitalPinConfig::Flags &flg,
	const DigitalPinConfig &cap
) {
	return flg != cap.options;
}

} } }
