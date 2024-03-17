/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/menu/renderers/BppMenuRenderer.hpp>
#include <duds/ui/menu/renderers/BppIconItem.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace menu { namespace renderers {

namespace graphics = duds::ui::graphics;

constexpr BppMenuRenderer::Flags BppMenuRenderer::ScrollBarMask;
constexpr BppMenuRenderer::Flags BppMenuRenderer::ScrollBarNeverHides;
constexpr BppMenuRenderer::Flags BppMenuRenderer::HorizontalList;
constexpr BppMenuRenderer::Flags BppMenuRenderer::InvertSelected;
constexpr BppMenuRenderer::Flags BppMenuRenderer::ValueRightJustified;
constexpr BppMenuRenderer::Flags BppMenuRenderer::DoNotShowText;
constexpr BppMenuRenderer::Flags BppMenuRenderer::ScrollBarShown;
constexpr BppMenuRenderer::Flags BppMenuRenderer::Calculated;
constexpr BppMenuRenderer::Flags BppMenuRenderer::InternalMask;

void BppMenuRenderer::iconDimensions(
	const graphics::ImageDimensions &dim
) {
	iconDim = dim;
	if (iconDim.empty()) {
		iconTxMg = 0;
	}
	flgs.clear(Calculated);
}

void BppMenuRenderer::maxVisible(std::uint16_t i) {
	if (flgs & HorizontalList) {
		items = i;
		flgs.clear(Calculated);
	}
}

void BppMenuRenderer::addScrollBar(
	std::uint16_t width,
	std::uint16_t margin,
	std::uint16_t minsize,
	ScrollBarPlacement place
) {
	// check for no change
	if ((width == scrollWidth) && (margin == scrollMg)) return;
	scrollWidth = width;
	scrollMg = margin;
	posInd = std::make_unique<duds::ui::graphics::BppPositionIndicator>(
		minsize
	);
	flgs.setMasked(place, ScrollBarMask);
	flgs.clear(Calculated);
}

void BppMenuRenderer::removeScrollBar() {
	posInd.reset();
	scrollMg = scrollWidth = 0;
	flgs.clear(Calculated | ScrollBarShown);
}

