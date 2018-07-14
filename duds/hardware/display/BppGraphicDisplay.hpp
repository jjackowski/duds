/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <duds/hardware/display/BppImage.hpp>

namespace duds { namespace hardware { namespace display {

/**
 * Base class for all errors specifically from a graphic display.
 */
struct GraphicDisplayError : virtual std::exception, virtual boost::exception { };

/**
 * Frame buffer dimensions. 
 */
typedef boost::error_info<struct Info_FrameDimensions, ImageDimensions>
	ImageErrorFrameDimensions;

/**
 * Base class for bit-per-pixel graphic displays.
 * @author  Jeff Jackowski
 */
class BppGraphicDisplay {
protected:
	/**
	 * The frame buffer.
	 */
	BppImage frmbuf;
	/**
	 * Writes out the given image to the display and updates the image in
	 * @a frmbuf to match.
	 * Called by write() after ensuring the dimensions of @a img and @a frmbuf
	 * match.
	 * @param img  The new image to show.
	 * @pre   The size of @a img and @a frmbuf are the same.
	 * @post  The image in @a frmbuf matches the image in @a img.
	 */
	virtual void outputFrame(const BppImage *img) = 0;
	/**
	 * Construct with an empty frame buffer.
	 */
	BppGraphicDisplay() = default;
	/**
	 * Construct with a frame buffer of the specified size.
	 * @param dim  The size to make the frame buffer.
	 */
	BppGraphicDisplay(const ImageDimensions &dim) : frmbuf(dim) { }
public:
	/**
	 * Provides access to the image in the frame buffer.
	 */
	const BppImage &frame() const {
		return frmbuf;
	}
	/**
	 * Returns the width of the frame buffer.
	 */
	int width() const {
		return frmbuf.width();
	}
	/**
	 * Returns the height of the frame buffer.
	 */
	int height() const {
		return frmbuf.height();
	}
	/**
	 * Returns the dimensions of the frame buffer.
	 */
	const ImageDimensions &dimensions() const {
		return frmbuf.dimensions();
	}
	/**
	 * Writes the new image to the display.
	 * @param img  The new image to show on the display.
	 * @pre   The size of @a img and @a frmbuf are the same.
	 * @post  The image in @a frmbuf matches the image in @a img.
	 * @throw DisplaySizeError  The dimensions of the supplied image do not
	 *                          match the frame buffer's dimensions.
	 */
	void write(const BppImage *img);
	/**
	 * Writes the new image to the display.
	 * @param img  The new image to show on the display.
	 * @pre   The size of @a img and @a frmbuf are the same.
	 * @post  The image in @a frmbuf matches the image in @a img.
	 * @throw DisplaySizeError  The dimensions of the supplied image do not
	 *                          match the frame buffer's dimensions.
	 */
	void write(const std::shared_ptr<const BppImage> &img) {
		write(img.get());
	}
};

} } }
