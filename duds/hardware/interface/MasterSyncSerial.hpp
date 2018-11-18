/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef MASTERSYNCSERIAL_HPP
#define MASTERSYNCSERIAL_HPP

#include <cstdint>
#include <memory>
#include <boost/noncopyable.hpp>
#include <duds/general/BitFlags.hpp>
#include <duds/general/DataSize.hpp>
#include <duds/hardware/interface/Conversationalist.hpp>

namespace duds { namespace hardware { namespace interface {

class MasterSyncSerialAccess;

/**
 * An abstraction for the master side of a simple synchronous serial
 * communication connection to some device. The abstraction is only for
 * working with a single device; it is expected that multiple such objects
 * will be used for each device on a bus. This abstraction is not intended for
 * use with more complex protocols like I2C.
 *
 * The abstraction and any similar kind of bus object should not be used for
 * a device if the bus is unimportant to accessing said device.
 * For instance, if an SPI device with Linux kernel support for the device is
 * used, the kernel's device driver should, and likely must, be used to work 
 * with the device. However, if the kernel is unaware of the device and the
 * program supplies its own driver, a MasterSyncSerial object can handle the
 * low-level communication.
 *
 * Single MasterSyncSerial objects do not allow for thread-safe use because
 * they are intended to communicate with a single device and such communication
 * typically does not work well or make sense to implement with multiple
 * threads. However, multiple MasterSyncSerial objects can be thread-safe. Even
 * if resources are shared between MasterSyncSerial objects, they must use
 * those resources in a thread-safe manner. DigitalPinMasterSyncSerial, for
 * instance, must obtain the DigitalPinSetAccess objects for all of its pins in
 * its open() function.
 *
 * @par State transitions
 * @dot
 * digraph MssStates {
 * 	graph [ bgcolor="transparent" ];
 * 	node [ fontname="Helvetica", shape=record, fontsize=10, height=0.2,
 * 		fillcolor="white", style="filled" ];
 * 	edge [ fontname="Helvetica", fontsize=10 ];
 * 	Start    [ shape=point, style=filled, fillcolor=black ];
 * 	Stop     [ shape=doublecircle, style=filled, fillcolor=black, label="" ];
 * 	// states
 * 	NotReady [ label="Not ready" ];
 * 	Ready    [ URL="\ref MssReady" ];
 * 	Open     [ URL="\ref MssOpen" ];
 * 	Communicating [ URL="\ref MssCommunicating" ];
 * 	// transitions
 * 	Start -> NotReady;
 * 	NotReady -> Ready [ label="Implementation defined\nrequirements met" ];
 * 	Ready -> Open [ label="Gain\naccess", URL="\ref access()" ];
 * 	Open -> Communicating [ label="Start\nconversation",
 * 		URL="\ref MasterSyncSerialAccess::start()" ];
 * 	Communicating -> Open [ label="Stop\nconversation",
 * 		URL="\ref MasterSyncSerialAccess::stop()" ];
 * 	Communicating -> Ready [ label="Lose\naccess" ];
 * 	Open -> Ready [ label="Lose\naccess" ];
 * 	Ready -> Stop [ label="Destruct" ];
 * }
 * @enddot
 * @par
 * When a MasterSyncSerial object is constructed, it is not ready. The
 * transition to the ready (MssReady) state is handled by derived classes
 * in an implementation defined manner. The ready state is checked within this
 * class to prevent misuse. Obtaining an access object (MasterSyncSerialAccess)
 * causes a transition from the ready to the open (MssOpen) state. This is
 * intended to allow the required resources to be locked by this object and
 * be put into a non-communicating idle state. The destruction of the access
 * object will cause a transition back to the ready state. The access object
 * may be used to transition from the open to the communicating
 * (MssCommunicating) state and back multiple times. This allows starting
 * and stoping conversations with the other end, which may entail some kind
 * of start/stop condition or selecting and deselecting a device. By keeping
 * the open and communicating states separate, it is possible to guarantee
 * that several conversations with the device will occur without an
 * interruption for communicating with another device using some of the
 * same resources.
 *
 * @author  Jeff Jackowski.
 */
class MasterSyncSerial : public Conversationalist, boost::noncopyable,
public std::enable_shared_from_this<MasterSyncSerial> {
	/**
	 * The access object calls select(), deselect(), and
	 * retire(MasterSyncSerialAccess *).
	 */
	friend class MasterSyncSerialAccess;
	/**
	 * A pointer to the current access object or nullptr.
	 */
	MasterSyncSerialAccess *mssacc;
	/**
	 * Removes the access object from use.
	 * @throw SyncSerialInvalidAccess  @a acc is not the current access object.
	 * @todo  Maybe only throw in debug builds.
	 */
	void retire(MasterSyncSerialAccess *acc);
public:
	/**
	 * Configuration flags for various synchronous serial options.
	 */
	typedef duds::general::BitFlags<struct MssFlags> Flags;
	/**
	 * Use a select line to tell a device to pay attention to the master.
	 */
	static constexpr Flags MssUseSelect = Flags(1);
	/**
	 * Before communication begins, and after it ends, the clock line should
	 * have a high logic level.
	 */
	static constexpr Flags MssClockIdleHigh = Flags(2);
	/**
	 * Output on the falling edge of the clock and read on the rising edge.
	 */
	static constexpr Flags MssOutFallInRise = Flags(4);
	/**
	 * Send data MSb first, little endian.
	 */
	static constexpr Flags MssMSbFirst = Flags(8);
	/**
	 * Communication is full duplex.
	 */
	static constexpr Flags MssFullDuplex = Flags(16);
	/**
	 * All the flags that are used for configuration rather than the current
	 * state.
	 */
	static constexpr Flags MssConfigMask = Flags(31);
	/**
	 * Indictates that all required resources for communication have been
	 * identified and passed a validity check. The MasterSyncSerial class
	 * clears this flag when constructed; after that, the flag is only
	 * changed by derived classes. Several functions in this base class use
	 * this flag to check for improper usage.
	 */
	static constexpr Flags MssReady = Flags(32);
	/**
	 * Indicates that all required resources for communication have been
	 * acquired. These are initally put into a non-communicating state.
	 */
	static constexpr Flags MssOpen = Flags(64);
	/**
	 * Indicates that communication is underway. If a select line is used, it
	 * will be in the selected state.
	 */
	static constexpr Flags MssCommunicating = Flags(128);
	/**
	 * The first flag that may be defined by a derived class.
	 * @warning  The bit position may change.
	 */
	static constexpr Flags MssFirstDerivedClassFlag = Flags(256);
	/**
	 * Flags for SPI mode 0.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode0 = MssMSbFirst | MssOutFallInRise
		| MssFullDuplex;
	/**
	 * Flags for SPI mode 1.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode1 = MssMSbFirst | MssFullDuplex;
	/**
	 * Flags for SPI mode 2.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode2 = MssMSbFirst | MssClockIdleHigh
		| MssFullDuplex;
	/**
	 * Flags for SPI mode 3.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode3 = MssMSbFirst | MssClockIdleHigh |
		MssOutFallInRise | MssFullDuplex;
	/**
	 * Flags for SPI mode 0 with the LSb transfered first.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode0LSb = MssOutFallInRise | MssFullDuplex;
	/**
	 * Flags for SPI mode 1 with the LSb transfered first.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode1LSb = MssFullDuplex;
	/**
	 * Flags for SPI mode 2 with the LSb transfered first.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode2LSb = MssClockIdleHigh | MssFullDuplex;
	/**
	 * Flags for SPI mode 3 with the LSb transfered first.
	 * This does @b not include MssUseSelect; this should be set if needed.
	 */
	static constexpr Flags MssSpiMode3LSb = MssClockIdleHigh
		| MssOutFallInRise | MssFullDuplex;
protected:
	/**
	 * The minimum time between changing the clock edge in nanoseconds.
	 * This is a period instead of a frequency because software implementations
	 * need to sleep for half a period very often, whereas implementations that
	 * require a frequency are typically using hardware support and need the
	 * frequency just once.
	 */
	unsigned int minHalfPeriod;
	/**
	 * Configuration flags.
	 */
	Flags flags;
	/**
	 * Attempts to forcibly cease communications by calling condStop() and
	 * close(). It is intended to be called in the destructor of derived
	 * classes. This base class cannot make the call in its destructor because
	 * the calls invoke virtual functions. The destructor may call this function
	 * unconditionally.
	 */
	void forceClose();
	/**
	 * Transitions the object from the ready (MssReady) to the open (MssOpen)
	 * state. This should acquire access to any required shared resources.
	 * @pre  This function will only be called when in the ready state.
	 * @post The object will be open, but the flag MssOpen is not set. The
	 *       caller of this function will set the flag.
	 * @throw boost::exception  Badness; state transision did not occur.
	 */
	virtual void open() = 0;
	/**
	 * Transitions the object from the open (MssOpen) to the ready (MssReady)
	 * state. This should relinquish access to any required shared resources.
	 * @pre  This function will only be called when in the open state.
	 * @post The object will be ready, but the flag MssOpen is still set. The
	 *       caller of this function will clear the flag.
	 */
	virtual void close() = 0;
	/**
	 * Denotes the start of a conversation; transitions from the open state to
	 * the communicating state.
	 * @pre   This object is in the ready state (MssReady) and not in the
	 *        communicating state (MssCommunicating). These conditions are
	 *        checked by condStart().
	 * @post  Communication may commence. If the other end needs to be selected,
	 *        the selection has occured. The MssCommunicating flag does @b not
	 *        need to be set; that is handled by condStart().
	 */
	virtual void start() = 0;
	/**
	 * Denotes the end of a conversation; transitions from the communicating
	 * state to the open state.
	 * @pre   This object is in the communicating state (MssCommunicating)
	 *        and not in the open state (MssOpen). These conditions are
	 *        checked by condStop().
	 * @post  The chip is deselected if the MssUseSelect flag is set. The
	 *        clock is in the idle state.
	 */
	virtual void stop() = 0;
	/**
	 * Calls start() if not currently communicating (clear MssCommunicating
	 * flag).
	 * @pre   The object is in the ready state (MssReady).
	 * @post  The object is in the communicating state (MssCommunicating).
	 * @throw SyncSerialNotReady  The object is not in the ready state
	 *                            (MssReady).
	 * @bug Exception changed to SyncSerialNotOpen
	 */
	void condStart();
	/**
	 * Calls stop() if currently communicating (set MssCommunicating flag).
	 * @post  The object is not in the communicating state (MssCommunicating).
	 * @throw SyncSerialNotReady  The object is not in the ready state
	 *                            (MssReady).
	 * @bug Exception changed to SyncSerialNotOpen
	 */
	void condStop();
	/**
	 * Changes the maximum clock frequency. This is an internal function to
	 * allow implementations control over the stored clock speed.
	 * @pre  No communication must current be active; @a selected must be false.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param freq  The requested maximum clock frequency in hertz.
	 */
	void clockFrequency(unsigned int freq);
	/**
	 * Changes the minimum clock period. This is an internal function to
	 * allow implementations control over the clock speed.
	 * @pre  No communication must current be active; @a selected must be false.
	 * @post @a minHalfPeriod has half the period of the clock in nanoseconds.
	 * @param nanos  The requested minimum clock period in nanoseconds.
	 */
	void clockPeriod(unsigned int nanos);
	/**
	 * Sends and/or receives @a bits of data. If full duplex communication is
	 * not supported, one of the buffers should be given a nullptr pointer. The
	 * buffers must not overlap.
	 * @pre    This object is in the communicating state from a prior call to
	 *         condStart().
	 * @param  out   The data to transmit, or nullptr to not transmit.
	 * @param  in    The buffer that will receive data, or nullptr if nothing
	 *               will be received.
	 * @param  bits  The number of bits to transfer. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotFullDuplex  Neither @a out and @a in are nullptr,
	 *                                  and the serial interface is half-duplex.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	virtual void transfer(
		const std::uint8_t * __restrict__ out,
		std::uint8_t * __restrict__ in,
		duds::general::Bits bits
	) = 0;
	/**
	 * Sends @a bits of data. If full duplex communication is used,
	 * received data is lost. The implementation in this base class is:
	 * @code
	 * transfer(buff, nullptr, bits);
	 * @endcode
	 * @pre    This object is in the communicating state from a prior call to
	 *         condStart().
	 * @param  buff  The data to transmit.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	virtual void transmit(const std::uint8_t *buff, duds::general::Bits bits);
	/**
	 * Receives @a bits of data. If full duplex communication is used,
	 * transmitted data is undefined unless an implementation cares to have a
	 * definition. The implementation in this base class is:
	 * @code
	 * transfer(nullptr, buff, bits);
	 * @endcode
	 * @pre    This object is in the communicating state from a prior call to
	 *         condStart().
	 * @param  buff  The buffer that will receive the data.
	 * @param  bits  The number of bits to send. Implementations may impose
	 *               limitations on this value, like requiring a multiple of 8.
	 * @throw  SyncSerialNotCommunicating  This object is not in the
	 *                                     communicating state.
	 * @throw  SyncSerialUnsupported    An operation unsupported by the
	 *                                  implementation was attempted. This may
	 *                                  happen if (@a bits % 8) is non-zero.
	 * @throw  SyncSerialIoError        An error prevented the communication.
	 */
	virtual void receive(std::uint8_t *buff, duds::general::Bits bits);
	/**
	 * Has a half-duplex Conversation with the connected device. The
	 * Conversation object defines all input and output parameters. On the
	 * ConversationPart objects, the @ref ConversationPart::MpfBreak "MpfBreak"
	 * flag is honored, but the @ref ConversationPart::MpfVarlen "MpfVarlen"
	 * flag is ignored. The transmit() and receive() functions are called to
	 * move the data.
	 * @pre   The object is in the open state or the communicating state.
	 * @post  The object is in the open state, but not the communicating state.
	 * @param conv  The conversation to have with the device on the other end.
	 * @throw SyncSerialIoError     An error prevented the communication.
	 */
	void converseAlreadyOpen(Conversation &conv);
public:
	/**
	 * Builds a MasterSyncSerial with an invalid clock period and all
	 * configuration flags clear.
	 */
	MasterSyncSerial() : minHalfPeriod(0), flags(0)  { }
	/**
	 * Builds a MasterSyncSerial object.
	 * @param flags   The initial set of configuration flags.
	 * @param period  The minimum clock period in nanoseconds.
	 */
	MasterSyncSerial(Flags flags, int period);
	/**
	 * Derived class destructors should assure that communication has stopped
	 * by calling forceClose(). This may be done unconditionally. forceClose()
	 * will call virtual functions. Those functions must be called before the
	 * destructor of the class that implements them has finished running; this
	 * base class cannot make the call.
	 */
	virtual ~MasterSyncSerial() = 0;
	/**
	 * Returns the current set of configuration flags.
	 * @todo  Change name to flags; change name of member flags.
	 */
	Flags configFlags() const noexcept {
		return flags & MssConfigMask;
	}
	/**
	 * Returns true when this serial interface is in use by checking for the
	 * existence of an access object.
	 */
	bool inUse() const noexcept {
		return mssacc != nullptr;
	}
	/**
	 * Returns the minimum clock period in nanoseconds. The actual clock period
	 * may be longer.
	 */
	unsigned int clockPeriod() const noexcept {
		return minHalfPeriod << 1;
	}
	/**
	 * Computes and returns the maximum clock frequency in Hertz. The actual
	 * clock frequency may be lower.
	 */
	unsigned int clockFrequency() const noexcept;
	/**
	 * Obtain access for communication; transitions the object from the ready
	 * (MssReady) to the open (MssOpen) state. Implementations handle the
	 * transition with their implementation of open(). If open() throws an
	 * exception, it will not be caught by this function and will prevent the
	 * state transition.
	 * @pre     The object is in the ready state.
	 * @post    The object is in the open state.
	 * @return  The access object for controlling communications.
	 * @throw SyncSerialNotReady  The object is not in the ready state.
	 * @throw SyncSerialInUse     The object is already in the open state.
	 */
	std::unique_ptr<MasterSyncSerialAccess> access();
	/**
	 * Obtain access for communication; transitions the object from the ready
	 * (MssReady) to the open (MssOpen) state. Implementations handle the
	 * transition with their implementation of open(). If open() throws an
	 * exception, it will not be caught by this function and will prevent the
	 * state transition.
	 * @pre   The object is in the ready state.
	 * @post  The object is in the open state.
	 * @param acc    The access object that will be used for controlling
	 *               communications. It must not already be providing access.
	 * @throw SyncSerialNotReady     The object is not in the ready state.
	 * @throw SyncSerialInUse        The object is already in the open state.
	 * @throw SyncSerialAccessInUse  The access object, @a acc, is providing
	 *                               access to a serial interface.
	 */
	void access(MasterSyncSerialAccess &acc);
	/**
	 * Obtain access for communication; transitions the object from the ready
	 * (MssReady) to the communicating (MssCommunicating) state.
	 * Implementations handle the transition with their implementation of
	 * open(). If open() throws an exception, it will not be caught by this
	 * function and will prevent the state transition.
	 * @pre     The object is in the ready state.
	 * @post    The object is in the communicating state.
	 * @return  The access object for controlling communications.
	 * @throw SyncSerialNotReady  The object is not in the ready state.
	 * @throw SyncSerialInUse     The object is in the open or
	 *                            communicating state.
	 */
	std::unique_ptr<MasterSyncSerialAccess> accessStart();
	/**
	 * Obtain access for communication; transitions the object from the ready
	 * (MssReady) to the communicating (MssCommunicating) state.
	 * Implementations handle the transition with their implementation of
	 * open(). If open() throws an exception, it will not be caught by this
	 * function and will prevent the state transition.
	 * @pre   The object is in the ready state.
	 * @post  The object is in the communicating state.
	 * @param acc    The access object that will be used for controlling
	 *               communications. It must not already be providing access.
	 * @throw SyncSerialNotReady     The object is not in the ready state.
	 * @throw SyncSerialInUse        The object is in the open or
	 *                               communicating state.
	 * @throw SyncSerialAccessInUse  The access object, @a acc, is providing
	 *                               access to a serial interface.
	 */
	void accessStart(MasterSyncSerialAccess &acc);
	/**
	 * Has a half-duplex Conversation with the connected device. An access
	 * object is not used to call this function, but one is acquired and
	 * released internally. The Conversation object defines all input and output
	 * parameters. On the ConversationPart objects, the
	 * @ref ConversationPart::MpfBreak "MpfBreak" flag is honored, but the
	 * @ref ConversationPart::MpfVarlen "MpfVarlen" flag is ignored. The
	 * transmit() and receive() functions are called to move the data.
	 * @pre   The object is in the ready state, but not the open state. There
	 *        must not be an access object to use this object.
	 * @post  The object is in the ready state and not the open state.
	 * @param conv  The conversation to have with the device on the other end.
	 * @throw SyncSerialInUse       The object is already in the open state.
	 * @throw SyncSerialIoError     An error prevented the communication.
	 */
	virtual void converse(Conversation &conv);
};

} } } // namespaces

#endif        //  #ifndef MASTERSYNCSERIAL_HPP
