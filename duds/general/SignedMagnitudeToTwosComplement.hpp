#include <type_traits>

namespace duds { namespace general {

/**
 * Converts a signed magnitude value to two's complement. Also has the effect
 * of a sign extention on the input value.
 * @tparam B  The number of bits used in the input. The value of @a x must be
 *            positioned (shifted) such that a positive value is already
 *            correct. The sign bit will be the MSb indicated by this value
 *            (1 << B). More significant bits than @a B bits are ignored.
 * @tparam T  The supplied integer type. A negative value will be properly
 *            represented in the signed variation of the type in the returned
 *            value. @a T may be signed or unsigned.
 * @param x   A signed magnitude value from a potentially limited number of
 *            bits that may not fill @a T.
 * @return    The two's complement version of @a x that fills @a T. Negative
 *            zero is reported as zero. The type is always signed.
 * @author    Jeff Jackowski
 * @author    Sean Eron Anderson (SignExtend() was used as starting point)
 * @license   Public domain
 */
template <unsigned B, typename T>
inline typename std::make_signed<T>::type SignedMagnitudeToTwosComplement(
	const T x
) {
	// one sign bit and one magnitude bit is the minimum requirement for this
	// operation to make sense
	static_assert(
		B > 1,
		"At least 2 bits are required for signed magnitude input."
	);
	// the sign bit must be inside the integer type
	static_assert(
		B <= (sizeof(T) * 8),
		"The signed magnitude input must fit within the integer type."
	);
	struct {
		typename std::make_signed<T>::type x:B;
	} s;
	s.x = (typename std::make_signed<T>::type)x;
	if (s.x < 0) {
		s.x = (~(1 << (B - 1)) & s.x) * -1;
	}
	return s.x;
}

} }
