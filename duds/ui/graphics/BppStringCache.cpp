/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppStringCache.hpp>
#include <duds/ui/graphics/BppImageErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

BppStringCache::BppStringCache(
	const BppFontSptr &font,
	unsigned int maxBytes,
	unsigned int maxStrings
) : fnt(font), maxB(maxBytes), maxS(maxStrings) {
	if (!maxS) {
		DUDS_THROW_EXCEPTION(StringCacheZeroSize());
	}
}

BppStringCache::BppStringCache(
	BppFontSptr &&font,
	unsigned int maxBytes,
	unsigned int maxStrings
) : fnt(std::move(font)), maxB(maxBytes), maxS(maxStrings) {
	if (!maxS) {
		DUDS_THROW_EXCEPTION(StringCacheZeroSize());
	}
}

void BppStringCache::clear() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	cache.clear();
	curB = 0;
}

const BppImageSptr &BppStringCache::text(
	const std::string &str,
	BppFont::Flags flags
) {
	{
		std::lock_guard<duds::general::Spinlock> lock(block);
		// attempt to find a match
		Cache::iterator iter = cache.find(std::make_tuple(str, flags));
		if (iter != cache.end()) {
			// move item to the end of the sequence
			Cache::index<index_seq>::type::iterator siter =
				cache.project<index_seq>(iter);
			Cache::index<index_seq>::type &sidx = cache.get<index_seq>();
			sidx.relocate(sidx.end(), siter);
			return iter->img;
		}
	}
	// no match; it must be rendered
	BppImageSptr img = fnt->render(str, flags);
	std::lock_guard<duds::general::Spinlock> lock(block);
	// store the result for later
	std::pair<Cache::iterator, bool> res =
		cache.emplace(BppString{img, str, flags});
	// insertion check
	if (res.second) {
		// update current image size sum
		curB += img->data().size() * sizeof(BppImage::PixelBlock);
		// cache size limit hit?
		if ((curB > maxB) || (cache.size() > maxS)) {
			Cache::index<index_seq>::type &sidx = cache.get<index_seq>();
			// loop through removals
			Cache::index<index_seq>::type::iterator iter = sidx.begin();
			do {
				// remove area from the sum
				curB -= iter->img->data().size() * sizeof(BppImage::PixelBlock);
				// remove item
				iter = sidx.erase(iter);
			}
			// loop until size of data is within limits, or the just added item
			// is the only item left
			while ((curB > maxB) && (cache.size() > 1));
		}
	}
	// else {
		// Must have had multiple requests for the same string simultaneously
		// that resulted in multiple renderings during time without a lock.
		// Moving the lock to cover the whole function will prevent this, but
		// I suspect this condition will be very rare, and releasing the lock
		// during rendering will allow more time for multiple threads to use
		// the cache without blocking.
	// }
	// return from item in cache
	return res.first->img;
}

} } }
