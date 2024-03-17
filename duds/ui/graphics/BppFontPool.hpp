/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2023  Jeff Jackowski
 */
#ifndef BPPFONTPOOL_HPP
#define BPPFONTPOOL_HPP
#include <duds/ui/graphics/BppStringCache.hpp>
#include <duds/ui/graphics/BppImageErrors.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * Handles a pool of fonts and their associated string caches to make it easier
 * to use fonts across various parts of an application.
 *
 * @author  Jeff Jackowski
 */
class BppFontPool : boost::noncopyable {
	/**
	 * Internal struct to hold a font and its string cache.
	 */
	struct FontAndCache {
		BppFontSptr fnt;
		BppStringCacheSptr sc;
	};
	/**
	 * The glyph images keyed by character.
	 */
	std::unordered_map<std::string, FontAndCache>  fonts;
	/**
	 * Used for thread safety.
	 */
	mutable duds::general::Spinlock block;
	/**
	 * An internal function to get a FontAndCache struct for the given font.
	 * @param fc    The destination FontAndCache.
	 * @param font  The name of the font to lookup.
	 * @throw       FontNotFoundError   The named font is not in the pool.
	 */
	void getFc(FontAndCache &fc, const std::string &font) const;
public:
	/**
	 * Adds an existing font and string cache pair to the pool.
	 * @param name   A name for the font. Used as the key value to find the
	 *               font and its string cache later.
	 * @param font   The font object.
	 * @param cache  A string cache made to work with the font.
	 * @throw        FontStringCacheMismatchError
	 *                 The string cache does not use @a font as its font.
	 */
	void add(
		const std::string &name,
		const BppFontSptr &font,
		const BppStringCacheSptr &cache
	);
	/**
	 * Adds an existing font and string cache pair to the pool.
	 * @param name   A name for the font. Used as the key value to find the
	 *               font and its string cache later.
	 * @param font   The font object. This pool will take ownership of the font.
	 * @param cache  A string cache made to work with the font. This pool will
	 *               take ownership of the string cache.
	 * @throw        FontStringCacheMismatchError
	 *                 The string cache does not use @a font as its font.
	 */
	void add(
		const std::string &name,
		BppFontSptr &&font,
		BppStringCacheSptr &&cache
	);
	/**
	 * Adds an existing font without a corresponding string cache.
	 * @param name   A name for the font. Used as the key value to find the
	 *               font and its string cache later.
	 * @param font   The font object.
	 */
	void addWithoutCache(const std::string &name, const BppFontSptr &font);
	/**
	 * Loads a font from an image archive file and adds it without a
	 * corresponding string cache.
	 * @param name      A name for the font. Used as the key value to find the
	 *                  font and its string cache later.
	 * @param fontpath  The path to the image archive file with the font data.
	 */
	void addWithoutCache(const std::string &name, const std::string &fontpath);
	/**
	 * Adds an existing font along with a newly created corresponding string
	 * cache.
	 * @param name   A name for the font. Used as the key value to find the
	 *               font and its string cache later.
	 * @param font   The font object.
	 */
	void addWithCache(const std::string &name, const BppFontSptr &font);
	/**
	 * Adds a newly loaded font along with a newly created corresponding string
	 * cache.
	 * @param name      A name for the font. Used as the key value to find the
	 *                  font and its string cache later.
	 * @param fontpath  The path to the image archive file with the font data.
	 */
	void addWithCache(const std::string &name, const std::string &fontpath);
	/**
	 * Adds a new name for an already added font. The font and its string cache
	 * will both be available from both names, and any other aliased names.
	 * @param existing  The name of the already added font to alias.
	 * @param newname   The additional name to give the existing font.
	 * @throw           FontNotFoundError   There is no font with the name in
	 *                                      @a existing inside this pool.
	 */
	void alias(const std::string &existing, const std::string &newname);
	/**
	 * Returns a shared pointer to a stored font, or an empty shared pointer
	 * if the font is not present.
	 * @param font  The name of the font to find.
	 * @return      The shared pointer to the font. It will be empty if there
	 *              is no font with the given name.
	 */
	BppFontSptr getFont(const std::string &font) const;
	/**
	 * Returns a shared pointer to a string cache, or an empty shared pointer
	 * if the font is not present.
	 * @param font  The name of the font with the string cache.
	 * @return      The shared pointer to the string cache. It will be empty if
	 *              there is no font with the given name.
	 */
	BppStringCacheSptr getStringCache(const std::string &font) const;
	/**
	 * Renders text without going through a string cache.
	 * Calls BppFont::render() and returns the result.
	 * @tparam String  The type of the string to render. It can be std::string
	 *                 for UTF-8, or std::u32string for UTF-32.
	 * @param font     The name of the font to use.
	 * @param str      The string to render.
	 * @param flags    The option flags. The default is to render varying width,
	 *                 fixed height text with each line aligned to the left.
	 * @return         The image with the rendered text.
	 */
	template <class String>
	BppImageSptr render(
		const std::string &font,
		const String &str,
		BppFont::Flags flags = BppFont::AlignLeft
	) const {
		FontAndCache fc;
		getFc(fc, font);
		try {
			return fc.fnt->render(str, flags);
		} catch (boost::exception &be) {
			be << FontName(font);
			throw;
		}
	}
	/**
	 * Gets text from a string cache if present, or renders from the font
	 * otherwise. Calls BppStringCache::text() and returns the result.
	 * @tparam String  The type of the string to render. It can be std::string
	 *                 for UTF-8, or std::u32string for UTF-32.
	 * @param font     The name of the font to use.
	 * @param str      The string to render.
	 * @param flags    The option flags. The default is to render varying width,
	 *                 fixed height text with each line aligned to the left.
	 * @return         The image with the rendered text.
	 */
	template <class String>
	ConstBppImageSptr text(
		const std::string &font,
		const String &str,
		BppFont::Flags flags = BppFont::AlignLeft
	) const {
		FontAndCache fc;
		getFc(fc, font);
		try {
			if (fc.sc) {
				return fc.sc->text(str, flags);
			}
			return fc.fnt->render(str, flags);
		} catch (boost::exception &be) {
			be << FontName(font);
			throw;
		}
	}
};

} } }

#endif        //  #ifndef BPPFONTPOOL_HPP
