/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#ifndef BPPPOSITIONINDICATOR_HPP
#define BPPPOSITIONINDICATOR_HPP

#include <duds/ui/graphics/BppImage.hpp>
#include <boost/exception/info.hpp>

namespace duds { namespace ui { namespace graphics {

/**
 * The base class for errors from the BppPositionIndicator.
 */
struct BppPositionIndicatorError : virtual std::exception, virtual boost::exception { };

/**
 * An attempt was made to set a minimum marker size that is less than one, or
 * that is too large to fit within the current dimensions.
 */
struct BppPositionIndicatorMarkerSizeError : BppPositionIndicatorError { };

/**
 * An attempt was made to set the dimensions to a size that is too small to fit
 * the position mark with enough room to allow it to move.
 */
struct BppPositionIndicatorDimensionTooSmall : BppPositionIndicatorError { };

/**
 * An attempt was made to set the range to zero.
 */
struct BppPositionIndicatorRangeTooSmall : BppPositionIndicatorError { };

/**
 * Error attribute with the length of the position mark in pixels.
 */
typedef boost::error_info<struct Info_IndicatorMinimumSize, std::uint16_t>
	PositionMarkMinimumSize;

/**
 * Error attribute with the range of positions for a position indicator.
 */
typedef boost::error_info<struct Info_IndicatorRange, std::uint16_t>
	IndicatorRange;

/**
 * A generalized position indicator that can be used to render a simple scroll
 * bar, progress bar, and other similar things.
 *
 * The indicator is drawn within given dimensions. If the dimensions are wider
 * than they are tall, the position marker will move horizontally. Otherwise,
 * it will move vertically. The whole area within the dimenions will be
 * rendered. The marker will use set (true) pixels by default, and the rest
 * of the area will use the opposite pixel state.
 *
 * The position marker is drawn to represent a start and end position. These
 * positions are placed within a range of 0 to range() - 1 so that they work
 * well with common containers, like std::vector. The value for range() needs
 * to be set by the program. The length of the marker will be scaled to
 * represent the distance between the start and end positions, but will never
 * be less than the set minimum length. The end position may be omitted to
 * use the start position as a mid-point; in this case, the marker will always
 * be its minimum length.
 *
 * The marker will only be drawn in the first or last pixel along its axis if
 * the position includes either the begining of the range or the end of the
 * range, respectively. This is intended to be a hint to the user that the
 * extreme end has been reached, and to make this hint visible even when large
 * ranges are used and rounding errors are present.
 *
 * @author  Jeff Jackowski
 */
class BppPositionIndicator {
	/**
	 * The location to render the indicator.
	 */
	ImageLocation ipos;
	/**
	 * The size of the indicator.
	 */
	ImageDimensions idim;
	/**
	 * The range of position values that may be used.
	 */
	std::uint16_t rng;
	/**
	 * The minimum length of the position marker.
	 */
	std::uint16_t minSize = 4;
	/**
	 * The pixel state used for the background, the area not covered by the
	 * indicator mark/slider thingy.
	 */
	bool bstate = false;
	/**
	 * Computes the pixel position, along the length of the indicator, for the
	 * position mark.
	 * @param pos  The position from 0 to @a rng - 1.
	 * @param len  The length of the indicator in pixels.
	 */
	int position(int pos, int len) const;
public:
	/**
	 * Makes a new indicator with its position, dimensions, and range left
	 * uninitialized. The minimum size is initialized to 4, and the background
	 * pixel state to clear (false).
	 * @post  Indicator position (position(const ImageLocation &)),
	 *        dimensions (dimensions(const ImageDimensions &)), and range
	 *        (range(std::uint16_t) or maxPosition(std::uint16_t)) must be
	 *        set prior to rendering.
	 * @note  The dimensions must be set prior to changing the minimum indicator
	 *        size.
	 */
	BppPositionIndicator() = default;
	/**
	 * Makes a new indicator with its position and dimensions left
	 * uninitialized.
	 * @post  Indicator position (position(const ImageLocation &)) and
	 *        dimensions (dimensions(const ImageDimensions &)) must be
	 *        set prior to rendering.
	 * @param minMarkerSize     The minimum size, in pixels, of the position
	 *                          marker that will be drawn onto the indicator.
	 * @param backgroundState   The pixel state for the background of the
	 *                          indicator. The marker state will be the opposite.
	 */
	BppPositionIndicator(
		std::uint16_t minMarkerSize,
		bool backgroundState = false
	) : minSize(minMarkerSize), bstate(backgroundState) { }
	/**
	 * Makes a new position indicator.
	 * @param indicatorPos      The position of where to render the indicator.
	 * @param indicatorDim      The dimensions of the indicator.
	 * @param minMarkerSize     The minimum size, in pixels, of the position
	 *                          marker that will be drawn onto the indicator.
	 * @param backgroundState   The pixel state for the background of the
	 *                          indicator. The marker state will be the opposite.
	 * @throw BppPositionIndicatorDimensionTooSmall
	 *          The dimensions offer fewer than 3 pixels to move in either axis.
	 * @throw BppPositionIndicatorMarkerSizeError
	 *          The given minimum position marker size is zero, or is too large
	 *          to fit within the given dimensions.
	 */
	BppPositionIndicator(
		const ImageLocation &indicatorPos,
		const ImageDimensions &indicatorDim,
		std::uint16_t minMarkerSize = 4,
		bool backgroundState = false
	);
	/**
	 * Returns the upper left position where the indictor will be drawn.
	 */
	const ImageLocation &position() const {
		return ipos;
	}
	/**
	 * Changes the upper left position where the indictor will be drawn.
	 */
	void position(const ImageLocation &pos) {
		ipos = pos;
	}
	/**
	 * Returns the dimensions (size) of the rendered indicator.
	 */
	const ImageDimensions &dimensions() const {
		return idim;
	}
	/**
	 * Changes the dimensions of the rendered indicator.
	 * @param dim  The new size to render the indicator. The position marker
	 *             will be horizontally oriented if the dimensions are wider
	 *             than they are tall, and vertical otherwise.
	 * @throw BppPositionIndicatorDimensionTooSmall
	 *          The dimensions offer fewer than (2 + minMarkerSize())
	 *          pixels to move in either axis.
	 */
	void dimensions(const ImageDimensions &dim);
	/**
	 * Returns the minimum size of the position marker in pixels.
	 */
	std::uint16_t minMarkerSize() const {
		return minSize;
	}
	/**
	 * Changes the minimum size of the position marker in pixels.
	 * @param size  The new size in pixels.
	 * @throw BppPositionIndicatorMarkerSizeError
	 *          The given minimum position marker size is zero, or is too large
	 *          to fit within the current dimensions.
	 */
	void minMarkerSize(std::uint16_t size);
	/**
	 * Returns the pixel state used for the background of the indicator. The
	 * position marker will use the opposite state.
	 */
	bool backgroundState() const {
		return bstate;
	}
	/**
	 * Changes the pixel state used for the background of the indicator. The
	 * position marker will use the opposite state.
	 * @param s  The new pixel state for the background.
	 */
	void backgroundState(bool s) {
		bstate = s;
	}
	/**
	 * Returns the pixel state used for the position marker of the indicator.
	 * The background will use the opposite state.
	 */
	bool markerState() const {
		return !bstate;
	}
	/**
	 * Changes the pixel state used for the position marker of the indicator.
	 * The background will use the opposite state.
	 * @param s  The new pixel state for the marker.
	 */
	void markerState(bool s) {
		bstate = !s;
	}
	/**
	 * Returns the range of positions that will be represented by the indicator.
	 * The ends of the range are zero and range() - 1.
	 */
	std::uint16_t range() const {
		return rng + 1;
	}
	/**
	 * Changes the range of positions that will be represented by the indicator.
	 * The ends of the range are zero and range() - 1. This works well with
	 * sizes from standard C++ containers.
	 * @param r  The new range.
	 * @throw BppPositionIndicatorRangeTooSmall
	 *          The given range is zero.
	 */
	void range(std::uint16_t r);
	/**
	 * Returns the maximum position that will be represented by the indicator.
	 * The ends of the range are zero and maxPosition().
	 */
	std::uint16_t maxPosition() const {
		return rng;
	}
	/**
	 * Returns the maximum position that will be represented by the indicator.
	 * The ends of the range are zero and maxPosition().
	 * @param p  The new maximum position value.
	 */
	void maxPosition(std::uint16_t p) {
		rng = p;
	}
	/**
	 * True when the indicator's marker will move horizontally.
	 */
	bool horizontal() const {
		return idim.w > idim.h;
	}
	/**
	 * True when the indicator's marker will move vertically.
	 */
	bool vertical() const {
		return idim.h >= idim.w;
	}
	/**
	 * Renders the indicator with the marker showing the given position.
	 * The start position is used for the top or left of the marker, and the
	 * end position is for the bottom or right. If the end position is equal or
	 * less than the start position, then the start is used as a mid-point for
	 * the marker. The marker is always at least minMarkerSize() pixels long.
	 *
	 * A common look for a progress bar is achived by using 0 for the start
	 * position, and the progress value for the end position.
	 *
	 * @pre   Either range(std::uint16_t) or maxPosition(std::uint16_t) has been
	 *        called.
	 *
	 * @param dest   The destination image.
	 * @param start  The start position. It should be between 0 and range() - 1.
	 *               Values beyond the range will be clipped to the ends of the
	 *               range.
	 * @param end    The end position. It should be bewteen @a start and
	 *               range() - 1 for a marker with its size scaled based on the
	 *               positions. It should be 0 if the marker needs to be placed
	 *               only based on the start position.
	 */
	void render(BppImage *dest, int start, int end = 0);
	/**
	 * Renders the indicator with the marker showing the given position.
	 * The start position is used for the top or left of the marker, and the
	 * end position is for the bottom or right. If the end position is equal or
	 * less than the start position, then the start is used as a mid-point for
	 * the marker. The marker is always at least minMarkerSize() pixels long.
	 *
	 * A common look for a progress bar is achived by using 0 for the start
	 * position, and the progress value for the end position.
	 *
	 * @pre   Either range(std::uint16_t) or maxPosition(std::uint16_t) has been
	 *        called.
	 *
	 * @param dest   The destination image.
	 * @param start  The start position. It should be between 0 and range() - 1.
	 *               Values beyond the range will be clipped to the ends of the
	 *               range.
	 * @param end    The end position. It should be bewteen @a start and
	 *               range() - 1 for a marker with its size scaled based on the
	 *               positions. It should be 0 if the marker needs to be placed
	 *               only based on the start position.
	 */
	void render(const BppImageSptr &dest, int start, int end = 0) {
		render(dest.get(), start, end);
	}
};

} } }

#endif        //  #ifndef BPPPOSITIONINDICATOR_HPP
