/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef DIGITALPORT_HPP
#define DIGITALPORT_HPP

#include <duds/hardware/interface/DigitalPinCap.hpp>
#include <duds/hardware/interface/DigitalPinAccessBase.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace duds { namespace hardware { namespace interface {

class DigitalPinAccess;
class DigitalPinSetAccess;

/**
 * Represents an interface to a group of hardware related digital GPIO lines.
 *
 * Derived classes will implement actual use of GPIO hardware.
 *
 * All derived non-abstract classes should call shutdown() in their destructors.
 * This function will first wait for all pins to become available so that any
 * users of the pins may finish. Then it waits on any threads that might be
 * hoping to obtain pin access to realize that it won't happen. It is safest to
 * de-initialize hardware after the shutdown function returns.
 *
 * Some functions come in pairs where one function has "Impl" at the end of its
 * name. The "Impl" functions contain the actual implementation, are not
 * public, and do not lock the port's @a block mutex. The non-Impl functions
 * may be public, and lock @b block before calling the corresponding Impl
 * function.
 *
 * @todo  Investigate ways to limit the locking of @a block before using
 *        @a pins. Some read operations likely do not need a lock if certain
 *        conditions can be met.
 *
 * @author  Jeff Jackowski
 */
class DigitalPort :
	boost::noncopyable,
	public std::enable_shared_from_this<DigitalPort>
{
	/**
	 * Used to serialize access to internal data for thread-safe operation.
	 * @todo  Consider blocking access to port config and port I/O separately.
	 */
	mutable std::mutex block;
	/**
	 * Used to efficently wait for resources to become available.
	 */
	std::condition_variable pinwait;
	friend DigitalPinAccess;
	friend DigitalPinSetAccess;
protected:
	/**
	 * Data stored for each pin controlled by the port.
	 */
	struct PinEntry {
		// add boost::signals2::signal objects for events
		/**
		 * A pointer to an access object, or nullptr if no access object for the
		 * pin exists. The object may be either a DigitalPinAccess or a
		 * DigitalPinSetAccess object.
		 */
		DigitalPinAccessBase *access;
		/**
		 * The configuration for the pin. Derived classes are responsible for
		 * initializing this value.
		 */
		DigitalPinConfig conf;
		/**
		 * The capabilities of the pin. Derived classes are responsible for
		 * initializing this value.
		 */
		DigitalPinCap cap;
		/**
		 * Initializes @a access to nullptr; other fields are left uninitialized.
		 */
		PinEntry() : access(nullptr) { }
		/**
		 * Initializes @a access to nullptr and @a cap using the values
		 * specified. @a conf is left uninitialized.
		 */
		PinEntry(const DigitalPinCap::Flags capf, std::uint16_t cur) :
			access(nullptr), cap(capf, cur) { }
		/**
		 * The object evaluates to false if the pin does not exist for use by
		 * this process.
		 */
		operator bool () const {
			return cap.exists();
		}
		/**
		 * Modify the pin to be non-existent.
		 */
		void markNonexistent() {
			cap = NonexistentDigitalPin;
			conf = DigitalPinConfig(DigitalPinConfig::ClearAll());
		}
	};
	typedef std::vector<PinEntry>  PinVector;
	/**
	 * Data on each pin handled by the port. The index of each pin is the local
	 * pin ID.
	 */
	PinVector pins;
private:
	/**
	 * An offset used to translate pin identification numbers between global
	 * scope and local scope. The local scope numbers always start at zero.
	 */
	unsigned int idOffset;
	/**
	 * A count of the threads waiting to access pins.
	 * @note  This should only be modifid when @a block is locked.
	 */
	int waiting;
	/**
	 * Used by the shutdown functions to force any thread waiting on access to
	 * pins to quit waiting and fail to get access.
	 */
	void shutdownAccessRequests();
	/**
	 * Checks a set of pins to see if they are all currently available.
	 * @pre  @a block is locked.
	 * @return  True if all pins are available; false otherwise.
	 */
	bool areAvailable(const unsigned int *reqpins, std::size_t len);
	/**
	 * Waits for all pins to become available.
	 * @pre  @a lock has a lock on @a block.
	 */
	void waitForAvailability(
		std::unique_lock<std::mutex> &lock,
		const unsigned int * const reqpins,  // global IDs
		const std::size_t len
	);
	/**
	 * Transfers or relinquishes access to pins.
	 * @param oldAcc  The access object that still has access.
	 * @param newAcc  A pointer to the access object to which access will be
	 *                transfered, or nullptr if access is relinquished.
	 * @post          Internal port data is updated for either @a newAcc to
	 *                have access, or the pins to be available. If available,
	 *                any threads waiting on access will be notified.
	 *                @a oldAcc must either be destroyed or modified to no
	 *                longer claim access. @a newAcc, if not nullptr, must
	 *                be updated to claim access.
	 */
	void updateAccess(const DigitalPinAccess &oldAcc, DigitalPinAccess *newAcc);
	/**
	 * Transfers or relinquishes access to pins.
	 * @param oldAcc  The access object that still has access.
	 * @param newAcc  A pointer to the access object to which access will be
	 *                transfered, or nullptr if access is relinquished.
	 * @post          Internal port data is updated for either @a newAcc to
	 *                have access, or the pins to be available. If available,
	 *                any threads waiting on access will be notified.
	 *                @a oldAcc must either be destroyed or modified to no
	 *                longer claim access. @a newAcc, if not nullptr, must
	 *                be updated to claim access.
	 */
	void updateAccess(
		const DigitalPinSetAccess &oldAcc,
		DigitalPinSetAccess *newAcc
	);
protected:
	/**
	 * Initializes internal data.
	 * @param numpins  The number of pre-allocated elements to make in @a pins.
	 * @param firstid  The global ID of the pin at index zero of @a pins.
	 */
	DigitalPort(unsigned int numpins, unsigned int firstid);
	/**
	 * Waits for access to all pins so that any user of access objects may
	 * finish with their operation, then destroys all pin data and awakens
	 * threads waiting on access, but they will not receive access. This
	 * function should be called early, likely first, in the destructors of
	 * derived classes. This will allow operations in progress to complete.
	 *
	 * This function is @b not called in ~DigitalPort(). By that time, the
	 * derived implementation will have destructed, which will prevent any
	 * access object from working properly. The access objects must be destroyed
	 * before then to avoid bad behavior, such as a process crash.
	 */
	void shutdown();

	/* *  UnimplementedError
	 * Adds pins to an already constructed port. The pins will be initialized
	 * to an unavailable state to avoid use until after the implementation has
	 * prepared the pin data for use.
	 * @throw PinExists
	 */
	//void addPins(unsigned int maxId);
	/* *  UnimplementedError
	 * Removes a pin from the port by changing its data to an unavailable state.
	 * This function will block until the pin is not being accessed. Then it
	 * will change the pin's state to unavailble. It will not change the size
	 * of the vector @a pins.
	 */
	//void removePin(unsigned int localPinId);

	/**
	 * Returns a reference to a pin's configuration on behalf of an access
	 * object.
	 * @param localPinId  The local ID of the pin to query.
	 * @todo  Check fot use of this function.
	 */
	const DigitalPinConfig &configRef(unsigned int localPinId) const {
		return pins[localPinId].conf;
	}

	std::vector<DigitalPinCap> capabilities(
		const std::vector<unsigned int> &pvec, bool global
	) const;
	virtual DigitalPinRejectedConfiguration::Reason proposeConfigImpl(
		unsigned int gid,
		DigitalPinConfig &pconf,
		DigitalPinConfig &iconf
	) const = 0;
	virtual bool proposeConfigImpl(
		const std::vector<unsigned int> &localPinIds,
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const = 0;
	virtual bool proposeFullConfigImpl(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const = 0;

	/**
	 * Modifies the configuration of a single pin with an independent
	 * configuration. Unless the implementation considers all configurations
	 * to be dependent, and configurations are always modified one pin at a
	 * time, then this function must be implemented.
	 * @param  globalPinId  The global pin ID; matches the index in the @a pins
	 *                      and @a proposed vectors.
	 * @param  cfg          The requested new configuration.
	 * @param pdata         A pointer to the port specific data stored in the
	 *                      corresponding access object for the pin.
	 * @return              The actual new configuration.
	 */
	DigitalPinConfig modifyConfig(
		unsigned int globalPinId,
		const DigitalPinConfig &cfg,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Does the work of the modifyConfig() functions in the case that the whole
	 * port configuration must be considered for possible changes. If
	 * proposeConfigImpl() reports the configuration is good,
	 * configurePort(const std::vector<DigitalPinConfig> &) will be called.
	 * @pre  There is already a lock on the port's internal data.
	 * @param propConf  The new proposed configuration. It must be the same size
	 *                  as @a pins. The values will be modified to be the actual
	 *                  applied configuration. Modification can occur even if
	 *                  an exception is thrown.
	 * @param initConf  The initial and current configuration. It must be the
	 *                  result of configurationImpl(). Included here to allow
	 *                  modifyConfig(DigitalPinConfig &, unsigned int)
	 *                  to fill the vector and use it as the starting point
	 *                  for @a propConf while avoiding a second call to
	 *                  configurationImpl() since a vector copy will be faster.
	 * @param pdata     A pointer to the port specific data stored in the
	 *                  corresponding access object for the pin.
	 */
	void modifyFullConfig(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Modifies the configuration of multiple pins. This is used to support
	 * changing multiple configurations without requiring the DigitalPort user
	 * to make calls for each pin individually, and to handle dependent
	 * configurations.
	 * @param cfgs     The new configurations for each pin using local indexing
	 *                 (matches @a pins). Will be changed to match the size of
	 *                 @a pins and to hold the finialized configurations.
	 * @param pdata    A pointer to the port specific data stored in the
	 *                 corresponding access object for the pin.
	 * @note   The implementation should lock @a block prior to accessing
	 *         internal data, like @a pins.
	 */
	void modifyConfig(
		std::vector<DigitalPinConfig> &cfgs,
		DigitalPinAccessBase::PortData *pdata
	);
	void modifyConfig(
		const std::vector<unsigned int> &pvec,
		std::vector<DigitalPinConfig> &cfgs,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Changes the hardware configuration for a single pin.
	 * If this function does not throw, its caller will record the configuration
	 * change inside @a pins. It is only called after error checking is
	 * performed on its parameters.
	 * @param cfg          The new configuration.
	 * @param localPinId   The local ID for the pin to modify.
	 * @param pdata        A pointer to the port specific data stored in the
	 *                     corresponding access object for the pin.
	 */
	virtual void configurePort(
		unsigned int localPinId,
		const DigitalPinConfig &cfg,
		DigitalPinAccessBase::PortData *pdata
	) = 0;
	/**
	 * Changes the hardware configuration for the whole port.
	 * If this function does not throw, its caller will record the configuration
	 * change inside @a pins. It is only called after error checking is
	 * performed on its parameters.
	 * @param cfgs    The new configuration. The indices are the local pin IDs.
	 *                The size must match the size of @a pins.
	 * @param pdata   A pointer to the port specific data stored in the
	 *                corresponding access object for the pin.
	 */
	virtual void configurePort(
		const std::vector<DigitalPinConfig> &cfgs,
		DigitalPinAccessBase::PortData *pdata
	) = 0;

	std::vector<DigitalPinConfig> configuration(
		const std::vector<unsigned int> &pvec, bool global
	) const;

	/**
	 * Returns the configuration of all pins in the port.
	 * @return     A copy of the current configuration.
	 * @pre        The thread has a lock on @a block.
	 */
	std::vector<DigitalPinConfig> configurationImpl() const;
	/**
	 * Does error checking in advance of calling
	 * inputImpl(unsigned int) to read the input of the given pin.
	 * @param gid    The global ID of the pin to read.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pin.
	 * @throw PinDoesNotExist      The requested pin is not handled by
	 *                             this port.
	 * @throw PinWrongDirection    The pin is not configured as an input.
	 */
	bool input(unsigned int gid, DigitalPinAccessBase::PortData *pdata);
	/**
	 * Does error checking in advance of calling
	 * inputImpl(const std::vector<unsigned int> &) to read the input of a
	 * set of pins.
	 * @param pvec     The local IDs of the pins to read.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pins.
	 * @throw PinDoesNotExist      A requested pin is not handled by
	 *                             this port.
	 * @throw PinWrongDirection    A pin is not configured as an input.
	 * @return   The input from the pins.
	 */
	std::vector<bool> input(
		const std::vector<unsigned int> &pvec,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Reads input from the given pin.
	 * @pre   The pin is configured as an input.
	 * @param gid    The global ID of the pin to read.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pin.
	 */
	virtual bool inputImpl(
		unsigned int gid,
		DigitalPinAccessBase::PortData *pdata
	) = 0;
	/**
	 * Reads input from the requested pins.
	 *
	 * The implementation in DigitalPort calls input(unsigned int). This
	 * only makes sense for ports that do not support simultaneous operations.
	 * An assertion exists to prevent such misuse.
	 *
	 * @pre   All the pins are configured as inputs.
	 * @param pvec    The global IDs of the pins to read.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pins.
	 * @return   The input from the pins.
	 */
	virtual std::vector<bool> inputImpl(
		const std::vector<unsigned int> &pvec,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Does error checking in advance of calling
	 * outputImpl(unsigned int, bool) to change the output of the given pin.
	 * @param gid    The global ID of the pin to change.
	 * @param state  The new output state.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pin.
	 * @throw PinDoesNotExist              The requested pin is not handled by
	 *                                     this port.
	 * @throw DigitalPinCannotOutputError  The pin cannot be configured as an
	 *                                     output.
	 */
	void output(
		unsigned int gid,
		bool state,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Does error checking in advance of calling
	 * outputImpl(const std::vector<unsigned int> &,const std::vector<bool> &)
	 * to change the output of a set of pins.
	 * @param pvec   The local IDs of the pins to change.
	 * @param state  The new output state.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pins.
	 * @throw DigitalPinConfigRangeError   The @a pvec and @a state vectors are
	 *                                     different sizes.
	 * @throw PinDoesNotExist              A requested pin is not handled by
	 *                                     this port.
	 * @throw DigitalPinCannotOutputError  One of the pins cannot be configured
	 *                                     as an output.
	 */
	void output(
		const std::vector<unsigned int> &pvec,
		const std::vector<bool> &state,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Changes the output state of the given pin. If the pin is not configured
	 * as an output, its configuration will not change. However, this new state
	 * will be the output state once the configuration is changed to output.
	 * @pre   The pin is capable of output.
	 * @param lid    The local ID of the pin to change.
	 * @param state  The new output state.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pin.
	 */
	virtual void outputImpl(
		unsigned int lid,
		bool state,
		DigitalPinAccessBase::PortData *pdata
	) = 0;
	/**
	 * Changes the outputs of several pins. If any of the pins are not
	 * configured as an output, thier configuration will not change. However,
	 * this new state will be the output state once the configuration is
	 * changed to output.
	 *
	 * The implementation in DigitalPort calls output(unsigned int, bool). This
	 * only makes sense for ports that do not support simultaneous operations.
	 * An assertion exists to prevent such misuse.
	 *
	 * @pre   For all implementations: the vector parameters are the same size.
	 * @pre   For all implementations: The pins are capable of output.
	 * @pre   For this implementation only: simultaneous operations are not
	 *        supported.
	 * @param pvec   The local ID of the pins to alter.
	 * @param state  The new output states.
	 * @param pdata  A pointer to the port specific data stored in the
	 *               corresponding access object for the pins.
	 */
	virtual void outputImpl(
		const std::vector<unsigned int> &pvec,
		const std::vector<bool> &state,
		DigitalPinAccessBase::PortData *pdata
	);
	/**
	 * Called after a new access object is made to allow a port implementation
	 * to take further action. The call is made while there is a lock on
	 * @a block. The default implementation does nothing.
	 * @param acc  The newly made access object.
	 * @throw exception  Any thrown exception will cause the access object to
	 *                   be retired and then the exception will be rethrown.
	 */
	virtual void madeAccess(DigitalPinAccess &acc);
	/**
	 * Called after a new access object is made to allow a port implementation
	 * to take further action. The call is made while there is a lock on
	 * @a block. The default implementation does nothing.
	 * @param acc  The newly made access object.
	 * @throw exception  Any thrown exception will cause the access object to
	 *                   be retired and then the exception will be rethrown.
	 */
	virtual void madeAccess(DigitalPinSetAccess &acc);
	/**
	 * Called just before an access object is retired to allow a port
	 * implementation to take further action. The call is made while there is
	 * a lock on @a block. The default implementation does nothing.
	 * @param acc  The access object that will be retired.
	 */
	virtual void retiredAccess(const DigitalPinAccess &acc) noexcept;
	/**
	 * Called just before an access object is retired to allow a port
	 * implementation to take further action. The call is made while there is
	 * a lock on @a block. The default implementation does nothing.
	 * @param acc  The access object that will be retired.
	 */
	virtual void retiredAccess(const DigitalPinSetAccess &acc) noexcept;
	/**
	 * Returns a reference to the port specific data in the given
	 * DigitalPinAccessBase object. Allows classes derived from DigitalPort
	 * access to the DigitalPinAccessBase::portdata member.
	 * @param acc  The access object with the desired data.
	 * @return     A reference to the port specific data. Modifications are
	 *             allowed.
	 */
	static DigitalPinAccessBase::PortData &portData(
		const DigitalPinAccessBase &acc
	) {
		return acc.portdata;
	}
	/**
	 * Provides a pointer to type @a T stored in the port specific data of the
	 * given DigitalPinAccessBase object. No type checking is performed.
	 * @tparam T   The requested data type. This can normally be inferred from
	 *             the @a ptr parameter.
	 * @param acc  The access object with the desired data.
	 * @param ptr  A pointer to the pointer that will be given a copy of the
	 *             pointer stored in @a acc.
	 */
	template <typename T>
	static void portDataPtr(const DigitalPinAccessBase &acc, T **ptr) {
		*ptr = (T*)acc.portdata.pointer;
	}

public:
	/**
	 * Derived classes should call shutdown() early in their destructors. That
	 * will assure no pins are in use.
	 */
	virtual ~DigitalPort();
	/**
	 * True if the implementation supports operating on multiple pins
	 * simultaneously. If false, the interfaces for operating on multiple pins
	 * will still be available, but the pins may be modified on over a period
	 * of time in an implementation defined order.
	 */
	virtual bool simultaneousOperations() const = 0;
	/**
	 * Returns the capabilities of a pin.
	 * @param globalPinId  The global ID of the pin to query.
	 * @return             A copy of the capabilities.
	 * @throw PinDoesNotExist   The requested pin does not exist within this
	 *                          port.
	 */
	DigitalPinCap capabilities(unsigned int globalPinId) const;
	/**
	 * Returns the capabilities of all pins in the port.
	 * @return     A copy of the capabilities.
	 */
	std::vector<DigitalPinCap> capabilities() const;
	/**
	 * Returns the capabilities of all the pins requested by global ID.
	 * @param pvec  A vector of global pin IDs.
	 * @return      A vector of pin capabilities ordered the same as
	 *              @a pvec. If there are any gaps in @a pvec (an element with
	 *              -1), the corresponding capability will be
	 *              NonexistentDigitalPin.
	 * @throw PinDoesNotExist  One of the pin IDs in @a pvec is for a pin
	 *                         that is not in this port.
	 */
	std::vector<DigitalPinCap> capabilities(
		const std::vector<unsigned int> &pvec
	) const {
		return capabilities(pvec, true);
	}
	/**
	 * Returns the capabilities of all the pins requested by local ID.
	 * @param pvec  A vector of local pin IDs.
	 * @return      A vector of pin capabilities ordered the same as
	 *              @a pvec. If there are any gaps in @a pvec (an element with
	 *              -1), the corresponding capability will be
	 *              NonexistentDigitalPin.
	 * @throw PinDoesNotExist  One of the pin IDs in @a pvec is for a pin
	 *                         that is not in this port.
	 * @note  Most public functions of this class take global pin IDs, which
	 *        is why this one has a name making it explicit that it takes
	 *        local pin IDs. This is needed to support DigitalPinSetAccess.
	 */
	std::vector<DigitalPinCap> capabilitiesLocalIds(
		const std::vector<unsigned int> &pvec
	) const {
		return capabilities(pvec, false);
	}
	/**
	 * Returns the current configuration of a pin.
	 * @param globalPinId  The global ID of the pin to query.
	 * @return             A copy of the current configuration. The
	 *                     configuration could change on the request of another
	 *                     thread, so a reference cannot be used.
	 * @throw PinDoesNotExist   The requested pin does not exist within this
	 *                          port.
	 */
	DigitalPinConfig configuration(unsigned int globalPinId) const;
	/**
	 * Returns the configuration of all pins in the port.
	 * @return     A copy of the current configuration.
	 */
	std::vector<DigitalPinConfig> configuration() const;
	/**
	 * Returns the configuration of all the pins requested by global ID.
	 * @param pvec  A vector of global pin IDs.
	 * @return      A vector of pin configurations ordered the same as
	 *              @a pvec. If there are any gaps in @a pvec (an element with
	 *              -1), the corresponding configuration will be
	 *              DigitalPinConfig::OperationNoChange.
	 * @throw PinDoesNotExist  One of the pin IDs in @a pvec is for a pin
	 *                         that is not in this port.
	 */
	std::vector<DigitalPinConfig> configuration(
		const std::vector<unsigned int> &pvec
	) const {
		return configuration(pvec, true);
	}
	/**
	 * Returns the configuration of all the pins requested by local ID.
	 * @param pvec  A vector of local pin IDs.
	 * @return      A vector of pin configurations ordered the same as
	 *              @a pvec. If there are any gaps in @a pvec (an element with
	 *              -1), the corresponding configuration will be
	 *              DigitalPinConfig::OperationNoChange.
	 * @throw PinDoesNotExist  One of the pin IDs in @a pvec is for a pin
	 *                         that is not in this port.
	 * @note  Most public functions of this class take global pin IDs, which
	 *        is why this one has a name making it explicit that it takes
	 *        local pin IDs. This is needed to support DigitalPinSetAccess.
	 */
	std::vector<DigitalPinConfig> configurationLocalIds(
		const std::vector<unsigned int> &pvec
	) const {
		return configuration(pvec, false);
	}
	/**
	 * Proposes a configuration change for a single pin. If the port is an
	 * implementation of DigitalPortDependentPins, the change may require
	 * altering the configuration of other pins.
	 * @param gid    The global ID of the pin to consider.
	 * @param pconf  The proposed new configuration. It is modified to be
	 *               what the actual configuration would be.
	 * @param iconf  The proposed initial configuration. If the value is
	 *               DigitalPinConfig::OperationNoChange, the current
	 *               configuration of the port will be used.
	 * @return       The reason for rejecting the proposal, which may be
	 *               @ref DigitalPinRejectedConfiguration::NotRejected "NotRejected".
	 * @throw        DigitalPinConfigurationError  Thrown from
	 *               DigitalPinCap::compatible().
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int gid,
		DigitalPinConfig &pconf,
		DigitalPinConfig &iconf
	) const;
	/**
	 * Proposes a configuration change for a single pin from the port's current
	 * configuration. If the port is an implementation of
	 * DigitalPortDependentPins, the change may require
	 * altering the configuration of other pins.
	 * @param gid    The global ID of the pin to consider.
	 * @param pconf  The proposed new configuration.
	 * @return       The reason for rejecting the proposal, which may be
	 *               @ref DigitalPinRejectedConfiguration::NotRejected "NotRejected".
	 * @throw        DigitalPinConfigurationError  Thrown from
	 *               DigitalPinCap::compatible().
	 */
	DigitalPinRejectedConfiguration::Reason proposeConfig(
		unsigned int gid,
		DigitalPinConfig &pconf
	) const {
		// should these parameters really not be const?
		DigitalPinConfig onc = DigitalPinConfig::OperationNoChange;
		return proposeConfig(gid, pconf, onc);
	}
	/**
	 * @throw        DigitalPinConfigRangeError  The number of
	 *               configurations in @a propConf or @a initConf does not
	 *               match the number of pins in @a localPinIds. An empty
	 *               @a initConf will not cause this exception.
	 * @throw        DigitalPinConfigurationError  Thrown from
	 *               DigitalPinCap::compatible().
	 */
	bool proposeConfig(
		const std::vector<unsigned int> &globalPinIds,
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	bool proposeConfigLocalIds(
		const std::vector<unsigned int> &localPinIds,
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	// above with current config for initial
	bool proposeConfig(
		const std::vector<unsigned int> &pins,
		std::vector<DigitalPinConfig> &propConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const {
		std::vector<DigitalPinConfig> initConf;
		return proposeConfig(pins, propConf, initConf, insertReason);
	}
	bool proposeConfigLocalIds(
		const std::vector<unsigned int> &pins,
		std::vector<DigitalPinConfig> &propConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const {
		std::vector<DigitalPinConfig> initConf;
		return proposeConfigLocalIds(pins, propConf, initConf, insertReason);
	}
	bool proposeFullConfig(
		std::vector<DigitalPinConfig> &propConf,
		std::vector<DigitalPinConfig> &initConf,
		std::function<void(DigitalPinRejectedConfiguration::Reason)> insertReason
			= std::function<void(DigitalPinRejectedConfiguration::Reason)>()
	) const;
	/*
	// (index-idOffset) == pin ID to reconfigure
	template <std::size_t N>
	void configure(DigitalPinConfig (&a)[N]);
	typedef std::initializer_list<DigitalPinConfig>  ConfigList;
	void configure(const ConfigList &cfgs, int start = 0);
	void configure(const std::vector<DigitalPinConfig> &cfgs, int start = 0);
	// use DigitalPinConfig with ID as key
	template <class C>
	void configure(const C &cfgs);
	// configure one pin
	void configure(unsigned int id, const DigitalPinConfig &cfg);
	*/
	/**
	 * Returns the offset for the port's pins. The value is the same as the
	 * global ID of the port's first pin, which has local ID zero. The value
	 * will not change for the lifetime of the DigitalPort object.
	 */
	unsigned int offset() const {
		return idOffset;
	}
	/**
	 * Returns the local ID for a pin given the global ID. The local ID is
	 * the one used to index into vectors of pins or related information.
	 * @param globalId  The global pin ID; the one used to identify the pin
	 *                  among all the pins available.
	 */
	unsigned int localId(unsigned int globalId) const {
		return globalId - idOffset;
	}
	/**
	 * Converts the provided global pin IDs to local pin IDs. Any gaps, or IDs
	 * of -1, are retained.
	 */
	std::vector<unsigned int> localIds(
		const std::vector<unsigned int> &globalIds
	) const;
	/**
	 * Returns the global ID for a pin given the local ID. The global ID is
	 * the one used to identify a specific pin on the hardware among all
	 * DigitalPort objects.
	 * @param localId  The local pin ID; the one used to index into vectors
	 *                 of pins or related information.
	 * @throw duds::general::ObjectDestructedError
	 */
	unsigned int globalId(unsigned int localId) const {
		return localId + idOffset;
	}
	/**
	 * Converts the provided local pin IDs to global pin IDs. Any gaps, or IDs
	 * of -1, are retained.
	 */
	std::vector<unsigned int> globalIds(
		const std::vector<unsigned int> &localIds
	) const;
	/**
	 * The maximum number of pins on the port. There may be fewer pins because
	 * not all pin IDs from offset() to (offset() + size()) may be populated.
	 */
	unsigned int size() const {
		return pins.size();
	}
	/**
	 * Returns true if the pin exists in this port.
	 * @param gid   The global pin ID to check.
	 */
	bool exists(unsigned int gid) const;
	/*
	 * Obtains a DigitalPin object from the port.
	 * @param id   The global pin ID to get.

	 @todo  REDO this !!!

	 */ /*
	std::shared_ptr<DigitalPin> pin(unsigned int id) const {
		std::lock_guard<duds::general::Spinlock> lock(spin);
		return pins.at(localId(id));
	} */
	/**
	 * Returns true if the given configuration for the given pin does not
	 * affect any other pins in the port. When true, the functions to change pin
	 * configurations will use simpler and faster code. When false, more
	 * complex logic will be used to assure the dependent configurations
	 * will still meet their requirements.
	 * @param gid      The global pin ID.
	 * @param newcfg   The proposed configuration to check.
	 * @param initcfg  The initial configuration for the pin.
	 * @throw PinDoesNotExist   The specified pin does not exist within this
	 *                          port.
	 */
	virtual bool independentConfig(
		unsigned int gid,
		const DigitalPinConfig &newcfg,
		const DigitalPinConfig &initcfg
	) const = 0;
	/**
	 * Returns true if all pins always have an independent configuration from
	 * all other pins. If so,
	 * independentConfig(unsigned int, const DigitalPinConfig &) must always
	 * return true for all valid inputs.
	 */
	virtual bool independentConfig() const = 0;

	void poll();
	// gets input state & records changes
	void pollInput();
	// calls event signals for changed pins
	void pollSignal();

	// ------------- Update docs -------------
	/**
	 * Obtain access objects to use a set of pins.
	 *
	 * The access objects to all requested pins are either all obtained, or
	 * not obtained. If not all the access objects can be obtained, and no
	 * exception is thrown, the calling thread will block and attempt to get
	 * the access objects later when they might all be available. Barring an
	 * exception or destruction of the PinStore, the thread will continue to
	 * block until the access objects are obtained.
	 *
	 * While the access objects will be obtained simultaneously, they are all
	 * independent from each other. They do not have to be relinquished at the
	 * same time, and can be relinquished in any order.
	 *
	 * This form is intended for use with arrays allocated on the stack, and
	 * with constant arrays for the requested pins. They might not be the safest
	 * of data structures, but they are fast.
	 *
	 * @param pins  An array of pin id values. All values must be for pins
	 *              in the port, and values <B>must not</B> be repeated.
	 *              The value @a -1 will be skipped over; the corresponding
	 *              item in @a acc will not be changed.
	 * @param len   The length of both the @a acc and @a pins arrays.
	 * @param acc   An array of unique pointers that will be assigned to the
	 *              access objects of the requested pins.
	 * @pre  @a acc contains constructed unique pointers.
	 * @throw PinEmptyAccessRequest
	 * @throw PinDoesNotExist   A requested pin does not exist within this
	 *                          port.
	 * @throw ObjectDestructedError  The PinStore object was destructed before the
	 *                          request could be fullfilled.
	 * @throw PinInUse          A pin was requested twice or more; there is a
	 *                          repeated value in @a pins. If thrown, @a acc
	 *                          may be modified.
	 */
	void access(
		const unsigned int *pins,
		const unsigned int len,
		std::unique_ptr<DigitalPinAccess> *acc
	);
	//  gcc probably won't figure out array length for acc
	template <std::size_t Len>
	void access(
		const unsigned int pins[Len],
		std::unique_ptr<DigitalPinAccess> acc[Len]
	) {
		access(pins, Len, acc);
	}
	std::unique_ptr<DigitalPinAccess> access(const unsigned int pin);
	/*
	 * Obtain access objects to use a set of pins.
	 *
	 * The access objects to all requested pins are either all obtained, or
	 * not obtained. If not all the access objects can be obtained, and no
	 * exception is thrown, the calling thread will block and attempt to get
	 * the access objects later when they might all be available. Barring an
	 * exception or destruction of the PinStore, the thread will continue to
	 * block until all the access objects are obtained.
	 *
	 * @param pins  An array of pin id values. All values must be for pins
	 *              already in @a store, and values <B>must not</B> be repeated.
	 *              The value @a -1 will be skipped over; the corresponding
	 *              object in @a acc will be made using the default constructor.
	 * @param len   The length of both the @a acc and @a pins arrays.
	 * @param acc   A unique pointer to an array of DigitalPinAccess objects.
	 * @pre         @a acc contains one of:
	 *                 - An allocated array of default constructed unused
	 *                   DigitalPinAccess objects of length @a len.
	 *                 - An allocated but uninitialized array long enough for
	 *                   @a len DigitalPinAccess objects.
	 *                 - An allocated array of destructed DigitalPinAccess
	 *                   objects of length @a len.
	 *                 - An empty unique pointer.
	 * @warning     No check is performed to assure any supplied constructed
	 *              access object is not already in use. The object's
	 *              @ref DigitalPinAccess::havePin() "havePin()" function
	 *              @b must return false. If this condition is not met, some
	 *              pins may be considered to be in-use as long as the process
	 *              continues to run, much like a memory leak.
	 * @post        If @a acc was empty, it will be given an array of length
	 *              @a len. If it was not empty, the given array will be used.
	 *              If a PinDoesNotExist or ObjectDestructedError exception is
	 *              thrown, @a acc will not be modified. Any other exception
	 *              may be thrown after @a acc is modified.
	 * @throw PinDoesNotExist   A requested pin does not exist within this
	 *                          port.
	 * @throw ObjectDestructedError  The PinStore object was destructed before the
	 *                          request could be fullfilled.
	 * @throw PinInUse          A pin was requested twice or more; there is a
	 *                          repeated value in @a pins. If thrown, @a acc
	 *                          may be modified.
	 */ /*
	void access(
		const unsigned int *pins,
		const unsigned int len
		std::unique_ptr<DigitalPinAccess[]> &acc,
	); */
	/**
	 * Obtain access objects to use a set of pins.
	 *
	 * The access objects to all requested pins are either all obtained, or
	 * not obtained. If not all the access objects can be obtained, and no
	 * exception is thrown, the calling thread will block and attempt to get
	 * the access objects later when they might all be available. Barring an
	 * exception or destruction of the PinStore, the thread will continue to
	 * block until the access objects are obtained.
	 *
	 * While the access objects will be obtained simultaneously, they are all
	 * independent from each other. They do not have to be relinquished at the
	 * same time, and can be relinquished in any order.
	 *
	 * @param pins  An array of pin id values. All values must be for pins
	 *              in the port, and values <B>must not</B> be repeated.
	 *              The value @a -1 will be skipped over; the corresponding
	 *              object in @a acc will be made using the default constructor.
	 * @param len   The length of both the @a acc and @a pins arrays.
	 * @param acc   An array of DigitalPinAccess objects.
	 * @pre         @a acc contains one of:
	 *                 - An allocated array of default constructed unused
	 *                   DigitalPinAccess objects of length @a len.
	 *                 - An allocated but uninitialized array long enough for
	 *                   @a len DigitalPinAccess objects.
	 *                 - An allocated array of destructed DigitalPinAccess
	 *                   objects of length @a len.
	 * @warning     No check is performed to assure any supplied constructed
	 *              access object is not already in use. If constructed, the
	 *              object's @ref DigitalPinAccess::havePin() "havePin()"
	 *              function @b must return false. If this condition is not met,
	 *              some pins may be considered to be in-use as long as the
	 *              process continues to run, much like a memory leak.
	 * @post        If a PinDoesNotExist or ObjectDestructedError exception is
	 *              thrown, @a acc will not be modified. Any other exception
	 *              may be thrown after @a acc is modified.
	 * @throw PinEmptyAccessRequest
	 * @throw PinDoesNotExist   A requested pin does not exist within this
	 *                          port.
	 * @throw ObjectDestructedError  The PinStore object was destructed before the
	 *                          request could be fullfilled.
	 * @throw PinInUse          A pin was requested twice or more; there is a
	 *                          repeated value in @a pins. If thrown, @a acc
	 *                          may be modified.
	 */
	void access(
		const unsigned int *pins,
		const unsigned int len,
		DigitalPinAccess *acc
	);
	void access(
		const std::vector<unsigned int> &pins,
		DigitalPinAccess *acc
	) {
		access(&(pins[0]), pins.size(), acc);
	}

	// had trouble with one length; not sure two will work
	/**  This is a nice idea that may fail at compile time. */
	template <std::size_t alen, std::size_t plen>
	void access(const unsigned int pins[plen], DigitalPinAccess acc[alen]) {
		static_assert(alen == plen,
			"The arrays must have the same number of objects.");
		access(pins, alen, acc);
	}
	/**
	 * Obtain access objects to use a set of pins.
	 *
	 * The access object will be granted use of all requested pins
	 * simultaneously. The thread will block until all pins can be acquired.
	 * If the access object already has pins, the requested pins will be
	 * appended. If a requested pin is already available through the given
	 * access object, the calling thread will be deadlocked.
	 *
	 * @param pins  An array of global pin ID values. All values must be for
	 *              pins in the port, and values <B>must not</B> be repeated.
	 *              The value @a -1 will be skipped over; the corresponding
	 *              spot in @a acc will be given -1 and will not represent a
	 *              usable pin.
	 * @pre         None of the pins specified in the @a pins array are already
	 *              available through @a acc. Failure to meet this requirement
	 *              will cause the calling thread to be deadlocked.
	 * @param len   The length of the @a pins array.
	 * @param acc   A DigitalPinSetAccess object.
	 * @post        If a PinDoesNotExist, PinSetWrongPort, or ObjectDestructedError
	 *              exception is thrown, @a acc will not be modified. Any other
	 *              exception may be thrown after @a acc is modified. Any
	 *              modification will leave @a acc in a valid state, and
	 *              may give it access to a subset of the requested pins.
	 * @throw PinEmptyAccessRequest
	 * @throw PinDoesNotExist   A requested pin does not exist within this
	 *                          port.
	 * @throw PinSetWrongPort   The given access object has already been used
	 *                          with another port.
	 * @throw ObjectDestructedError  The PinStore object was destructed before the
	 *                          request could be fullfilled.
	 * @throw PinInUse          A pin was requested twice or more; there is a
	 *                          repeated value in @a pins. If thrown, @a acc
	 *                          may be modified.
	 */
	void access(
		const unsigned int *pins,
		const unsigned int len,
		DigitalPinSetAccess &acc
	);
	void access(
		const unsigned int *begin,
		const unsigned int *end,
		DigitalPinSetAccess &acc
	) {
		access(begin, end - begin, acc);
	}
	void access(
		const std::vector<unsigned int> &pins,
		DigitalPinSetAccess &acc
	) {
		access(&(pins[0]), pins.size(), acc);
	}
	std::unique_ptr<DigitalPinSetAccess> access(
		const std::vector<unsigned int> &pins
	);
	// Would need forward iterators, not input iterators, to use the iterators
	// instead of vector. Will traverse range at least twice.
	template <class InputIter>
	void access(
		const InputIter &begin,
		const InputIter &end,
		DigitalPinSetAccess &acc
	) {
		std::vector<unsigned int> pvec(begin, end);
		access(&(pvec[0]), pvec.size(), acc);
	}

	// ------------- Update above docs -------------

	/*
	Todo:
	Maybe have a function to handle polling for events that can be used
	in place of or in addition to interrupts?
	*/

};

/**
 * Added to exceptions thrown by DigitalPort objects.
 */
typedef boost::error_info<struct Info_DigitalPortAffected, const DigitalPort*>
	DigitalPortAffected;

/*

Why is this here?

inline unsigned int DigitalPinAccess::globalId() const {
	return port->globalId(pid);
}
inline const DigitalPinConfig &DigitalPinAccess::config() const noexcept {
	return port()->configRef(pid);
}

inline void DigitalPinAccess::configure(const DigitalPinConfig &conf) {
	port->configure(pid, conf);
}

inline unsigned int DigitalPinSetAccess::globalId(unsigned int pos) const {
	unsigned int lid = pins.at(pos);
	if (lid == -1) {
		return lid;
	} else {
		return port->globalId(lid);
	}
}

*/

} } }

#endif        //  #ifndef DIGITALPORT_HPP