void BppMenuRenderer::recalculateDimensions(
	duds::ui::graphics::ImageDimensions fitDim
) {
	// the dimensions may need modification; scroll bar will be removed
	duds::ui::graphics::ImageDimensions fit(fitDim);
	int width = fitDim.w;
	int place = (flgs & ScrollBarMask).flags();
	// using scroll bar?
	if (posInd) {
		// vertical placement for scroll bar?
		if (place < ScrollBottom) {
			// remove its width
			width -= scrollWidth + scrollMg;
			fit.w = width;
		} else {
		// horizontal scroll?
			// remove its size
			fit.h -= scrollWidth + scrollMg;
		}
	}
	if (flgs & HorizontalList) {
		// max width for each item
		width /= items;
	}
	// proposed text dimensions; do not alter dimensions to use until they fit
	duds::ui::graphics::ImageDimensions propTextDim;
	// compute text dimensions even if no text displayed; use result to test
	// for display area that is too small
	propTextDim.w = width;
	// showing a separate value column?
	if (valWidth) {
		propTextDim.w -= valWidth + valMg;
	}
	// use font dimensions for minimum height
	graphics::ImageDimensions fntDim =
		cache->font()->estimatedMaxCharacterSize();
	propTextDim.h = fntDim.h;
	// selection and disabled icons use the same space
	graphics::ImageDimensions iDim =
		duds::ui::graphics::MaxExtent(selIcon, disIcon);
	selDisWidth = iDim.w;
	propTextDim.w -= iDim.w;
	propTextDim.h = std::max(propTextDim.h, iDim.h);
	// next, toggle icons
	iDim = duds::ui::graphics::MaxExtent(togOnIcon, togOffIcon);
	toggleWidth = iDim.w;
	propTextDim.w -= iDim.w;
	propTextDim.h = std::max(propTextDim.h, iDim.h);
	// when horizontal, the margin between items is part of the width
	if (flgs & BppMenuRenderer::HorizontalList) {
		propTextDim.w -= itemMg;
	}
	// showing an item specific icon column?
	if (!iconDim.empty()) {
		propTextDim.w -= iconDim.w + iconTxMg;
		propTextDim.h = std::max(propTextDim.h, iconDim.h);
	}
	// too short or small to fit? must fit at least one character for text
	// width, and not exceed height
	if ((propTextDim.h > fit.h) ||
		((flgs & DoNotShowText) && (propTextDim.w < 0)) ||
		((~flgs & DoNotShowText) && (propTextDim.w < fntDim.w))
	) {
		DUDS_THROW_EXCEPTION(BppMenuDestinationTooSmall() <<
			graphics::ImageErrorSourceDimensions(propTextDim) <<
			graphics::ImageErrorTargetDimensions(fit)
		);
	}
	// showing item text?
	if (~flgs & DoNotShowText) {
		// use proposed text dimensions
		textDim = propTextDim;
		// width fits full space
		itemDim.w = width;
	} else {
		// no text dimensions
		textDim.w = textDim.h = 0;
		// width is adequate to fit what will be displayed
		itemDim.w = width - propTextDim.w;
	}
	itemDim.h = textDim.h;
	// compute number of items visible, and determine if a partial item
	// is visible
	int fullshow;
	if (flgs & HorizontalList) {
		// use fit width that excludes scroll bar if present
		fullshow = fit.w / (itemDim.w + itemMg);
		fracshow = fit.w % (itemDim.w + itemMg);
	} else {
		// use fit width that excludes scroll bar if present
		fullshow = fit.h / (itemDim.h + itemMg);
		fracshow = fit.h % (itemDim.h + itemMg);
	}
	items = fullshow;
	// only allow fractionally visible items if at least two full items
	// can be shown, and the fractional part is at least a few pixels
	if ((fullshow > 1) && (fracshow > (2 + itemMg))) {
		items += (fracshow ? 1 : 0);
	} else {
		fracshow = 0;
	}
	// scroll bar
	if (posInd) {
		// size the scroll bar to fit
		posInd->position(duds::ui::graphics::ImageLocation(
			place == ScrollRight ? fitDim.w - scrollWidth : 0,
			place == ScrollBottom ? fitDim.h - scrollWidth : 0
		));
		posInd->dimensions(duds::ui::graphics::ImageDimensions(
			place < ScrollBottom ? scrollWidth : fitDim.w,
			place < ScrollBottom ? fitDim.h : scrollWidth
		));
		flgs.set(ScrollBarShown);
	}
	// store new output dimension
	destDim = fitDim;
	// flag recalcuation done
	flgs.set(Calculated);
}

