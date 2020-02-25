/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPINSETACCESS_HPP
#define DIGITALPINSETACCESS_HPP

#include <duds/hardware/interface/DigitalPort.hpp>
#include <duds/hardware/interface/DigitalPinAccessBase.hpp>

namespace duds { namespace hardware { namespace interface {

/**
 * Provides access to multiple pins on a DigitalPort.
 * This allows using multiple pins in a single operation.
 *
 * @todo  Allow a DigitalPort implementation to provide bit flags for use as
 *        a mask of the pins to access. The size of the mask will need to be
 *        dynamic, like std::vector<bool>. The mask will need to be available
 *        as integers with the flags, sort of like std::bitset<T>::to_ulong().
 *        The size of the mask must not be limited to what fits in a single
 *        integer, unlike std::bitset.
 *
 * @author  Jeff Jackowski
 */
class DigitalPinSetAccess : public DigitalPinAccessBase {
	/**
	 * The port local pin IDs this object may use.
	 */
	std::vector<unsigned int> pinvec;
	/**
	 * Used by DigitalPort.
	 */
	DigitalPinSetAccess(DigitalPort *port, std::vector<unsigned int> &&pids) :
		DigitalPinAccessBase(port), pinvec(std::move(pids)) { }
	/**
	 * Reserves additional space in @a pins so that upcoming pushes onto the
	 * vector will not cause multiple memory reallocations.
	 * @param len  The anticipated number of items about to be added.
	 */
	void reserveAdditional(unsigned int len);
	friend DigitalPort;
public:
	/**
	 * Constructs an access object with nothing to access.
	 */
	DigitalPinSetAccess() = default;
	/**
	 * Relinquish access on destruction.
	 */
	~DigitalPinSetAccess() {
		retire();
	}
	/*
	 * A move constructor. Causes troubles internal to DigitalPort.
	 */
	//DigitalPinSetAccess(DigitalPinAccess &&old) noexcept;
	/**
	 * Relinquish access.
	 */
	void retire() noexcept;
	/**
	 * Relinquish access to a single pin.
	 * Good idea?
	 * Leave -1 in its spot, unless at end which can be removed.
	 * @param pos  The pin's position within the access object.
	 */
	void retire(unsigned int pos);
	/**
	 * A move assignment. This requires a call to DigitalPort::updateAccess(),
	 * which needs to synchronize on its internal data. As a result, move
	 * assignments are not speedy. However, they assure pin access is transfered
	 * without being lost.
	 */
	DigitalPinSetAccess &operator=(DigitalPinSetAccess &&old) noexcept;
	/**
	 * Provides access to the internal vector of local pin IDs.
	 */
	const std::vector<unsigned int> &localIds() const {
		return pinvec;
	}
	/**
	 * Returns a vector of global pin IDs for the pins accessed by this
	 * object.
	 */
	std::vector<unsigned int> globalIds() const {
		return port()->globalIds(pinvec);
	}
	/**
	 * Returns true if this object has been given any pins to access.
	 */
	bool havePins() const {
		return (port() != nullptr) && !pinvec.empty();
	}
	/**
	 * Returns true if the given position is for an existent pin rather than
	 * a gap or a position past the end.
	 * @param pos  The position to check.
	 */
	bool exists(unsigned int pos) const {
		return (pos < pinvec.size()) && (pinvec[pos] != -1);
	}
	/**
	 * Returns the number of pins in this access object. The count includes
	 * pins set as -1; gaps in the pins to access.
	 */
	unsigned int size() const {
		return (unsigned int)pinvec.size();
	}
	/**
	 * Returns the local pin ID of the pin at the given position inside this
	 * set of pins.
	 * @param pos  The position of the pin in this set.
	 */
	unsigned int localId(unsigned int pos) const {
		// check range using at()
		return pinvec.at(pos);
	}
	/**
	 * Returns the global pin ID of the pin at the given position inside this
	 * set of pins.
	 * @param pos  The position of the pin in this set.
	 */
	unsigned int globalId(unsigned int pos) const {
		// check range using at()
		return port()->globalId(pinvec.at(pos));
	}
	/**
	 * Returns the capabilities of the specified pin.
	 * @param pos  The position of the pin in this set.
	 */
	DigitalPinCap capabilities(unsigned int pos) const {
		return port()->capabilities(globalId(pos));
	}
	/**
	 * Returns the capabilities of all the pins in this set. Any gaps in the
	 * set (ID is -1) will have a value of
	 * DigitalPinCap::Nonexistent.
	 */
	std::vector<DigitalPinCap> capabilities() const {
		return port()->capabilitiesLocalIds(pinvec);
	}
	/**
	 * Returns the current configuration of the specified pin.
	 * @param pos  The position of the pin in this set.
	 */
	DigitalPinConfig configuration(unsigned int pos) const {
		return port()->configuration(globalId(pos));
	}
	/**
	 * Returns the current configuration all pins in this set. Any gaps in the
	 * set (ID is -1) will have a configuration of
	 * DigitalPinConfig::OperationNoChange.
	 */
	std::vector<DigitalPinConfig> configuration() const {
		return port()->configurationLocalIds(pinvec);
	}
	/**
	 * Produces a vector with port local pin IDs that are a subset of the pins
	 * in this access object.
	 * @param pos  The pins to find specified by their position in this pin
	 *             set. Values of -1 are copied into the result to maintain
	 *             any gaps.
	 * @return     The pins identified by their port local IDs.
	 * @warning    This function does not check for repeated values.
	 * @throw      PinDoesNotExist  A specified position is outside the
	 *             bounds of this set.
	 */
	std::vector<unsigned int> subset(const std::vector<unsigned int> &pos) const;
	/**
	 * Propose a new configuration for the given pin using the current
	 * configuration as the initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param conf  The proposed configuration.
	 * @param pos   The position of the pin in this set.
	 * @return      The reason the configuration is bad, or
	 *              DigitalPinRejectedConfiguration::NotRejected if it can
	 *              be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int pos,
		DigitalPinConfig &conf
	) const {
		return port()->proposeConfig(globalId(pos), conf);
	}
	/**
	 * Propose a new configuration for the given pin using a hypothetical
	 * given initial configuration. The pin's configuration is
	 * not changed; this only checks for a valid supported change.
	 * @param proposed  The proposed configuration.
	 * @param initial   The initial configuration. It should be a valid
	 *                  configuration for the pin and port, but it doesn't
	 *                  need to be the current configuration.
	 * @param pos       The position of the pin in this set.
	 * @return          The reason the configuration is bad, or
	 *                  DigitalPinRejectedConfiguration::NotRejected if it
	 *                  can be used.
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int pos,
		DigitalPinConfig &proposed,
		DigitalPinConfig &initial
	) const {
		return port()->proposeConfig(globalId(pos), proposed, initial);
	}
	/**
	 * Propose a new configuration for the entire pin set using a hypothetical
	 * given initial configuration. The configuration is not changed; this
	 * only checks for a valid supported change.
	 * @param propConf      The proposed configuration.
	 * @param initConf      The initial configuration. It should be a valid
	 *                      configuration for the pins and port, but it doesn't
	 *                      need to be the current configuration.
	 * @param insertReason  A function that, if specified, will be called for
	 *                      each pin, in the order specified by this set, with
	 *                      the rejection reason for that pin. The reason will
	 *                      be DigitalPinRejectedConfiguration::NotRejected if
	 *                      the pin's proposed configuration is good. The
	 *                      function is optional.
	 * @return              True if the proposed configuration is good. False
	 *                      if there was any rejection.
	 */
	bool proposeConfig(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) {
		return port()->proposeConfigLocalIds(
			pinvec,
			propConf,
			initConf,
			insertReason
		);
	}
	/*
	template <class FwdConfIter>
	std::vector<DigitalPinConfig> proposeConfig(
		const FwdConfIter &begin,
		const FwdConfIter &end,
	) const {
		// need to have a copy of the configs for changes
		std::vector<DigitalPinConfig> proposed(begin, end);
		port()->proposeConfig(proposed, pinvec);
		return proposed;
		// provide DigitalPinRejectedConfiguration ???
	}
	template <class FwdConfIter>
	void modifyConfig(const FwdConfIter &begin, const FwdConfIter &end) const;
	*/
	/**
	 * Modifies the configuration of a pin. If the port implementation is a
	 * derivative of DigitalPortDependentPins, the change may affect multiple
	 * pins, and the configuration of other pins may prevent the requested
	 * change.
	 * @param pos   The position of the pin in this set.
	 * @param conf  The requested new configuration.
	 * @return      The actual new configuration.
	 */
	DigitalPinConfig modifyConfig(
		unsigned int pos,
		const DigitalPinConfig &conf
	) const {
		return port()->modifyConfig(globalId(pos), conf, &portdata);
	}
	/**
	 * Sets the configuration for all the pins.
	 * @bug   The set may not have a gap (pin ID -1). This will result in a
	 *        DigitalPinConfigError exception being thrown.
	 * @param conf  The configuration to set for all pins.
	 * @throw DigitalPinConfigError
	 */
	void modifyConfig(const DigitalPinConfig &conf) const;
	/**
	 * Sets the configuration for all the pins.
	 * @param conf  The new configuration. The vector makes a parallel data
	 *              structure with the pins stroed in this object. If a gap
	 *              exists (ID is -1), the configuration must be set to
	 *              DigitalPinConfig::OperationNoChange.
	 * @throw DigitalPinConfigRangeError  The size of @a conf is not the same
	 *                                    as size().
	 * @throw DigitalPinConfigError
	 */
	void modifyConfig(std::vector<DigitalPinConfig> &conf) const {
		port()->modifyConfig(pinvec, conf, &portdata);
	}
	/**
	 * Sets the configuration for a subset of the pins.
	 * @param pos   A vector of the pins to use specified by their position in
	 *              this pin set.
	 * @param conf  The new configuration. The vector makes a parallel data
	 *              structure with the specified subset of pins. If a gap
	 *              exists (ID is -1), the configuration must be set to
	 *              DigitalPinConfig::OperationNoChange.
	 * @throw DigitalPinConfigRangeError  The size of @a conf is not the same
	 *                                    as the size of @a pos.
	 * @throw DigitalPinConfigError
	 */
	void modifyConfig(
		const std::vector<unsigned int> &pos,
		std::vector<DigitalPinConfig> &conf
	) const {
		port()->modifyConfig(subset(pos), conf, &portdata);
	}
	/**
	 * Samples the input state of a pin.
	 * @param pos   The position of the pin in this set.
	 * @throw PinWrongDirection    This pin is not configured as an input.
	 */
	bool input(unsigned int pos) const {
		return port()->input(globalId(pos), &portdata);
	}
	/**
	 * Samples the input state of all the pins.
	 * @return   The input from the pins.
	 * @throw PinWrongDirection  Not all the pins are not configured as an input.
	 */
	std::vector<bool> input() const {
		return port()->input(pinvec, &portdata);
	}
	/**
	 * Samples the input state of a subset of the pins.
	 * @param pos  A vector of the pins to sample specified by their position
	 *             in this pin set.
	 * @return     The input from the pins in the same order as @a pos.
	 * @throw PinWrongDirection   A pin in @a pos is not configured as an input.
	 */
	std::vector<bool> input(const std::vector<unsigned int> &pos) const {
		return port()->input(subset(pos), &portdata);
	}
	/**
	 * Changes the output state of a pin. If the pin is not currently
	 * configured to output, the configuration will not change, but the new
	 * output state will be used when the pin becomes an output in the future.
	 * @param pos    The position of the pin in this set.
	 * @param state  The new output state.
	 */
	void output(unsigned int pos, bool state) const {
		port()->output(globalId(pos), state, &portdata);
	}
	/**
	 * Changes the output state of all the pins. If a pin is not currently
	 * configured to output, the configuration will not change, but the new
	 * output state will be used when the pin becomes an output in the future.
	 * @param state  The new output states. The vector makes a parallel data
	 *               structure with the pins in this set (@a pinvec). Gaps
	 *               (ID of -1) are not presently supported.
	 */
	void output(const std::vector<bool> &state) const {
		port()->output(pinvec, state, &portdata);
	}
	/**
	 * Changes the output state of all the pins. If a pin is not currently
	 * configured to output, the configuration will not change, but the new
	 * output state will be used when the pin becomes an output in the future.
	 * @bug   The set may not have a gap (pin ID -1). This will result in a
	 *        DigitalPinConfigError exception being thrown.
	 * @param state  The new output state for all pins.
	 */
	void output(bool state) const;
	/**
	 * Changes the output state of a subset of the pins. If a pin is not
	 * currently configured to output, the configuration will not change, but
	 * the new output state will be used when the pin becomes an output in the
	 * future.
	 * @param pos    A vector of the pins to use specified by their position in
	 *               this pin set.
	 * @param state  The new output states. The vector makes a parallel data
	 *               structure with the specified subset of pins. Gaps
	 *               (ID of -1) are not presently supported.
	 * @param state  The new output state.
	 */
	void output(
		const std::vector<unsigned int> &pos,
		const std::vector<bool> &state
	) const {
		port()->output(subset(pos), state, &portdata);
	}

