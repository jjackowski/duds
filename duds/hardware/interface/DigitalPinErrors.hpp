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
 * Various errors involving the use of digital GPIO hardware.
 * @author  Jeff Jackowski
 */

#include <duds/hardware/interface/PinErrors.hpp>
#include <duds/general/BitFlags.hpp>
#include <vector>

namespace duds { namespace hardware { namespace interface {

/**
 * Indicates an error with the digital pin's configuration. If this class is
 * used rather than a derived one, it should include an error attribute with
 * a DigitalPinRejectedConfiguration::Reason with more detailed information
 * and potentially multiple configuration issues.
 */
struct DigitalPinConfigError : PinError { };

/**
 * A requested operation or configuration is not supported.
 */
struct DigitalPinUnsupportedOperation : DigitalPinConfigError,
	virtual PinUnsupportedOperation { };

/**
 * Indicates that mutually exclusive option flags were set for pin direction.
 */
struct DigitalPinConflictingDirectionError : DigitalPinConfigError { };

/**
 * Indicates that mutually exclusive option flags were set for pull
 * ups or downs.
 */
struct DigitalPinConflictingPullError : DigitalPinConfigError { };

/**
 * Indicates that mutually exclusive option flags were set for pin events.
 */
struct DigitalPinConflictingEventError : DigitalPinConfigError { };

/**
 * Indicates that mutually exclusive option flags were set for pin interrupts.
 */
struct DigitalPinConflictingInterruptError : DigitalPinConfigError { };

/**
 * Indicates that mutually exclusive option flags were set for pin output drive.
 */
struct DigitalPinConflictingOutputError : DigitalPinConfigError { };

/**
 * Indicates that both the maximum and minimum output current were specified,
 * and the maximum is less than the minimum.
 */
struct DigitalPinConflictingCurrentError : DigitalPinConfigError { };

/**
 * Indicates that an operation specified more than one configuration for a pin.
 * The configurations may be identical; specifying the same pin more than
 * once in the same operation is not permitted.
 */
struct DigitalPinMultipleConfigError : DigitalPinConfigError { };

/**
 * Indicates that the specified configuration data includes too many or too
 * few items, or has parallel data structures of inconsistent sizes.
 */
struct DigitalPinConfigRangeError : DigitalPinConfigError { };

/**
 * Indicates that a request to configure a pin to output was made of a pin
 * that cannot output.
 */
struct DigitalPinCannotOutputError : DigitalPinConfigError { };

/**
 * Indicates that a request to configure a pin to input was made of a pin
 * that cannot intput.
 */
struct DigitalPinCannotInputError : DigitalPinConfigError { };

struct DigitalPinNumericRangeError : PinError { };
typedef boost::error_info<
	struct Info_DigitalPinNumericOutput,
	std::int64_t
> DigitalPinNumericOutput;
typedef boost::error_info<
	struct Info_DigitalPinNumericBits,
	int
> DigitalPinNumericBits;


/**
 * Holds error types and codes that detail why a configuration for a digital
 * pin was rejected. These are placed outside a class becuase they can be
 * claimed equally by DigitalPin and DigitalPort, but both classes need to use
 * the same error classes and values.
 */
namespace DigitalPinRejectedConfiguration {

/**
 * A set of bit flags for storing pin configuration errors. This allows
 * multiple errors to be reported. If the stored value is zero, it indicates
 * no errors.
 */
typedef duds::general::BitFlags<struct DigitalPinConfigErrorFlags, std::uint8_t>
	Reason;

/**
 * There is no error with the requested pin configuration for the referenced
 * pin.
 */
constexpr Reason NotRejected = Reason::Zero();

/**
 * There is an unspecified error with the requested pin configuration for the
 * referenced pin that is not covered by another error flag.
 */
constexpr Reason UnspecifiedError = Reason::Bit(0);

/**
 * The I/O direction configuration is not supported by the hardware or driver.
 */
constexpr Reason UnsupportedDirection = Reason::Bit(1);

/**
 * The requested pull up or pull down configuration is not supported by
 * the hardware or driver.
 */
constexpr Reason UnsupportedInputPull = Reason::Bit(2);

/**
 * The event pin configuration is not supported by the hardware or driver.
 */
constexpr Reason UnsupportedEvent = Reason::Bit(3);

/**
 * The requested interrupt configuration is not supported by the hardware or
 * driver.
 */
constexpr Reason UnsupportedInterrupt = Reason::Bit(4);

/**
 * The requested output configuration is not supported by the hardware or
 * driver.
 */
constexpr Reason UnsupportedOutput = Reason::Bit(5);

/**
 * The requested pin configuration for the referenced pin affects multiple
 * pins in a manner that was not allowed by the configuration.
 */
constexpr Reason AffectsOthers = Reason::Bit(6);

/**
 * The requested configuration of another pin implied a change to the
 * referenced pin's configuration, but the referenced pin's configuration
 * explicitly disallowed the change.
 */
constexpr Reason WronglyAffected = Reason::Bit(7);

/**
 * The referenced pin's requested configuration either implied a disallowed
 * change of another pin's configuration, or the reverse.
 */
constexpr Reason BadEffect = AffectsOthers | WronglyAffected;

/**
 * Completely unsupported.
 */
constexpr Reason Unsupported =
	UnsupportedDirection | UnsupportedInputPull | UnsupportedEvent |
	UnsupportedInterrupt | UnsupportedOutput;

/**
 * Allows attaching the configuration rejection flags to an exception.
 * @warning  There isn't a good way to output this yet for user inspection.
 */
typedef boost::error_info<
	struct Info_DigitalPinRejectedConfiguration,
	Reason
> ReasonInfo;

typedef boost::error_info<
	struct Info_DigitalPinRejectedConfigurationVector,
	std::vector<Reason>
> ReasonVectorInfo;

/**
 * Needed for using Reason with boost::error_info; ReasonInfo will not build
 * without stream operators.
 */
inline std::ostream &operator<<(std::ostream &os, const Reason &r) {
	return os << (int)(r.flags());
}

}

} } }
