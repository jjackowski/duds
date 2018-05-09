namespace duds { namespace general {

/**
 * Returns true only if the given value is a power of 2.
 * @tparam Integer  The integer type of the value.
 * @param  i        The integer value to check.
 */
template <typename Integer>
constexpr inline bool IsPowerOf2(Integer i) {
    return i && !(i & (i - 1));
}

/**
 * Reverses the bits in a given value.
 * From http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel,
 * with some reformatting and changed to a C++ template. The code is in the
 * public domain.
 * @tparam Integer  The integer type of the value.
 * @param  i        The integer value to reverse.
 * @return          The integer with the bits in reverse order.
 * @author          Ken Raeburn
 * @author          Jeff Jackowski (changed to template, power of 2 check)
 * @license         Public domain
 */
template <typename Integer>
inline Integer ReverseBits(Integer i) {
	static_assert(
		IsPowerOf2(sizeof(Integer)),
		"Size must be a power of 2 for this algorithm."
	);
	std::size_t s = sizeof(Integer) * 8;
	Integer m = -1;
	while (s >>= 1) {
			m ^= (m << s);
			i = ((i >> s) & m) | ((i << s) & ~m);
	}
	return i;
}

/**
 * Reverse the bits of a given byte.
 * From
 * http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv,
 * with some reformatting into a constexpr function. The code is in the
 * public domain.
 * @param  i   The integer value to reverse.
 * @return     The integer with the bits in reverse order.
 * @author     Rich Schroeppel
 * @author     Jeff Jackowski (changed to constexpr function)
 * @license    Public domain
 */
constexpr inline std::uint8_t ReverseBits(const std::uint8_t i) {
	return (i * 0x0202020202ULL & 0x010884422010ULL) % 1023;
}

} }
