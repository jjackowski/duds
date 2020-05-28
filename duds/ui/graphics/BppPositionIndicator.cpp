/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */

#include <duds/ui/graphics/BppPositionIndicator.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

BppPositionIndicator::BppPositionIndicator(
	const ImageLocation &indicatorPos,
	const ImageDimensions &indicatorDim,
	std::uint16_t minMarkSize,
	bool backgroundState
) : ipos(indicatorPos), idim(indicatorDim), bstate(backgroundState)
{
	// the indicator needs at least 3 pixels to move about
	if (indicatorDim.empty() || ((indicatorDim.h < 3) && (indicatorDim.w < 3)))  {
			DUDS_THROW_EXCEPTION(BppPositionIndicatorDimensionTooSmall() <<
			ImageErrorDimensions(indicatorDim)
		);
	}
	// checks sizes; may throw
	this->minMarkerSize(minMarkSize);
}

void BppPositionIndicator::dimensions(
	const ImageDimensions &dim
) {
	if (dim.empty() || ((dim.h >= dim.w) && (minSize > (dim.h - 2))) ||
		(!(dim.h >= dim.w) && (minSize > (dim.w - 2)))
	) {
		DUDS_THROW_EXCEPTION(BppPositionIndicatorDimensionTooSmall() <<
			ImageErrorDimensions(dim) << PositionMarkMinimumSize(minSize)
		);
	}
	idim = dim;
}

void BppPositionIndicator::minMarkerSize(std::uint16_t size) {
	if ((size < 1) ||
		(vertical() && (size > (idim.h - 2))) ||
		(!vertical() && (size > (idim.w - 2)))
	) {
		DUDS_THROW_EXCEPTION(BppPositionIndicatorMarkerSizeError() <<
			PositionMarkMinimumSize(size)
		);
	}
	minSize = size;
}

void BppPositionIndicator::range(std::uint16_t r) {
	if (r < 1) {
		DUDS_THROW_EXCEPTION(BppPositionIndicatorRangeTooSmall() <<
			IndicatorRange(r)
		);
	}
	rng = r - 1;
}

int BppPositionIndicator::position(int pos, int len) const {
	// position is at the start of the range?
	if (pos <= 0) {
		return 0;
	}
	// position is at the end of the range?
	if (pos >= rng) {
		return len;
	}
	// position is somewhere in the middle of the range.
	// Find position for shortened length to ensure start and end positions are
	// distinctly different from in-between positions, even after rounding
	// errors and such.
	return pos * (len - 2) / rng + 1;
}

void BppPositionIndicator::render(BppImage *dest, int start, int end) {
	// top or left
	ImageLocation tl = ipos;
	ImageDimensions isize;
	// length to fill
	int len;
	if (vertical()) {
		len = idim.h;
		isize.w = idim.w;
	} else {
		// make location seem to be vertical
		tl.swapAxes();
		len = idim.w;
		isize.w = idim.h;
	}
	int sp, ep;
	// indicate a range?
	if (end > start) {
		// start position
		sp = position(start, len);
		// end position
		ep = position(end, len);
		// length too short?
		if ((ep - sp) < minSize) {
			if (sp == 0) {
				// start the minimum length from the begining
				ep = minSize;
			} else if (ep == len) {
				// start the minimum length from the end
				sp = ep - minSize;
			} else {
				// find the mid-point of the range and use the signle position
				// code below
				start = end = (start + end) / 2;
				ep = 0;
			}
		}
		// set position & dimension of indicator
		if (ep) {
			tl.y += sp;
			isize.h = ep - sp;
		}
	} else {
		ep = 0;
	}
	// indicate a single position, or a range too small for minimum length?
	if (end <= start) {
		// remove minimum length from the indicator's movement range
		len -= minSize;
		// find top or left position of indicator
		sp = position(start, len);
		tl.y += sp;
		// always use minimum size
		isize.h = minSize;
	}
	// code above works on vertical data
	if (horizontal()) {
		// switch to horizontal
		tl.swapAxes();
		isize.swapAxes();
		// blank indicator; probably more efficent that blanking two sections
		// because of the image data arrangement
		dest->drawBox(ipos, idim, bstate);
	} else if (sp) {
		// blank indicator
		// top first
		dest->drawBox(ipos, ImageDimensions(idim.w, sp - 1), bstate);
	}
	// render indicator
	dest->drawBox(tl, isize, !bstate);
	if ((ep != len) && vertical()) {
		// blank indicator
		// bottom next
		dest->drawBox(
			ImageLocation(tl.x, tl.y + isize.h + 1),
			ImageDimensions(idim.w, idim.h - isize.h - sp - 1),
			bstate
		);
	}

}

} } }
