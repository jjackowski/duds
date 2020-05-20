/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/menu/MenuOutputAccess.hpp>
#include <duds/ui/graphics/BppStringCache.hpp>

namespace duds { namespace ui { namespace menu { namespace renderers {

/**
 * Base class for errors from the BppMenuRenderer.
 */
struct BppMenuRendererError : virtual std::exception, virtual boost::exception { };

/**
 * The destination image provided for the menu is too small to render a single
 * menu item.
 */
struct BppMenuDestinationTooSmall : BppMenuRendererError { };

/**
 * The menu render is configured to render text, but has no string cache.
 */
struct BppMenuLacksStringCache : BppMenuRendererError { };

/**
 * Renders menus to bit-per-pixel images. This class is not thread-safe; if
 * rendering from multiple threads is required, use a separate instance for
 * each thread.
 *
 * The menu items can be ordered vertically (default) or horizontally. If
 * horizontal, all items will be in the same row. Either way, each item will
 * have the same columns, each the same size, which may include one or more
 * of the following in the given order:
 * -# Selection and disabled icons
 *    - Included if one or both icons are provided, otherwise omitted.
 *      - selectedIcon(const duds::ui::graphics::ConstBppImageSptr &)
 *      - disabledIcon(const duds::ui::graphics::ConstBppImageSptr &)
 *    - Sized to fit the larger icon.
 *    - Selection can optionally be shown by inverting the selected item using
 *      the InvertSelected configuration flag. This works independently of the
 *      selection icon, but if neither inversion nor a selection icon is used,
 *      then the user can only tell which item is selected if only one item
 *      is shown.
 * -# Toggle on and off icons
 *    - Included if one or both icons are provided, otherwise omitted.
 *      - toggledOnIcon(const duds::ui::graphics::ConstBppImageSptr &)
 *      - toggledOffIcon(const duds::ui::graphics::ConstBppImageSptr &)
 *    - Sized to fit the larger icon.
 * -# BppMenuIconItem icon
 *    - Icons provided by the menu items.
 *    - Sized based on the dimensions given to
 *      iconDimensions(const duds::ui::graphics::ImageDimensions &).
 *    - Omitted if the dimensions are
 *      @ref duds::ui::graphics::ImageDimensions::empty() "empty" (zero size).
 * -# Icon to text margin
 *    - Optional extra space set by call to iconTextMargin(std::uint16_t).
 * -# Item label text
 *    - Sized by subtracting the width of everything else to fit within the
 *      destination image and using the largest height.
 *    - Omitted if the non-default configuration flag DoNotShowText is used.
 *    - Requires a string cache be provided to either a constructor or to
 *      stringCache(const duds::ui::graphics::BppStringCacheSptr &).
 * -# Value margin
 *    - Optional extra space between the label text and value text set by
 *      call to valueMargin(std::uint16_t).
 *    - Only included if the item value text has its own column.
 * -# Item value text
 *    - This column is omitted unless a non-zero size specified by call to
 *      valueWidth(std::uint16_t).
 *    - If omitted, the value text will be rendered with the label text. This
 *      will likely need some more development work to look good.
 *    - The text may optionally be right justified by using the configuration
 *      flag ValueRightJusified.
 * -# Item margin
 *    - Optional extra space between menu items set by call to
 *      itemMargin(std::uint16_t).
 *    - Only present as a column on horizontally oriented menus. Vertically
 *      oriented menus place the space as rows of pixels between the items.
 *
 * Each renderer instance caches information used to define the bounds of each
 * MenuItem based on the instance's configuration, the provided font inside
 * the string cache, and the size of the output image. When the configuration
 * changes in a way that could affect the size of the items, and when the
 * size of the output image changes, the internal item size data is recomputed.
 * As a result, it works best to configure everything once and use the same
 * size destination image for all menu renders. In that case, rendering
 * different menus with one renderer incurs no size-recompute penalty.
 *
 * Menus are oriented vertically (one item per row) by default. In this case,
 * the number of menu items shown is based upon the computed size of each item
 * and the size of the destination image. For horizontally oriented (one item
 * per column) menus, the maximum number of items to show must be provided in
 * a call to maxVisible(std::uint16_t) before rendering. Each item will take
 * the same amount of space on the destination image.
 *
 * @todo    Horizontal ordering needs testing; assume broken for now.
 * @todo    Support rendering the selected menu item's description text
 *          somewhere.
 * @author  Jeff Jackowski
 */
class BppMenuRenderer {
public:
	typedef duds::general::BitFlags<struct BppMenuFlags>  Flags;
	/**
	 * Items are arranged horizontally instead of vertically.
	 */
	static constexpr Flags HorizontalList          = Flags::Bit(0);
	/**
	 * The selected item will be rendered inverted.
	 */
	static constexpr Flags InvertSelected          = Flags::Bit(1);
	/**
	 * Right justify value text when values are placed in a column separate
	 * from the item label. If the value column width is zero or not provided,
	 * this flag will have no effect.
	 */
	static constexpr Flags ValueRightJusified      = Flags::Bit(2);
	/**
	 * Only show icons, not text, for the menu items. This will prevent values,
	 * as well as text naming a menu item, from being shown.
	 */
	static constexpr Flags DoNotShowText           = Flags::Bit(3);
private:
	/**
	 * When set, indicates that internal dimension values have been calculated.
	 * This avoids the need to recalculate them every time the menu is rendered.
	 */
	static constexpr Flags Calculated              = Flags::Bit(15);
	/**
	 * The second byte (the one next to the least significant byte) is reserved
	 * for internal flags.
	 */
	static constexpr Flags InternalMask            = Flags(0xFF00);
	/**
	 * Icon used to denote a selected menu item.
	 */
	duds::ui::graphics::ConstBppImageSptr selIcon;
	/**
	 * Icon used to denote a disabled menu item.
	 */
	duds::ui::graphics::ConstBppImageSptr disIcon;
	/**
	 * Icon used to denote a menu item in its toggled off state.
	 */
	duds::ui::graphics::ConstBppImageSptr togOffIcon;
	/**
	 * Icon used to denote a menu item in its toggled on state.
	 */
	duds::ui::graphics::ConstBppImageSptr togOnIcon;
	/**
	 * Pre-rendered strings for menu text.
	 */
	duds::ui::graphics::BppStringCacheSptr cache;
	/**
	 * Computed dimensions of a complete menu item.
	 */
	duds::ui::graphics::ImageDimensions itemDim;
	/**
	 * The size of a menu item's text.
	 */
	duds::ui::graphics::ImageDimensions textDim;
	/**
	 * Destination dimensions; used to tell when to recompute dimensions of
	 * parts of the menu.
	 */
	duds::ui::graphics::ImageDimensions destDim =
		duds::ui::graphics::ImageDimensions(0, 0);
	/**
	 * The size of a menu item's icon.
	 */
	duds::ui::graphics::ImageDimensions iconDim =
		duds::ui::graphics::ImageDimensions(0, 0);
	/**
	 * Option flags that modify how the menu is rendered.
	 */
	Flags flgs;
	/**
	 * The maximum width of the selection icon and the disabled icon.
	 */
	std::uint16_t selDisWidth = 0;
	/**
	 * The maximum width of the toggle icons.
	 */
	std::uint16_t toggleWidth = 0;
	/**
	 * The width of the value column. If zero, the value will be combined with
	 * the item label and will not have its own column.
	 */
	std::uint16_t valWidth = 0;
	/**
	 * The margin in pixels between the label and value columns,
	 */
	std::uint16_t valMg = 0;
	/**
	 * The margin in pixels between menu items.
	 */
	std::uint16_t itemMg = 0;
	/**
	 * Space in pixels to have between icons and text.
	 */
	std::uint16_t iconTxMg = 0;
	/**
	 * Number of items to display.
	 */
	std::uint16_t items = 0;
	/**
	 * The number of pixels to show of a partially visible menu item.
	 */
	std::uint16_t fracshow;
	/**
	 * Recalculates the dimension values needed to render a menu that fits into
	 * the given dimensions.
	 * @param fitDim  The dimensions in which the menu must fit.
	 * @throw BppMenuDestinationTooSmall  Current settings will not fit the
	 *                                    menu item(s) into @a fitDim.
	 */
	void recalculateDimensions(duds::ui::graphics::ImageDimensions fitDim);
public:
	/**
	 * Constructs a new menu renderer without a string cache or font. This can
	 * work well for menus that won't render any text, or will be further
	 * configured later.
	 * @param cfg       The inital configuration flags.
	 */
	BppMenuRenderer(Flags cfg = Flags::Zero()) : flgs(cfg & ~InternalMask) { }
	/**
	 * Constructs a new menu renderer.
	 * @param cachePtr  The cache of rendered strings to use for menu text.
	 * @param cfg       The inital configuration flags.
	 */
	BppMenuRenderer(
		const duds::ui::graphics::BppStringCacheSptr &cachePtr,
		Flags cfg = Flags::Zero()
	) : cache(cachePtr), flgs(cfg & ~InternalMask) { }
	/**
	 * Constructs a new menu renderer; best for horizontally oriented menus.
	 * @param cachePtr  The cache of rendered strings to use for menu text.
	 * @param vmItems   The maximum number of visible menu items. This is only
	 *                  useful if the menu is oriented horizontally.
	 * @param cfg       The inital configuration flags.
	 */
	BppMenuRenderer(
		const duds::ui::graphics::BppStringCacheSptr &cachePtr,
		int vmItems,
		Flags cfg = Flags::Zero()
	) : cache(cachePtr), items(vmItems), flgs(cfg & ~InternalMask) { }
	/**
	 * Returns the configuration flags for this renderer.
	 */
	Flags flags() const {
		return flgs;
	}
	/**
	 * Changes the configuration flags for this renderer.
	 */
	void flags(Flags cfg) {
		flgs = cfg & ~InternalMask;
	}
	/**
	 * Returns the currently set icon used to indicate item selection. This
	 * shows the user which option they are poised to chose.
	 */
	const duds::ui::graphics::ConstBppImageSptr &selectedIcon() const {
		return selIcon;
	}
	/**
	 * Changes the optional selection icon used to inform the user what item
	 * they have selected and are poised to chose.
	 */
	void selectedIcon(const duds::ui::graphics::ConstBppImageSptr &img) {
		selIcon = img;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the currently configured disabled icon image.
	 */
	const duds::ui::graphics::ConstBppImageSptr &disabledIcon() const {
		return disIcon;
	}
	/**
	 * Changes the optional icon used to inform the user that a menu item is
	 * currently disabled.
	 */
	void disabledIcon(const duds::ui::graphics::ConstBppImageSptr &img) {
		disIcon = img;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the icon used to signify a toggle state in the off position.
	 */
	const duds::ui::graphics::ConstBppImageSptr &toggledOffIcon() const {
		return togOffIcon;
	}
	/**
	 * Changes the option icon used to signify a toggle state in the off
	 * position.
	 */
	void toggledOffIcon(const duds::ui::graphics::ConstBppImageSptr &img) {
		togOffIcon = img;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the icon used to signify a toggle state in the on position.
	 */
	const duds::ui::graphics::ConstBppImageSptr &toggledOnIcon() const {
		return togOnIcon;
	}
	/**
	 * Changes the option icon used to signify a toggle state in the on
	 * position.
	 */
	void toggledOnIcon(const duds::ui::graphics::ConstBppImageSptr &img) {
		togOnIcon = img;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the dimensions used for rendering icons stored inside menu items
	 * derived from BppIconItem.
	 */
	const duds::ui::graphics::ImageDimensions &iconDimensions() const {
		return iconDim;
	}
	/**
	 * Sets the dimensions used for rendering icons stored inside menu items
	 * derived from BppIconItem. Use an empty size to prevent icons from
	 * being rendered.
	 * @post  If an empty size is used, @a iconTextMargin will be set to zero.
	 * @sa    BppMenuIconItem
	 * @sa    GenericBppMenuIconItem
	 */
	void iconDimensions(const duds::ui::graphics::ImageDimensions &dim);
	/**
	 * Returns the size of the margin between icons from menu items and the text
	 * label of the item.
	 */
	std::uint16_t iconTextMargin() const {
		return iconTxMg;
	}
	/**
	 * Sets the size of the margin between icons from menu items and the text
	 * label of the item.
	 * @sa  BppMenuIconItem
	 * @sa  GenericBppMenuIconItem
	 */
	void iconTextMargin(std::uint16_t itm) {
		iconTxMg = itm;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the string cache currently used to render menu item text.
	 */
	const duds::ui::graphics::BppStringCacheSptr &stringCache() const {
		return cache;
	}
	/**
	 * Changes the string cache used to render menu item text.
	 */
	void stringCache(const duds::ui::graphics::BppStringCacheSptr &sc) {
		cache = sc;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the maximum number of visible items to show on the menu; try to
	 * avoid calling.
	 * @pre   Either the menu is horizontally oriented and maxVisible() has been
	 *        called, or the menu has already been rendered.
	 * @note  The number of visible items may change for vertically oriented
	 *        menus if the size of the image used to hold the rendered menu
	 *        image changes. This update occurs inside recalculateDimensions().
	 */
	std::uint16_t maxVisible() const {
		return items;
	}
	/**
	 * Sets the maximum number of visible items for horizontally oriented menus.
	 * For vertically oriented menus, this does nothing.
	 * @param i  The number of items to show.
	 */
	void maxVisible(std::uint16_t i);
	/**
	 * Returns the width in pixels of the value column.
	 */
	std::uint16_t valueWidth() const {
		return valWidth;
	}
	/**
	 * Changes the width in pixels of the value column.
	 * @param w  The new width of the pixels column. If zero, the column will
	 *           be omitted.
	 */
	void valueWidth(std::uint16_t w) {
		valWidth = w;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the margin in pixels between a menu item's label text and its
	 * value text.
	 */
	std::uint16_t valueMargin() const {
		return valMg;
	}
	/**
	 * Changes the margin in pixels between a menu item's label text and its
	 * value text. This margin is only used if the value width is non-zero.
	 * @param m  The new margin size.
	 * @sa valueWidth(std::int16_t)
	 */
	void valueMargin(std::uint16_t m) {
		valMg = m;
		flgs.clear(Calculated);
	}
	/**
	 * Returns the margin in pixels placed bewteen each menu item.
	 */
	std::uint16_t itemMargin() const {
		return itemMg;
	}
	/**
	 * Changes the margin in pixels placed bewteen each menu item.
	 * @param im  The new margin size.
	 */
	void itemMargin(std::uint16_t im) {
		itemMg = im;
		flgs.clear(Calculated);
	}
	/**
	 * Renders a menu to the given image.
	 * @param dest  The destination image. If its size is different than the
	 *              last image used, or if this is the first time rendering,
	 *              size data for the menu items and their parts will be
	 *              recomputed.
	 * @param mova  Output access to the menu to render.
	 * @throw BppMenuLacksStringCache     The renderer is configured to show
	 *                                    text, but doesn't have the string
	 *                                    cache needed to render the text.
	 * @throw BppMenuDestinationTooSmall  Current settings will not fit the
	 *                                    menu item(s) into @a dest.
	 */
	void render(
		duds::ui::graphics::BppImageSptr &dest,
		duds::ui::menu::MenuOutputAccess &mova
	);
	/**
	 * Renders a menu to the given image.
	 * @param dest  The destination image. If its size is different than the
	 *              last image used, or if this is the first time rendering,
	 *              size data for the menu items and their parts will be
	 *              recomputed.
	 * @param mov   The MenuOutput of the menu to render. An access object to
	 *              the menu output is made inside this function.
	 * @throw BppMenuLacksStringCache     The renderer is configured to show
	 *                                    text, but doesn't have the string
	 *                                    cache needed to render the text.
	 * @throw BppMenuDestinationTooSmall  Current settings will not fit the
	 *                                    menu item(s) into @a dest.
	 */
	void render(
		duds::ui::graphics::BppImageSptr &dest,
		duds::ui::menu::MenuOutput &mov
	) {
		duds::ui::menu::MenuOutputAccess mova(mov, items);
		render(dest, mova);
	}
	/**
	 * Renders a menu of the given size.
	 * @param dim   The size of the image that will hold the rendered menu.
	 *              If the size is different than the last size used, or if
	 *              this is the first time rendering, size data for the menu
	 *              items and their parts will be recomputed.
	 * @param mova  Output access to the menu to render.
	 * @return      An image with the rendered menu.
	 * @throw BppMenuLacksStringCache     The renderer is configured to show
	 *                                    text, but doesn't have the string
	 *                                    cache needed to render the text.
	 * @throw BppMenuDestinationTooSmall  Current settings will not fit the
	 *                                    menu item(s) into @a dest.
	 */
	duds::ui::graphics::BppImageSptr render(
		const duds::ui::graphics::ImageDimensions &dim,
		duds::ui::menu::MenuOutputAccess &mova
	) {
		graphics::BppImageSptr img = graphics::BppImage::make(dim);
		render(img, mova);
		return img;
	}
	/**
	 * Renders a menu of the given size.
	 * @param dim   The size of the image that will hold the rendered menu.
	 *              If the size is different than the last size used, or if
	 *              this is the first time rendering, size data for the menu
	 *              items and their parts will be recomputed.
	 * @param mov   The MenuOutput of the menu to render. An access object to
	 *              the menu output is made inside this function.
	 * @return      An image with the rendered menu.
	 * @throw BppMenuLacksStringCache     The renderer is configured to show
	 *                                    text, but doesn't have the string
	 *                                    cache needed to render the text.
	 * @throw BppMenuDestinationTooSmall  Current settings will not fit the
	 *                                    menu item(s) into @a dest.
	 */
	duds::ui::graphics::BppImageSptr render(
		const duds::ui::graphics::ImageDimensions &dim,
		duds::ui::menu::MenuOutput &mov
	) {
		duds::ui::menu::MenuOutputAccess mova(mov, items);
		return render(dim, mova);
	}
};

} } } }
