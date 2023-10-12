/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2023  Jeff Jackowski
 */
#include <duds/ui/graphics/BppFontPool.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

void BppFontPool::add(
	const std::string &name,
	const BppFontSptr &font,
	const BppStringCacheSptr &cache
) {
	if (!font) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(name));
	}
	if (cache && (cache->font() != font)) {
		DUDS_THROW_EXCEPTION(FontStringCacheMismatchError() << FontName(name));
	}
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, FontAndCache{font, cache});
}

void BppFontPool::add(
	const std::string &name,
	BppFontSptr &&font,
	BppStringCacheSptr &&cache
) {
	if (!font) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(name));
	}
	if (cache && (cache->font() != font)) {
		DUDS_THROW_EXCEPTION(FontStringCacheMismatchError() << FontName(name));
	}
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, FontAndCache{std::move(font), std::move(cache)});
}

void BppFontPool::addWithoutCache(
	const std::string &name,
	const BppFontSptr &font
) {
	if (!font) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(name));
	}
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, FontAndCache{font, BppStringCacheSptr()});
}

void BppFontPool::addWithoutCache(
	const std::string &name,
	const std::string &fontpath
) {
	FontAndCache fc;
	fc.fnt = BppFont::make(fontpath);
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, std::move(fc));
}

void BppFontPool::addWithCache(
	const std::string &name,
	const BppFontSptr &font
) {
	if (!font) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(name));
	}
	FontAndCache fc;
	fc.fnt = font;
	fc.sc = BppStringCache::make(fc.fnt);
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, std::move(fc));
}

void BppFontPool::addWithCache(
	const std::string &name,
	const std::string &fontpath
) {
	FontAndCache fc;
	fc.fnt = BppFont::make(fontpath);
	fc.sc = BppStringCache::make(fc.fnt);
	std::lock_guard<duds::general::Spinlock> lock(block);
	fonts.emplace(name, std::move(fc));
}

void BppFontPool::alias(
	const std::string &existing,
	const std::string &newname
) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<std::string, FontAndCache>::const_iterator iter =
		fonts.find(existing);
	if (iter == fonts.cend()) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(existing));
	}
	fonts.emplace(newname, FontAndCache{iter->second.fnt, iter->second.sc});
}

BppFontSptr BppFontPool::getFont(const std::string &font) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<std::string, FontAndCache>::const_iterator iter =
		fonts.find(font);
	if (iter == fonts.cend()) {
		return BppFontSptr();
	}
	return iter->second.fnt;
}

BppStringCacheSptr BppFontPool::getStringCache(const std::string &font) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<std::string, FontAndCache>::const_iterator iter =
		fonts.find(font);
	if (iter == fonts.cend()) {
		return BppStringCacheSptr();
	}
	return iter->second.sc;
}

void BppFontPool::getFc(FontAndCache &fc, const std::string &font) const {
	std::lock_guard<duds::general::Spinlock> lock(block);
	std::unordered_map<std::string, FontAndCache>::const_iterator iter =
		fonts.find(font);
	if (iter == fonts.cend()) {
		DUDS_THROW_EXCEPTION(FontNotFoundError() << FontName(font));
	}
	fc = iter->second;
}

} } }
