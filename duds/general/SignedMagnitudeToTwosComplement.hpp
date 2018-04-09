#include <type_traits>

namespace duds { namespace general {

/**
 * Converts a signed magnitude value to two's complement. Also has the effect
 * of a sign extention on the input value.
 * @tparam B  The number of bits used in the input. The value of @a x must be
 *            positioned (shifted) such that a positive value is already
 *            correct. The sign bit will be the MSb indicated by this value.
 * @tparam T  The supplied integer type. A negative value will be properly
 *            represented in the signed variation of the type in the returned
 *            value. @a T may be signed or unsigned.
 * @param x   A signed magnitude value from a potentially limited number of
 *            bits that may not fill @a T.
 * @return    The two's complement version of @a x that fills @a T. The type is
 *            always signed.
 * @author    Jeff Jackowski
 * @author    Sean Eron Anderson (SignExtend() was used as starting point)
 * @license   Public domain
 */
template <unsigned B, typename T>
inline typename std::make_signed<T>::type SignedMagnitudeToTwosComplement(
	const T x
) {
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
