/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#include <duds/ui/graphics/BppImage.hpp>
#include <duds/general/BitFlags.hpp>
#include <duds/general/Spinlock.hpp>
#include <unordered_map>

namespace duds { namespace ui { namespace graphics {

/**
 * Renders strings using a font made of BppImage objects for glyphs.
 * The glyph images may come from a BppImage archive file or stream, may
 * be provided using the add() function, or can be generated in a
 * renderGlyph() function implemented by a derived class. A cache of glyph
 * images is maintained by this base class. The renderGlyph() function is
 * called whenever a glyph is requested but not already in the cache of
 * glyph images.
 *
 * @author  Jeff Jackowski
 */
class BppFont : boost::noncopyable {
protected:
	/**
	 * The glyph images keyed by character.
	 */
	std::unordered_map<char32_t, ConstBppImageSptr>  glyphs;
	/**
	 * Used for thread safety.
	 */
	duds::general::Spinlock block;
	/**
	 * Called to render the requested glyph when it is not present in the
	 * @a glyphs map. The base implementation always throws GlyphNotFoundError.
	 * along with a Character attribute. Implementors should do the same when
	 * the glyph cannot be supplied.
	 * @note         The thread will have a lock on @a block when this function
	 *               is called.
	 * @throw        GlyphNotFoundError  The glyph is not provided by the font.
	 * @param gc     The character code of the glyph.
	 * @return       The glyph image.
	 */
	virtual BppImageSptr renderGlyph(char32_t gc);
public:
	BppFont() = default;
	/**
	 * @copydoc load(const std::string &)
	 */
	BppFont(const std::string &path) {
		load(path);
	}
	/**
	 * @copydoc load(std::istream &)
	 */
	BppFont(std::istream &is) {
		load(is);
	}
	/**
	 * Returns a shared pointer to a new BppFont object.
	 */
	static std::shared_ptr<BppFont> make() {
		return std::make_shared<BppFont>();
	}
	/**
	 * Returns a shared pointer to a new BppFont object constructed using the
	 * BppFont(const std::string &) constructor.
	 */
	static std::shared_ptr<BppFont> make(const std::string &path) {
		return std::make_shared<BppFont>(path);
	}
	/**
	 * Returns a shared pointer to a new BppFont object constructed using the
	 * BppFont(std::istream &) constructor.
	 */
	static std::shared_ptr<BppFont> make(std::istream &is) {
		return std::make_shared<BppFont>(is);
	}
	/**
	 * Loads glyphs from an image archive in the specified file. The glyphs
	 * from the archive will augment what is already stored in this object,
	 * and will replace glyphs if there is a collision.
	 * @note  Only images with a name that is just a single character will be
	 *        kept. All other images will be discarded.
	 * @bug   Character values above 127 from files made by the
	 *        @ref DUDStoolsBppic will not be made availble. This is because the
	 *        compiler will encode the character code using UTF-8, and the code
	 *        here does not decode UTF-8.
	 * @param path  The path of the archive file to load.
	 * @throw ImageArchiveStreamError
	 *        Failed to open the file.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	void load(const std::string &path);
	/**
	 * Loads glyphs from an input stream. The glyphs from the archive stream
	 * will augment what is already stored in this object,
	 * and will replace glyphs if there is a collision.
	 * @param is  The input stream that will provide the image archive.
	 * @throw ImageNotArchiveStreamError
	 *        The stream does not have an image archive stream.
	 * @throw ImageArchiveStreamTruncatedError
	 *        The stream appears to have an incomplete copy of the archive
	 *        stream. Any images fully read prior to the error will be
	 *        available.
	 * @throw ImageArchiveUnsupportedVersionError
	 *        The software does not support the claimed archive version.
	 */
	void load(std::istream &is);
	/**
	 * Adds or replaces a glyph in the font.
	 * @todo  change param names; the image is the glyph.
	 * @param gc     The character code of the glyph.
	 * @param img    The image to store.
	 */
	void add(char32_t gc, const BppImageSptr &img);
	/**
	 * Adds or replaces a glyph in the font.
	 * @param gc     The character code of the glyph.
	 * @param img    The image to store. The shared pointer will be moved.
	 */
	void add(char32_t gc, BppImageSptr &&img);
	/**
	 * Returns the glyph of the specified character code.
	 * @param gc     The character code of the glyph.
	 * @return       A shared pointer to the requested glyph image.
	 * @throw        GlyphNotFoundError  The glyph is not provided by the font.
	 */
	const ConstBppImageSptr &get(char32_t gc);
	/**
	 * Returns the glyph of the specified character code.
	 * @param gc     The character code of the glyph.
	 * @return       A shared pointer to the requested glyph image, or an
	 *               empty shared pointer if the font lacks the glyph.
	 */
	ConstBppImageSptr tryGet(char32_t gc);
	/**
	 * Returns a somewhat decent estimate of the largest size of a character
	 * without actually inspecting all characters. If the result is zero, the
	 * estimation failed.
	 * @note  This simplistic font code lacks any real font metrics, so the
	 *        result from this function may not be very good. It should do well
	 *        with fixed width fonts. It may fail completely for fonts that are
	 *        actually a set of icons and lack many characters.
	 */
	ImageDimensions estimatedMaxCharacterSize();
	/**
	 * Option flags that affect how text is rendered.
	 */
	typedef duds::general::BitFlags<struct BppFontRenderingFlags>  Flags;
	/**
	 * All glyphs rendered with the same width using the maximum width of the
	 * glyphs used in the string.
	 */
	static constexpr Flags FixedWidth = Flags::Bit(0);
	/**
	 * Compute fixed width individually for each line, so each line may have
	 * a different width per glyph. Takes precedence over FixedWidth.
	 */
	static constexpr Flags FixedWidthPerLine = Flags::Bit(1);
	/**
	 * Each line will have the height of its tallest glyph rather than the
	 * tallest glyph of the entire string.
	 */
	static constexpr Flags VariableHeight = Flags::Bit(2);
	/**
	 * Align each line to the left. This is the default.
	 */
	static constexpr Flags AlignLeft = Flags::Zero();
	/**
	 * Center each line in the resulting image.
	 */
	static constexpr Flags AlignCenter = Flags::Bit(3);
	/**
	 * Align each line to the right.
	 */
	static constexpr Flags AlignRight = Flags::Bit(4);
	/**
	 * All alignment flags.
	 */
	static constexpr Flags AlignMask = AlignCenter | AlignRight;
	/**
	 * Renders the given text using this object's font.
	 *
	 * The newline character can be used to denote the start of another line.
	 * Lines are only made explicitly.
	 *
	 * Variable sized glyphs are supported. Glyphs are aligned vertically along
	 * their lower edge. This places shorter glyphs lower than taller ones.
	 *
	 * @param text   The text to render.
	 * @param flags  The option flags. The default is to render varying width,
	 *               fixed height text with each line aligned to the left.
	 * @return       A new image with the rendered text.
	 * @bug          Only tested for rendering a single line of text.
	 * @throw        GlyphNotFoundError  A glyph in @a text is not provided
	 *                                   by the font.
	 */
	BppImageSptr render(const std::string &text, Flags flags = AlignLeft);
	/*
	BppImageSptr renderSingleLine(const std::string &text, Flags flags = AlignLeft);
	*/
};

typedef std::shared_ptr<BppFont>  BppFontSptr;

} } }
