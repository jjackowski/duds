#include <type_traits>

namespace duds { namespace general {

/**
 * Performs a sign extention operation.
 * From http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend,
 * with some reformatting, a swap of the template parameters, and an update
 * to take advantage of C++11. The code is in the public domain.
 * @tparam B  The number of bits used in the input. The value of @a x must be
 *            positioned (shifted) such that a positive value is already
 *            correct.
 * @tparam T  The supplied integer type. A negative value will be properly
 *            represented in the signed variation of type in the returned
 *            value. @a T may be signed or unsigned.
 * @param x   A signed value from a limited number of bits that does not fill
 *            @a T.
 * @return    A sign extended version of @a x. The type is always signed.
 * @author    Sean Eron Anderson
 * @author    Jeff Jackowski (used std::make_signed so T can be unsigned).
 * @license   Public domain
 */
template <unsigned B, typename T>
inline typename std::make_signed<T>::type SignExtend(const T x) {
	struct {
		typename std::make_signed<T>::type x:B;
	} s;
	return s.x = (typename std::make_signed<T>::type)x;
}

} }