void BppMenuRenderer::render(
	graphics::BppImageSptr &dest,
	duds::ui::menu::MenuOutputAccess &mova
) {
	// need to have the string cache but don't have one?
	if ((~flgs & DoNotShowText) && !cache) {
		DUDS_THROW_EXCEPTION(BppMenuLacksStringCache());
	}
	if (!dest) {
		DUDS_THROW_EXCEPTION(BppMenuDestinationMissing());
	}
	// destination image dimensions will be used in a number of places
	const graphics::ImageDimensions &fitDim = dest->dimensions();
	// ensure item dimensions have been computed
	if ((~flgs & Calculated) || (destDim != fitDim)) {
		recalculateDimensions(fitDim);
		// different number of items fit than shown previously?
		if (items != mova.maxVisible()) {
			// fix it
			mova.maxVisible(items);
		}
	}
	int place = (flgs & ScrollBarMask).flags();
	int scrollSize = scrollMg + scrollWidth;
	// have scroll bar that hides?
	if (posInd && (~flgs & ScrollBarNeverHides) && (place < ScrollBottom)) {
		// shown previously but not needed now?
		if (mova.showingAll()) {
			if (flgs & ScrollBarShown) {
				// enlarge text label area
				itemDim.w += scrollSize;
				textDim.w += scrollSize;
				flgs.clear(ScrollBarShown);
			}
			// no space used by missing scroll bar
			scrollSize = 0;
		} else if ((~flgs & ScrollBarShown) && !mova.showingAll()) {
			// reduce text label area
			itemDim.w -= scrollSize;
			textDim.w -= scrollSize;
			flgs.set(ScrollBarShown);
		}
	}
	// should have caused recalculateDimensions() to throw
	assert((destDim.w >= itemDim.w) && (destDim.h >= itemDim.h));
	// ensure image is clear
	dest->clearImage();
	// If dimension allows for a fraction of an item, then all of top/first
	// item should be shown and partial of bottom/last, until the last menu
	// item is visible and the selection is about half-way there among visible
	// items. Once selection is within half way across visible items, the items
	// shift so that the last item is compltely visible.
	graphics::BppImageSptr img;
	// the visible index of the fractionally visible item
	int fracidx;
	// is there an item that will be partially visible?
	if (fracshow && ((items - 1) < mova.size())) {
		// last visible item will be rendered, and selected item is more than
		// half-way to the end?
		if (mova.showingLast() && (mova.selectedVisible() > (mova.size() / 2))) {
			// the fractional item will be the first in the visible list
			fracidx = 0;
		} else {
			// the fractional item will be the last in the visible list
			fracidx = mova.size() - 1;
			// write first items to the destinatiom image directly
			img = dest;
		}
	} else {
		fracidx = -1;
		// write items to the destinatiom image directly
		img = dest;
	}
	// make useful scroll bar data to avoid some more conditionals later
	int startX;
	if (place == ScrollLeft) {
		startX = scrollSize;
	} else {
		startX = 0;
	}
	graphics::ImageLocation pos(startX, 0);
	int idx = 0;
	// render each item
	for (duds::ui::menu::MenuItem* mitem : mova) {
		// useful for advancing the position later, especially for horizontal
		// menus
		graphics::ImageLocation startPos(pos);
		// working on a fractionally visible item?
		if (idx == fracidx) {
			// render into an image large enough for one menu item
			pos = graphics::ImageLocation(0, 0);
			img = graphics::BppImage::make(itemDim);
		}
		// note selection status
		bool selected = mova.selectedVisible() == idx;
		// figure out if rendering should be inverted or not
		//bool selectInv = selected && (flgs & InvertSelected);
		// selection icon rendering
		if (selIcon && selected) {
			// selection icon is never rendered inverted, but inverting the
			// item will be done later for the whole item
			img->write(
				selIcon,
				pos
				// maybe center vertically ???
			);
		}
		// disabled icon rendering
		if (disIcon && mitem->isDisabled()) {
			// disabled icon is never rendered inverted; item cannot be selected
			img->write(disIcon, pos);
		}
		if (selIcon || disIcon) {
			pos.x += selDisWidth;
		}
		// toggled icon rendering
		if (togOffIcon || togOnIcon) {
			if (mitem->isToggle()) {
				// render toggle off icon?
				if (togOffIcon && !mitem->isToggledOn()) {
					img->write(togOffIcon, pos);
				} else if (togOnIcon && mitem->isToggledOn()) {
					img->write(togOnIcon, pos);
				}
			}
			// update the position for rendering the next item
			pos.x += toggleWidth;
		}
		// if menu item icons should be rendered . . .
		if (!iconDim.empty()) {
			// . . . see if the item has an icon
			BppIconItem *iitem = dynamic_cast<BppIconItem*>(mitem);
			if (iitem && iitem->icon()) {
				// have icon
				img->write(
					iitem->icon(),
					pos,
					iitem->icon()->dimensions().minExtent(iconDim)
				);
			}
			// update the position for rendering the next item
			pos.x += iconDim.w + iconTxMg;
		}
		// render text label
		if (~flgs & DoNotShowText) {
			graphics::ConstBppImageSptr text;
			// have label text?
			if (!mitem->label().empty()) {
				if (valWidth || mitem->value().empty() || (flgs & ValueRightJustified)) {
					// get the rendered label text
					text = cache->text(mitem->label());
				}
				if (!text && !(flgs & ValueRightJustified)) {
					// get the rendered label and value text
					text = cache->text(mitem->label() + " " + mitem->value());
				} else {
					// render value text and label text separately
					graphics::ConstBppImageSptr valtext;
					valtext = cache->text(mitem->value());
					// compute width of value
					graphics::ImageDimensions vdim(
						textDim.minExtent(text->dimensions())
					);
					vdim.w = textDim.w - valMg - vdim.w;
					// any space to fit value?
					if (vdim.w > 0) {
						img->write(
							valtext,
							graphics::ImageLocation(
								textDim.w - valtext->dimensions().w,
								pos.y
							),
							vdim.minExtent(valtext->dimensions())
						);
					}
				}
				// put it in the destination image
				img->write(
					text,
					pos,
					// dimensions must not exceed text area or size of the text
					textDim.minExtent(text->dimensions())
				);
			}
			// put value text in own column?
			if (valWidth) {
				// update the position for rendering the value
				pos.x += textDim.w + valMg;
				// have value text?
				if (!mitem->value().empty()) {
					// render the value text
					text = cache->text(
						mitem->value(),
						(flgs & ValueRightJustified) ?
						graphics::BppFont::AlignRight :
						graphics::BppFont::AlignLeft
					);
					// remaining dimensions to fill
					graphics::ImageDimensions valDim(itemDim.w - pos.x, itemDim.h);
					// text left justified, exactly fits or is too long?
					if (!(flgs & ValueRightJustified) || text->width() >= valDim.w) {
						// write out the text; justification is irrelevant
						img->write(text, pos, valDim);
						// no need to blank out any area
					} else {
						// write out the text to the right
						img->write(
							text,
							graphics::ImageLocation(
								pos.x + valDim.w - text->width(),
								pos.y
							)
						);
					}
				}
			}
		}
		// invert selected item?
		if (selected && (flgs & InvertSelected)) {
			if (flgs & HorizontalList) {
				img->drawBox(startPos, itemDim, graphics::BppImage::OpXor);
			} else {
				// faster than drawBox()
				img->invertLines(pos.y, itemDim.h);
				// scroll margin in inverted region?
				if (scrollSize && (place < ScrollBottom)) {
					// blank the margin only; scroll bar render will do the rest
					img->drawBox(
						place == ScrollRight ? destDim.w - scrollSize : 0,
						pos.y,         // y
						scrollSize,    // width == margin for scroll
						itemDim.h,     // height
						false          // state
					);
				}
			}
		}
		// deal with fractionally visible item
		if (idx == fracidx) {
			// first item?
			if (idx == 0) {
				if (flgs & HorizontalList) {
					dest->write(
						img,
						graphics::ImageLocation(0, 0),
						graphics::ImageLocation(itemDim.w - fracshow, 0),
						graphics::ImageDimensions(fracshow, itemDim.h)
					);
					// advance to the right
					pos.x = fracshow + itemMg;
				} else {
					dest->write(
						img,
						graphics::ImageLocation(0, 0),
						graphics::ImageLocation(0, itemDim.h - fracshow),
						graphics::ImageDimensions(itemDim.w, fracshow)
					);
					// advance downward
					pos.x = startX;
					pos.y = fracshow + itemMg;
				}
				// write the rest of the items to the destination image directly
				img = dest;
			}
			// last item?
			else {
				if (flgs & HorizontalList) {
					dest->write(
						img,
						startPos,
						graphics::ImageLocation(0, 0),
						graphics::ImageDimensions(fracshow, itemDim.h)
					);
				} else {
					dest->write(
						img,
						startPos,
						graphics::ImageLocation(0, 0),
						graphics::ImageDimensions(itemDim.w, fracshow)
					);
				}
			}
		} else {
			// move rendering position
			if (flgs & HorizontalList) {
				// advance to the right
				pos.x = startPos.x + itemDim.w + itemMg;
			} else {
				// advance downward
				pos.x = startX;
				pos.y += itemDim.h + itemMg;
			}
		}
		// keep track of display index
		++idx;
	}
	// scroll bar
	if (posInd && (flgs & ScrollBarShown)) {
		// using visible() seems like a good idea, but the item indices used in
		// the next line are from a vector that includes all the menu's items,
		// even if not visible, so using visible() will result in the scroll bar
		// indicating the end of the menu early if items are hidden
		posInd->range(mova.menu()->size()); //visible());
		posInd->render(dest, mova.firstIndex(), mova.lastIndex());
	}
}

} } } }
