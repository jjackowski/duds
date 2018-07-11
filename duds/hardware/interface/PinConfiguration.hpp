/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <duds/hardware/interface/DigitalPinSet.hpp>
#include <duds/hardware/interface/DigitalPin.hpp>
#include <duds/hardware/interface/ChipSelect.hpp>
#include <unordered_map>
#include <set>

namespace duds { namespace hardware { namespace interface {

class ChipSelectManager;
class DigitalPort;

/**
 * Base for all errors directly thrown by PinConfiguration. These errors
 * should all be related to the configuration itself, or to the operation of
 * PinConfiguration, and not to issues related to DigitalPort and other objects
 * used by PinConfiguration.
 */
struct PinConfigurationError : virtual std::exception, virtual boost::exception { };
/**
 * The requested port is not named in the configuration.
 */
struct PortDoesNotExistError : PinConfigurationError { };
/**
 * A given pin ID cannot be used with the port, such as an ID that is less than
 * the port's ID offset.
 */
struct PortBadPinIdError : PinConfigurationError { };
/**
 * The configuration gives the same name to more than one port.
 */
struct PortDuplicateError : PinConfigurationError { };
/**
 * The same pin global ID is used for more than one pin.
 */
struct PortDuplicatePinIdError : PinConfigurationError { };
/**
 * A given pin ID could not be parsed.
 */
struct PinBadIdError : PinConfigurationError { };
/**
 * The configuration gives the same name to more than one chip select manager.
 */
struct SelectManagerDuplicateError : PinConfigurationError { };
/**
 * A select manager was given an unknown type, or no type.
 */
struct SelectManagerUnknownTypeError : PinConfigurationError { };
/**
 * The requested chip select manager is not named in the configuration.
 */
struct SelectManagerDoesNotExistError : PinConfigurationError { };
/**
 * A required chip select manager has not yet been created most likely because
 * the port providing its pins has not yet been attached.
 */
struct SelectManagerNotCreated : PinConfigurationError { };
/**
 * The configuration gives the same name to more than one chip select.
 */
struct SelectDuplicateError : PinConfigurationError { };
/**
 * A selection logic state in the configuration could not be parsed.
 */
struct SelectBadStateError : PinConfigurationError { };
/**
 * A chip selection manager was defined without any pins.
 */
struct SelectNoPinsError : PinConfigurationError { };
/**
 * A chip selection manager was defined with pins from more than one port.
 */
struct SelectMultiplePortsError : PinConfigurationError { };
/**
 * The requested chip select does not exist. This can happen when querying for
 * the select, or when a select associated with a pin set in the configuration
 * is not defined by the configuration.
 */
struct SelectDoesNotExistError : PinConfigurationError { };
/**
 * A pin set was defined with pins from more than one port.
 */
struct SetMultiplePortsError : PinConfigurationError { };
/**
 * The configuration gives the same name to more than one pin set.
 */
struct SetDuplicateError : PinConfigurationError { };
/**
 * The requested pin set is not defined by the configuration.
 */
struct SetDoesNotExistError : PinConfigurationError { };
/**
 * A required pin set has not yet been created most likely because
 * the port providing its pins has not yet been attached.
 */
struct SetNotCreatedError : PinConfigurationError { };

/**
 * The name of the port as it appears in the configuration.
 */
typedef boost::error_info<struct Info_PortName, std::string>
	PortName;
/**
 * A pin name or number from the configuration that could not be used.
 */
typedef boost::error_info<struct Info_PortId, std::string>
	PinBadId;
/**
 * The global ID of a pin from a configuration that is involved in the error.
 */
typedef boost::error_info<struct Info_PortPinId, unsigned int>
	PortPinId;
/**
 * The name of a chip select manager from a configuration.
 */
typedef boost::error_info<struct Info_SelectManagerName, std::string>
	SelectManagerName;
/**
 * The name of a chip select from a configuration.
 */
typedef boost::error_info<struct Info_SelectName, std::string>
	SelectName;
/**
 * The string with a select logic state that could not be parsed.
 */
typedef boost::error_info<struct Info_SelectBadState, std::string>
	SelectBadState;
/**
 * The string with a chip selection manager type that is not valid.
 */
typedef boost::error_info<struct Info_SelectBadType, std::string>
	SelectBadType;
/**
 * The name of a pin set from a configuration.
 */
typedef boost::error_info<struct Info_SetName, std::string>
	SetName;

/**
 * Parses configuration data for DigitalPort, DigitalPin, DigitalPinSet,
 * ChipSelectManager, and ChipSelect objects. The configuration data can
 * be inspected without creating any of the objects to be configured, and
 * thus without accessing the hardware affected by the configuration.
 * Separate from this object, a DigitalPort must be created. The port to
 * configure must be attached to this configuration by a call to attachPort().
 * This will create all the other objects listed in the configuration.
 * Those objects can be found by querying for their name.
 *
 * Intended usage follows this order:
 * -# Parse a configuration file with
 *    [boost::property_tree](https://www.boost.org/doc/libs/1_67_0/doc/html/property_tree.html).
 * -# Construct a PinConfiguration and give it a subtree from the parsed
 *    configuration.
 * -# Make a suitable DigitalPort object.
 * -# @ref PinConfiguration::attachPort() "Attach the DigitalPort" to the
 *    PinConfiguration.
 * -# Query the PinConfiguration for the needed objects by name.
 * -# The PinConfiguration may be destroyed when it no longer needs to be
 *    queried.
 *
 * [Boost property tree](https://www.boost.org/doc/libs/1_67_0/doc/html/property_tree.html)
 * is used to parse and represnt data prior to passing that data to this object.
 * See the @ref DUDStoolsPinConf page for detailed documentation on what is
 * expected of the property tree.
 *
 * This object is not thread-safe during parsing. When parsing is not underway,
 * all queries are thread-safe.
 *
 * @author  Jeff Jackowski
 */
class PinConfiguration {
public:
	struct Port;
	/**
	 * Holds configuration data for a single digital I/O pin.
	 */
	struct Pin {
		/**
		 * The Port object that supplies the pin.
		 */
		Port *parent;
		/**
		 * Optional pin name.
		 */
		std::string name;
		/**
		 * Assigned global ID.
		 */
		unsigned int gid;
		/**
		 * Port ID.
		 */
		unsigned int pid;
		/**
		 * There is explicitly no pin connected to the corresponding global ID.
		 */
		static constexpr unsigned int NoPin = -1;
		/**
		 * No ID was specified, but there may still be a pin depending on the
		 * context.
		 */
		static constexpr unsigned int NoIdSpecified = -2;
		/**
		 * Parse pin subtree data stored in a property tree.
		 * @throw PortBadPinIdError  A pin ID could not be parsed.
		 */
		void parse(
			const std::pair<const std::string, boost::property_tree::ptree> &item,
			Port *owner
		);
		Pin();
		/**
		 * Create a new Pin with parsed pin subtree data stored in a property
		 * tree.
		 * @throw PortBadPinIdError  A pin ID could not be parsed.
		 */
		Pin(
			const std::pair<const std::string, boost::property_tree::ptree> &item,
			Port *owner
		);
	};
	/**
	 * Index in type Pins that is sorted by pin global ID. The index is
	 * non-unique because the IDs are finalized after the elements are created.
	 * The IDs will be unique after parsing has completed.
	 */
	struct index_gid { };
	/**
	 * Index in type Pins that is sorted by pin port ID. The index is
	 * non-unique because the Pins type is used to store a directory of all pins
	 * and there can be no requirement that port specific IDs are unique across
	 * all ports.
	 */
	struct index_pid { };
	/**
	 * Index in type Pins that is sorted by the optional pin name. The index is
	 * non-unique because all pins that aren't provided a name have the same
	 * empty name.
	 */
	struct index_name { };
	/**
	 * Index in type Pins that is maintained by parsing order. Items are in the
	 * same order as seen in the configuration file.
	 */
	struct index_seq { };
	/**
	 * Holds the configuration data for digital pins indexed by global ID,
	 * port ID, arbitrary name, and order in the configuration file.
	 */
	typedef boost::multi_index::multi_index_container<
		Pin,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<index_gid>,
				boost::multi_index::member<Pin, unsigned int, &Pin::gid>
			>,
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<index_pid>,
				boost::multi_index::member<Pin, unsigned int, &Pin::pid>
			>,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<index_name>,
				boost::multi_index::member<Pin, std::string, &Pin::name>
			>,
			boost::multi_index::sequenced<
				boost::multi_index::tag<index_seq>
			>
		>
	> Pins;
	/**
	 * Holds configuration data for a single digital port.
	 */
	struct Port {
		/**
		 * The attached DigitalPort. This will be empty after parsing and prior
		 * to attachment.
		 */
		std::shared_ptr<DigitalPort> dport;
		/**
		 * The pins described by the configuration file. The Pin objects map
		 * global pin IDs to port specific IDs, and can request some pins be
		 * unavailable. If any mapping cannot be honored, the configuration
		 * should be rejected.
		 */
		Pins pins;
		/**
		 * A hint as to what DigitalPort implementation should be used. It can
		 * be ignored.
		 * @todo  Used for device file path in
		 *        duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(),
		 *        so maybe change the name.
		 */
		std::string typeval;
		/**
		 * The pin ID offset for the port; used to translate between global and
		 * port pin IDs.
		 */
		unsigned int idOffset;
		Port();
		void parse(
			const std::pair<const std::string, boost::property_tree::ptree> &item
		);
		/**
		 * Convenience function that provides the pin global ID index for the
		 * port's pins.
		 */
		const PinConfiguration::Pins::index<index_gid>::type &gidIndex() const {
			return pins.get<index_gid>();
		}
		/**
		 * Convenience function that provides the sequential index for the
		 * port's pins.
		 */
		const PinConfiguration::Pins::index<index_seq>::type &seqIndex() const {
			return pins.get<index_seq>();
		}
	};
	typedef std::unordered_map<std::string, Port>  PortMap;
	/**
	 * Holds configuration data for a single chip select manager.
	 */
	struct SelMgr {
		/**
		 * The select manager based on this configuration. It is created after
		 * the attachment of the DigitalPort that supplies its pins.
		 */
		std::shared_ptr<ChipSelectManager> csm;
		/**
		 * The global IDs of the pins that this select manager should use. The
		 * pins must all come from the same port.
		 */
		std::vector<unsigned int> pins;
		/**
		 * A mapping of a name for a chip select to the chip ID used by the
		 * select manager.
		 */
		std::unordered_map<std::string, unsigned int> selNames;
		/**
		 * The port that will provide the pins for this select manager.
		 */
		Port *usePort;
		/**
		 * The type of chip select manager requested.
		 */
		enum MgrType {
			/**
			 * Not yet set or a bad value.
			 */
			Unknown,
			/**
			 * Use ChipBinarySelectManager.
			 */
			Binary,
			/**
			 * Use ChipMultiplexerSelectManager.
			 */
			Multiplexer,
			/**
			 * Use ChipPinSelectManager.
			 */
			Pin
		};
		/**
		 * The type of chip select manager requested.
		 */
		MgrType type;
		/**
		 * True for high selection state with binary manager, or for initially
		 * selected state with pin manager. No function otherwise.
		 */
		bool initSelHigh;
		SelMgr();
		SelMgr(
			const std::pair<const std::string, boost::property_tree::ptree> &item,
			const PinConfiguration *pinconf
		);
	};
	typedef std::unordered_map<std::string, SelMgr>  SelMgrMap;
	/**
	 * Holds configuration data for a single chip select.
	 */
	struct ChipSel {
		/**
		 * The chip select object for this configuration. It is configured after
		 * the attachment of the DigitalPort used by the chip select's manager.
		 */
		ChipSelect sel;
		/**
		 * The chip select manager configuration for this object.
		 */
		SelMgr *mgr;
		/**
		 * The chip ID that is selected by this chip select. IDs are only unique
		 * within a single manager.
		 */
		int chipId;
		ChipSel() = default;
		ChipSel(SelMgr *m, int id);
	};
	typedef std::unordered_map<std::string, ChipSel>  ChipSelMap;
	/**
	 * Holds configuration data for a single digital pin set.
	 */
	struct PinSet {
		/**
		 * The DigitalPinSet for this configuration. It is configured after the
		 * attachment of the DigitalPort that supplies the pins used by the set.
		 */
		DigitalPinSet dpSet;
		/**
		 * The pins used by this set.
		 */
		Pins pins;
		/**
		 * The name of an optional chip select that has been associated with
		 * this pin set. If there is no associated chip select, this string will
		 * be empty.
		 */
		std::string selName;
		/**
		 * The port that will provide the pins for this set.
		 */
		Port *usePort;
		PinSet() = default;
		PinSet(
			const std::pair<const std::string, boost::property_tree::ptree> &item,
			const PinConfiguration *pinconf
		);
		/**
		 * Convenience function that provides the sequential index for the
		 * sets's pins.
		 */
		const PinConfiguration::Pins::index<index_seq>::type &seqIndex() const {
			return pins.get<index_seq>();
		}
		PinConfiguration::Pins::index<index_seq>::type::iterator begin() {
			return pins.get<index_seq>().begin();
		}
		PinConfiguration::Pins::index<index_seq>::type::iterator end() {
			return pins.get<index_seq>().end();
		}
	};
	typedef std::unordered_map<std::string, PinSet>  PinSetMap;
private:
	/**
	 * All pins mentioned in the confiuration across all ports.
	 */
	Pins allpins;
	/**
	 * Port configurations stored by name.
	 */
	PortMap ports;
	/**
	 * Select manager configurations stored by name.
	 */
	SelMgrMap selMgrs;
	/**
	 * Chip select configurations stored by name.
	 */
	ChipSelMap chipSels;
	/**
	 * Pin set configurations stored by name.
	 */
	PinSetMap pinSets;
public:
	/**
	 * Make an empty configuration.
	 */
	PinConfiguration() = default;
	/**
	 * Constructs and parses the pin configuration that starts at the given
	 * subtree.
	 *
	 * [Boost property tree](https://www.boost.org/doc/libs/1_67_0/doc/html/property_tree.html)
	 * is used to parse and represnt data prior to passing that data to this
	 * object. See the @ref DUDStoolsPinConf page for detailed documentation
	 * on what is expected of the property tree.
	 *
	 * @param pt  The property tree node that is the parent of all the pin
	 *            configuration nodes. No boost::property_tree::ptree objects
	 *            are retained in this class.
	 * @throw PortDuplicateError
	 * @throw SelectManagerDuplicateError
	 * @throw SetDuplicateError
	 * @throw PinBadIdError
	 * @throw PortDuplicatePinIdError
	 * @throw SelectBadStateError
	 * @throw SelectManagerUnknownTypeError
	 * @throw SelectDuplicateError
	 * @throw SelectNoPinsError
	 * @throw SelectMultiplePortsError
	 * @throw SetMultiplePortsError
	 * @throw SelectDoesNotExistError
	 */
	PinConfiguration(const boost::property_tree::ptree &pt);
	/**
	 * Parses the pin configuration that starts at the given subtree.
	 *
	 * [Boost property tree](https://www.boost.org/doc/libs/1_67_0/doc/html/property_tree.html)
	 * is used to parse and represnt data prior to passing that data to this
	 * function. See the @ref DUDStoolsPinConf page for detailed documentation
	 * on what is expected of the property tree.
	 *
	 * @param pt  The property tree node that is the parent of all the pin
	 *            configuration nodes. No boost::property_tree::ptree objects
	 *            are retained in this class.
	 * @throw PortDuplicateError
	 * @throw SelectManagerDuplicateError
	 * @throw SetDuplicateError
	 * @throw PinBadIdError
	 * @throw PortDuplicatePinIdError
	 * @throw SelectBadStateError
	 * @throw SelectManagerUnknownTypeError
	 * @throw SelectDuplicateError
	 * @throw SelectNoPinsError
	 * @throw SelectMultiplePortsError
	 * @throw SetMultiplePortsError
	 * @throw SelectDoesNotExistError
	 */
	void parse(const boost::property_tree::ptree &pt);
	/**
	 * Attaches the given DigitalPort to the named port in the configuration.
	 * The port must be configured to have or not have the pins explicitly
	 * listed in the configuration. After checking that, objects in the
	 * configuration that need the port are created.
	 * @pre         Configuration parsing has already been done successfully.
	 * @param dp    The DigitalPort to attach to the configuration.
	 * @param name  The name given to the port in the configuration.
	 * @throw DigitalPortDoesNotExistError  @a dp is empty.
	 * @throw PortDoesNotExistError         The configuration does not have a
	 *                                      port with the given name.
	 * @throw DigitalPortHasPinError        A pin configured with the port ID
	 *                                      of "none" exists in the given port.
	 * @throw DigitalPortLacksPinError      A pin configured to exist is
	 *                                      missing from the given port.
	 */
	void attachPort(
		const std::shared_ptr<DigitalPort> &dp,
		const std::string &name = "default"
	);
	/**
	 * Finds the pin from the given name or global ID according to this pin
	 * configuration. The result is independent of any DigitalPort objects.
	 * @pre         Configuration parsing has already been done successfully.
	 * @param  str  A string with either an arbitrary name or a number with a
	 *              global ID.
	 * @return The Pin object that corresponds to the requested pin.
	 */
	const Pin &pin(const std::string &str) const;
	/**
	 * Finds the global ID of the given pin according to this pin configuration.
	 * The result is independent of any DigitalPort objects.
	 * @pre         Configuration parsing has already been done successfully.
	 * @param  str  A string with either an arbitrary name or a number with a
	 *              global ID.
	 * @return The global ID of the requested pin.
	 * @throw  PortBadPinIdError  The requested pin does not exist.
	 */
	unsigned int pinGlobalId(const std::string &str) const {
		return pin(str).gid;
	}
	/**
	 * Finds the configuration data for the named DigitalPort.
	 * @param name  The name given to the port in the configuration.
	 * @pre         Configuration parsing has already been done successfully.
	 * @throw PortDoesNotExistError
	 */
	const Port &port(const std::string &name = "default") const;
	/**
	 * Finds the configuration data for the named DigitalPinSet.
	 * @param name  The name given to the pin set in the configuration.
	 * @pre         Configuration parsing has already been done successfully.
	 *              PinSet::dpSet will only be valid after its required port
	 *              has been attached.
	 * @throw SetDoesNotExistError     The requested set is not defined in the
	 *                                 configuration data.
	 */
	const PinSet &pinSet(const std::string &name) const;
	/**
	 * Finds the configuration data for the named ChipSelect.
	 * @param name  The name given to the chip select in the configuration.
	 * @pre         Configuration parsing has already been done successfully.
	 *              ChipSel::sel will only be valid after its required port
	 *              has been attached.
	 * @throw SelectDoesNotExistError  The requested chip select is not defined
	 *                                 in the configuration data.
	 */
	const ChipSel &chipSelect(const std::string &name) const;
	/**
	 * Finds the configuration data for the named ChipSelectManager.
	 * @param name  The name given to the manager in the configuration.
	 * @pre         Configuration parsing has already been done successfully.
	 *              SelMgr::csm will only be valid after its required port
	 *              has been attached.
	 * @throw SelectManagerDoesNotExistError  The requested chip select manager
	 *                                        is not defined in the
	 *                                        configuration data.
	 */
	const SelMgr &selectManager(const std::string &name) const;
	/**
	 * Gets the DigitalPinSet and ChipSelect objects that are attached to the
	 * named set configuration.
	 * @pre  A configuration has been successfully parsed, and the DigitalPort
	 *       object(s) needed for the set have been attached.
	 * @param dpset    The object that will receive the DigitalPinSet
	 *                 configuration.
	 * @param sel      The object that will receive the associated ChipSelect
	 *                 object. If there is no configured chip select, the
	 *                 object's @ref ChipSelect::configured() "configured()"
	 *                 function will return false.
	 * @param setName  The name given to the pin set in the configuration.
	 * @throw SetDoesNotExistError     The requested set is not defined in the
	 *                                 configuration data.
	 * @throw SelectDoesNotExistError  The requested chip select is not defined
	 *                                 in the configuration data.
	 * @throw SetNotCreatedError       The requested set has not yet been
	 *                                 created. This most likely means that the
	 *                                 port providing its pins hasn't been
	 *                                 attached by a call to attachPort().
	 * @throw SelectManagerNotCreated  The requested manager for the chip select
	 *                                 has not yet been created. This most
	 *                                 likely means that the port providing its
	 *                                 pins hasn't been attached by a call to
	 *                                 attachPort().
	 */
	void getPinSetAndSelect(
		DigitalPinSet &dpset,
		ChipSelect &sel,
		const std::string &setName
	) const;
	/**
	 * Gets the DigitalPinSet object named in the configuration.
	 * @pre  A configuration has been successfully parsed, and the DigitalPort
	 *       object needed for the set has been attached.
	 * @param setName  The name given to the pin set in the configuration.
	 * @throw SetDoesNotExistError     The requested set is not defined in the
	 *                                 configuration data.
	 * @throw SetNotCreatedError       The requested set has not yet been
	 *                                 created. This most likely means that the
	 *                                 port providing its pins hasn't been
	 *                                 attached by a call to attachPort().
	 */
	const DigitalPinSet &getPinSet(const std::string &setName) const;
	/**
	 * Gets the ChipSelect object named in the configuration.
	 * @pre  A configuration has been successfully parsed, and the DigitalPort
	 *       object needed for the set has been attached.
	 * @param selName  The name given to the chip select in the configuration.
	 * @throw SelectDoesNotExistError  The requested chip select is not defined
	 *                                 in the configuration data.
	 * @throw SelectManagerNotCreated  The requested manager for the chip select
	 *                                 has not yet been created. This most
	 *                                 likely means that the port providing its
	 *                                 pins hasn't been attached by a call to
	 *                                 attachPort().
	 */
	const ChipSelect &getSelect(const std::string &selName) const;
	/**
	 * Makes a DigitalPin object for the named pin in the configuration.
	 * @pre  A configuration has been successfully parsed, and the DigitalPort
	 *       object that handles the pin has been attached.
	 * @param pinName  The name of the pin in the configuration.
	 * @throw PinBadIdError          The configuration does not define a pin
	 *                               with the given name.
	 * @throw PortDoesNotExistError  The port that supplies the pin has not
	 *                               been attached to this configuration.
	 * @throw PinDoesNotExist        The attched DigitalPort object claims to
	 *                               not have the pin.
	 */
	DigitalPin getPin(const std::string &pinName) const;
	/**
	 * True if the named chip select has been found in the already parsed
	 * configuration. This is used inside the parsing code, but can be used
	 * elsewhere.
	 * @param name  The name given to the chip select in the configuration.
	 */
	bool haveChipSelect(const std::string &name) const {
		return chipSels.count(name) > 0;
	}
};

} } }