	// convenience functions -- may expand later

	/**
	 * True if the port supports operating on multiple pins simultaneously. If
	 * false, the pins may be modified on over a period of time in an
	 * implementation defined order.
	 */
	bool simultaneousOperations() const {
		return port()->simultaneousOperations();
	}
	/**
	 * Returns true if all pins on the port always have an independent
	 * configuration from all other pins.
	 */
	bool independentConfig() const {
		return port()->independentConfig();
	}
	/**
	 * Returns true if the pin is configured as an input.
	 * @param pos  The position of the pin in this set.
	 */
	bool isInput(unsigned int pos) const {
		return configuration(pos) & DigitalPinConfig::DirInput;
	}
	/**
	 * Returns true if the pin is configured as an output.
	 * @param pos  The position of the pin in this set.
	 */
	bool isOutput(unsigned int pos) const {
		return configuration(pos) & DigitalPinConfig::DirOutput;
	}
	/**
	 * Returns true if the pin can operate as an input.
	 * @param pos  The position of the pin in this set.
	 */
	bool canBeInput(unsigned int pos) const {
		return capabilities(pos) & DigitalPinCap::Input;
	}
	/**
	 * Returns true if the pin can operate as an output.
	 * @param pos  The position of the pin in this set.
	 */
	bool canBeOutput(unsigned int pos) const {
		return capabilities(pos) &  (
			DigitalPinCap::OutputPushPull |
			DigitalPinCap::OutputDriveLow |
			DigitalPinCap::OutputDriveHigh
		);
	}
	/**
	 * Returns true if the pin can provide a non-input high impedence state
	 * (or maybe allow input state?).
	 * @param pos  The position of the pin in this set.
	 */
	bool canFloat(unsigned int pos) const {
		return capabilities(pos) & DigitalPinCap::OutputHighImpedance;
	}
	/**
	 * Writes out a number in binary to the pins. The LSb is given to the pin
	 * at position 0, the next bit to position 1, and so on. As with the other
	 * output functions, the pin configuration will not be changed.
	 * @tparam Int   The integer type used to hold the number.
	 * @param  val   The number to write.
	 * @param  bits  The number of bits to write. This may not exceed the number
	 *               of pins in this set. If it exceeds the number of bits in
	 *               @a Int, the more significant bits will be zero.
	 * @throw  PinRangeError  The number of bits to write is less than 1 or
	 *                        greater than the number of pins in this set.
	 * @throw DigitalPinNumericRangeError  The given value is too large to fit
	 *                                     in the requested number of bits.
	 */
	template <typename Int>
	void write(Int val, int bits) const {
		// range check; must have enough pins
		if ((bits < 1) || (bits > pinvec.size())) {
			DUDS_THROW_EXCEPTION(PinRangeError());
		}
		// range check; value must fit
		/** @bug  Will fail for unsigned types with negative values. */
		if (val >= (1 << bits)) {
			DUDS_THROW_EXCEPTION(DigitalPinNumericRangeError() <<
				DigitalPinNumericOutput(val) << DigitalPinNumericBits(bits)
			);
		}
		// prepare a place to store the output.
		std::vector<bool> out;
		out.reserve(bits);
		// produce the output
		for (; bits; --bits) {
			out.push_back(val & 1);
			val >>= 1;
		}
		// whole set?
		if (out.size() == pinvec.size()) {
			// write the value using all pins
			port()->output(pinvec, out, &portdata);
		} else {
			// produce a subset of the pins
			std::vector<unsigned int> pins(
				pinvec.begin(),
				pinvec.begin() + out.size()
			);
			// write out the number
			port()->output(pins, out, &portdata);
		}
	}
	/**
	 * Writes out a number in binary to the pins. The LSb is given to the pin
	 * at position 0, the next bit to position 1, and so on. All the pins in
	 * this set will be used. As with the other output functions, the pin
	 * configuration will not be changed.
	 * @tparam Int   The integer type used to hold the number.
	 * @param  val   The number to write.
	 */
	template <typename Int>
	void write(Int val) const {
		write(val, pinvec.size());
	}
};

// maybe a full-port access object?

} } }

#endif        //  #ifndef DIGITALPINSETACCESS_HPP
