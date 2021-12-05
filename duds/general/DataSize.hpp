/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#ifndef DATASIZES_HPP
#define DATASIZES_HPP

#include <duds/general/Errors.hpp>

namespace duds { namespace general {

/**
 * Used to report a failure to make an exact conversion of a size represented
 * by a DataSize object. This is only used for expressions that are not
 * constexpr. A constexpr expression that fails at an exact conversion will
 * cause a compile-time error.
 */
struct DataSizeConversionError : virtual std::exception, virtual boost::exception { };

/**
 * The number of bits that make up the size unit of the source DataSize object.
 */
typedef boost::error_info<struct Info_DataSizeSourceUnit, std::size_t>
	DataSizeSourceUnit;

/**
 * The number of bits that make up the size unit of the target DataSize object.
 */
typedef boost::error_info<struct Info_DataSizeTargetUnit, std::size_t>
	DataSizeTargetUnit;

/**
 * The number of blocks from a DataSize object that cannot be converted. The
 * represented size is the number of blocks multiplied by the bit size of the
 * DataSize template parameter.
 */
typedef boost::error_info<struct Info_DataSizeBlocks, std::size_t>
	DataSizeBlocks;

#if defined(DUDS_ERRORS_VERBOSE) || defined(DOXYGEN)
/**
 * Throws a DataSizeConversionError exception and adds in error metadata.
 * This is used instead of ::DUDS_THROW_EXCEPTION or BOOST_THROW_EXCEPTION
 * because they are seen as void type expressions rather than a throw
 * exception, and the DataSize template class needs them to be seen as a throw
 * to be used within constexpr funcions.
 * @param bits     The bits per block in the source.
 * @param trgbits  The bits per block in the target.
 * @param blocks   The number of blocks that could not be converted.
 */
#define THROW_DATASIZE_CONVERSION_ERROR(bits, trgbits, blocks)  \
	throw boost::enable_current_exception(DataSizeConversionError()) << \
			DataSizeSourceUnit(bits) << DataSizeTargetUnit(trgbits) << \
			DataSizeBlocks(blocks) << \
			boost::throw_function(BOOST_CURRENT_FUNCTION) << \
			boost::throw_file(__FILE__) << ::boost::throw_line((int)__LINE__) << \
			duds::general::StackTrace(boost::stacktrace::stacktrace())
#else
#define THROW_DATASIZE_CONVERSION_ERROR(bits, trgbits, blocks)  \
	throw boost::enable_current_exception(DataSizeConversionError()) << \
			DataSizeSourceUnit(bits) << DataSizeTargetUnit(trgbits) << \
			DataSizeBlocks(blocks) << \
			boost::throw_function(BOOST_CURRENT_FUNCTION) << \
			boost::throw_file(__FILE__) << ::boost::throw_line((int)__LINE__)
#endif

/**
 * A class to assist with specifiying the sizes of data with scaling units,
 * much like std::chrono::duration does with time. The base unit used is the
 * bit in order to work for any application, not just data storage on common
 * computers. The template takes the number of bits for its scalar, and stores
 * the multiple of those bits for its indicated size. Many of the functions
 * are declared constexpr, and the class is trivially constructable and
 * copyable.
 *
 * DataSize objects are implicitly convertable to other DataSize template
 * types, and will convert the stored size to match units. If such a
 * conversion cannot represent the size exactly in an integer, the conversion
 * will fail. If the conversion is in a constexpr expression, the failure is
 * at compile time. Otherwise, it is at runtime with a DataSizeConversionError
 * exception. There are conversion functions that end with "Rounded" that will
 * round up when the size cannot be exactly represented so that the result
 * will be at least as large as the input.
 *
 * @tparam  Bits  The number of bits that make up one unit of data for this
 *          type. For example, 8 for a byte, and 1024 for a kilobyte. Several
 *          typedefs for common sizes exist, including Bytes and Kilobytes.
 *
 * @sa Bits
 * @sa Kilobits
 * @sa Megabits
 * @sa Gigabits
 * @sa Bytes
 * @sa Kilobytes
 * @sa Megabytes
 *
 * @author  Jeff Jackowski
 */
template <std::size_t Bits>
class DataSize {
	/**
	 * The indicated size stored as a multiple of @a Bits.
	 */
	std::size_t numblocks;
public:
	/**
	 * Returns the number of bits in each unit for this type; same as @a Bits.
	 */
	static constexpr std::size_t blockSize() {
		return Bits;
	}
	/**
	 * Trival constructor.
	 */
	DataSize() = default;
	/**
	 * Initalizes the size to the indicated value. The represented size will be
	 * @a blocks multiplied by @a Bits.
	 * @param blocks  The size specified as a multuple of @a Bits.
	 */
	constexpr DataSize(std::size_t blocks) : numblocks(blocks) { }
	/**
	 * Constructs a new DataSize object representing the same size as the given
	 * object, but with a multiple different from @a Bits.
	 * @note     When used in a constexpr expression, this constructor will fail
	 *           to compile if ds.bits() divided by @a Bits has a reminder.
	 *           This is intentional.
	 * @warning  This constructor will @b never fail to compile if not used in
	 *           an explicitly constexpr expression.
	 * @bug      The constructor should fail to compile when not used in an
	 *           explicitly constexpr expression if @a Bits divided by
	 *           @a ConvBits has a reminder, and the parameter @a ds is
	 *           constexpr.
	 * @tparam ConvBits  The number of bits in each unit of the converted
	 *                   result. This will need to be explicitly specified.
	 * @param  ds        The source DataSize object.
	 * @return The size converted to new units. It represents the same number
	 *         of bits.
	 * @throw  DataSizeConversionError  The size cannot be excactly represented
	 *                                  by a DataSize<ConvBits> object. This
	 *                                  will only be thrown from expressions
	 *                                  that are not constexpr.
	 */
	template <std::size_t ConvBits>
	constexpr DataSize(const DataSize<ConvBits> &ds) :
	numblocks(
		// the throw prevents compilation when the conditional is false and
		// this function is used in a constexpr expression
		(ds.bits() % Bits) ?
		THROW_DATASIZE_CONVERSION_ERROR(ConvBits, Bits, ds.blocks()) :
		(ds.bits() / Bits)
	) { }
	/**
	 * Returns the size specified as a multuple of @a Bits.
	 */
	constexpr std::size_t blocks() const {
		return numblocks;
	}
	/**
	 * Returns the size specified in bits.
	 * @note  The maximum size that can be represented as bits is one bit short
	 *        of 2EB (exabytes).
	 */
	constexpr std::uint64_t bits() const {
		return (std::uint64_t)numblocks * (std::uint64_t)Bits;
	}
	/**
	 * Returns a new DataSize object representing the same size, but with a
	 * multiple different from @a Bits.
	 * @note   When used in a constexpr expression, this function will fail
	 *         to compile if bits() divided by @a OtherDataSize has a reminder.
	 *         This is intentional.
	 * @tparam OtherDataSize  The number of bits in each unit of the converted
	 *                        result. This will need to be explicitly specified.
	 * @return The size converted to new units. It represents the same number
	 *         of bits.
	 * @throw  DataSizeConversionError  The size cannot be excactly represented
	 *                                  by a DataSize<OtherDataSize> object.
	 *                                  This will only be thrown from
	 *                                  expressions that are not constexpr.
	 */
	template <class OtherDataSize>
	constexpr OtherDataSize size() const {
		// the throw prevents compilation when the conditional is false and
		// this function is used in a constexpr expression
		return (bits() % OtherDataSize::blockSize()) ?
		THROW_DATASIZE_CONVERSION_ERROR(Bits, OtherDataSize::blockSize(), numblocks) :
		OtherDataSize(bits() / OtherDataSize::blockSize());
	}
	/**
	 * Implicit conversion of DataSize objects to other DataSize classes using
	 * a different size multiple. The conversion will fail if the size cannot
	 * be exactly represented.
	 * @note   When used in a constexpr expression, this function will fail
	 *         to compile if bits() divided by @a ConvBits has a reminder.
	 *         This is intentional.
	 * @tparam ConvBits  The number of bits in each unit of the converted
	 *                   result. This will need to be explicitly specified.
	 * @return A new DataSize object with the same size in bits converted to
	 *         new units.
	 * @throw  DataSizeConversionError  The size cannot be excactly represented
	 *                                  by a DataSize<ConvBits> object. This
	 *                                  will only be thrown from expressions
	 *                                  that are not constexpr.
	 */
	template <std::size_t ConvBits>
	constexpr operator DataSize<ConvBits>() const {
		return size<ConvBits>();
	}
	/**
	 * Returns a new DataSize object representing a size with a multiple
	 * different from @a Bits that is as small as possible while representing
	 * at least as many bits as this object.
	 * @tparam OtherDataSize  The number of bits in each unit of the converted
	 *                        result. This will need to be explicitly specified.
	 * @return The size converted to new units. It will be larger if the
	 *         conversion could not be exact.
	 */
	template <class OtherDataSize>
	constexpr OtherDataSize sizeRounded() const {
		return OtherDataSize(
			(bits() / OtherDataSize::blockSize()) +
			((bits() % OtherDataSize::blockSize()) ? 1 : 0)
		);
	}
	/**
	 * Returns the size specified in bytes, or fails if the size cannot be
	 * exactly represented as an integer value of bytes.
	 * @note   When used in a constexpr expression, this function will fail
	 *         to compile if bits() divided by 8 has a reminder.
	 *         This is intentional.
	 * @note  The maximum size that can be represented as bytes is one byte
	 *        short of 16EB (exabytes).
	 */
	constexpr std::uint64_t bytes() const {
		return size< DataSize<8> >().blocks();
	}
	/**
	 * Returns the size specified in bytes, rounded up.
	 * @note  The maximum size that can be represented as bytes is one byte
	 *        short of 16EB (exabytes).
	 */
	constexpr std::uint64_t bytesRounded() const {
		return sizeRounded< DataSize<8> >().blocks();
	}
	/**
	 * Checks equality by comparing the number of bits so that any DataSize
	 * template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator == (const DataSize<OtherBits> &op) const {
		return bits() == op.bits();
	}
	/**
	 * Checks inequality by comparing the number of bits so that any DataSize
	 * template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator != (const DataSize<OtherBits> &op) const {
		return bits() != op.bits();
	}
	/**
	 * Greater-than comparison done by comparing the number of bits so that any
	 * DataSize template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator > (const DataSize<OtherBits> &op) const {
		return bits() > op.bits();
	}
	/**
	 * Less-than comparison done by comparing the number of bits so that any
	 * DataSize template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator < (const DataSize<OtherBits> &op) const {
		return bits() < op.bits();
	}
	/**
	 * Greater-than-or-equal comparison done by comparing the number of bits so
	 * that any DataSize template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator >= (const DataSize<OtherBits> &op) const {
		return bits() >= op.bits();
	}
	/**
	 * Less-than-or-equal comparison done by comparing the number of bits so
	 * that any DataSize template types may be compared without a conversion.
	 */
	template <std::size_t OtherBits>
	constexpr bool operator <= (const DataSize<OtherBits> &op) const {
		return bits() <= op.bits();
	}
	/**
	 * Adds two sizes. Differing DataSize templates may be used; this will cause
	 * an implicit conversion which will fail if the conversion cannot be
	 * exact. The result is that the DataSize template with the smaller uint
	 * (@a Bits) should generally be on the left side of the expression.
	 * @code
	 * Bytes(8) + Kilobytes(8); // works
	 * Kilobits(8) + Bits(8);  // fails
	 * @endcode
	 */
	constexpr DataSize operator + (const DataSize &op) const {
		return DataSize(numblocks + op.numblocks);
	}
	/**
	 * Subtracts two sizes. Differing DataSize templates may be used; this will
	 * cause an implicit conversion which will fail if the conversion cannot be
	 * exact. The result is that the DataSize template with the smaller uint
	 * (@a Bits) should generally be on the left side of the expression.
	 * @code
	 * Bytes(8) - Kilobytes(8); // works
	 * Kilobits(8) - Bits(8);  // fails
	 * @endcode
	 */
	constexpr DataSize operator - (const DataSize &op) const {
		return DataSize(numblocks - op.numblocks);
	}
	/**
	 * Adds two DataSize objects and stores the result.
	 */
	DataSize &operator += (const DataSize &op) {
		numblocks += op.numblocks;
		return *this;
	}
	/**
	 * Subtracts two DataSize objects and stores the result.
	 */
	DataSize &operator -= (const DataSize &op) {
		numblocks -= op.numblocks;
		return *this;
	}
	/**
	 * Muliplies the size by a scalar value. Multiplying two DataSize objects
	 * is not directly allowed since the result should be a value in square bits.
	 */
	constexpr DataSize operator * (int scalar) const {
		return DataSize(numblocks * scalar);
	}
	/**
	 * Divides the size by a scalar value. Dividing two DataSize objects is not
	 * directly allowed since the result would be unitless (not bits or anything
	 * else).
	 */
	constexpr DataSize operator / (int scalar) const {
		return DataSize(numblocks / scalar);
	}
	/**
	 * Multiplies a DataSize object by a scalar and stores the result.
	 */
	DataSize &operator *= (int scalar) {
		numblocks *= scalar;
		return *this;
	}
	/**
	 * Divides a DataSize object by a scalar and stores the result.
	 */
	DataSize &operator /= (int scalar) {
		numblocks /= scalar;
		return *this;
	}
};

/**
 * DataSize type for a size in bits.
 */
typedef DataSize<1> Bits;

/**
 * DataSize type for a size in nibbles.
 */
typedef DataSize<4> Nibbles;

/**
 * DataSize type for a size in bytes.
 */
typedef DataSize<8> Bytes;

/**
 * DataSize type for a size in kilobytes.
 */
typedef DataSize<1024*8> Kilobytes;

/**
 * DataSize type for a size in megabytes.
 */
typedef DataSize<1024*1024*8> Megabytes;

/**
 * DataSize type for a size in kilobits.
 */
typedef DataSize<1000> Kilobits;

/**
 * DataSize type for a size in megabits.
 */
typedef DataSize<1000000> Megabits;

/**
 * DataSize type for a size in gigabits.
 */
typedef DataSize<1000000000> Gigabits;

} }

#endif        //  #ifndef DATASIZES_HPP
