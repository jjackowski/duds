/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppFont.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * Maintains a cache for rendered strings that helps avoid re-rendering
 * strings that may need to be shown many times. The cache is limited in
 * size by the number of strings and the total size of all the rendered
 * images in bytes. All operations are thread-safe.
 * @author  Jeff Jackowski
 */
class BppStringCache : boost::noncopyable {
	/**
	 * The font to use for rendering.
	 */
	BppFontSptr fnt;
	/**
	 * Stores an image and the data used to create it using the font.
	 */
	struct BppString {
		/**
		 * The rendered image of the text.
		 */
		ConstBppImageSptr img;
		/**
		 * The text rendered into @a img.
		 */
		std::string text;
		/**
		 * The font rendering flags.
		 */
		BppFont::Flags flags;
	};
	/**
	 * The container type for the rendered strings.
	 * It has two indices:
	 * -# A combination of the string and the font rendering flags. Used to find
	 *    existing items.
	 * -# The sequence of access. Used to determine which item(s) to remove when
	 *    the cache grows past its limits.
	 */
	typedef boost::multi_index::multi_index_container<
		BppString,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<struct index_text>,
				boost::multi_index::composite_key<
					BppString,
					boost::multi_index::member<
						BppString,
						std::string,
						&BppString::text
					>,
					boost::multi_index::member<
						BppString,
						BppFont::Flags,
						&BppString::flags
					>
				>
			>,
			boost::multi_index::sequenced<
				boost::multi_index::tag<struct index_seq>
			>
		>
	> Cache;
	/**
	 * The cache of rendered strings.
	 */
	Cache cache;
	/**
	 * The maximum number of strings the cache may hold.
	 */
	unsigned int maxS;
	/**
	 * The maximum size of rendered text images, in bytes, the cache may hold.
	 */
	unsigned int maxB;
	/**
	 * The current size of all rendered text images in the cache.
	 */
	unsigned int curB = 0;
	/**
	 * Used for thread safety.
	 */
	duds::general::Spinlock block;
public:
	/**
	 * Creates a cache of rendered strings made using the given font.
	 * @param font        The font to use to render the cached text.
	 * @param maxBytes    The maximum size of the rendered string images in
	 *                    bytes. If this value is very low, even zero, then
	 *                    only one rendered image will be cached.
	 * @param maxStrings  The maximum number of rendered strings that may be
	 *                    held by the cache. This value cannot be zero.
	 * @throw             StringCacheZeroSize  The cache size limits prevent
	 *                                         any image from being kept in
	 *                                         the cache. @a maxBytes is zero.
	 */
	BppStringCache(
		const BppFontSptr &font,
		unsigned int maxBytes = 256 * 1024,
		unsigned int maxStrings = -1
	);
	/**
	 * Creates a cache of rendered strings made using the given font.
	 * @param font        The font to use to render the cached text. This object
	 *                    will take ownership of the font.
	 * @param maxBytes    The maximum size of the rendered string images in
	 *                    bytes. If this value is very low, even zero, then
	 *                    only one rendered image will be cached.
	 * @param maxStrings  The maximum number of rendered strings that may be
	 *                    held by the cache. This value cannot be zero.
	 * @throw             StringCacheZeroSize  The cache size limits prevent
	 *                                         any image from being kept in
	 *                                         the cache. @a maxBytes is zero.
	 */
	BppStringCache(
		BppFontSptr &&font,
		unsigned int maxBytes = 256 * 1024,
		unsigned int maxStrings = -1
	);
	/**
	 * Returns the font object used by this cache to render text.
	 */
	const BppFontSptr &font() const {
		return fnt;
	}
	/**
	 * Returns the maximum size of the cached images in bytes.
	 */
	unsigned int maxBytes() const {
		return maxB;
	}
	/**
	 * Returns the maximum number of cached images.
	 */
	unsigned int maxStrings() const {
		return maxS;
	}
	/**
	 * Returns the total size in bytes of all the cached images.
	 */
	unsigned int bytes() const {
		return curB;
	}
	/**
	 * Returns the number of currently stored cached strings.
	 */
	unsigned int strings() const {
		return cache.size();
	}
	/**
	 * Clears all text images from the cache.
	 */
	void clear();
	/**
	 * Returns an image of the requested string either from a pre-rendered
	 * item in the cache or by rendering a new image. If the text is rendered,
	 * it is done by calling BppFont::render() on this object's font. After
	 * rendering, the cache size limits are enforced by removing the cached
	 * item(s) that were last requested farthest in the past until maximums
	 * are not exceeded. Returning a copy of the image's shared pointer
	 * prevents cache evictions from destroying images before they are used.
	 *
	 * @param text   The text to render.
	 * @param flags  The option flags. The default is to render varying width,
	 *               fixed height text with each line aligned to the left.
	 * @return       A const image with the rendered text.
	 * @throw        GlyphNotFoundError  A glyph in @a str is not provided
	 *                                   by the font.
	 */
	ConstBppImageSptr text(
		const std::string &str,
		BppFont::Flags flags = BppFont::AlignLeft
	);
};

typedef std::shared_ptr<BppStringCache>  BppStringCacheSptr;

} } }
