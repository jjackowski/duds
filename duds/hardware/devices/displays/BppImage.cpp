/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */

#include <duds/hardware/devices/displays/BppImage.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace hardware { namespace devices { namespace displays {

std::ostream &operator << (std::ostream &os, const ImageLocation &il) {
	os << '(' << il.x << ',' << il.y << ')';
	return os;
}

std::ostream &operator << (std::ostream &os, const ImageDimensions &id) {
	os << '[' << id.w << ',' << id.h << ']';
	return os;
}

bool ImageDimensions::withinBounds(const ImageLocation &loc) const {
	return (loc.x >= 0) && (loc.x < w) && (loc.y >= 0) && (loc.y < h);
}


BppImage::BppImage(const ImageDimensions &id) :
img(bufferBlockSize(id.w, id.h)), dim(id),
blkPerLine(bufferBlocksPerLine(id.w))
{ }

BppImage::BppImage(BppImage &&mv) :
img(std::move(mv.img)), dim(mv.dim), blkPerLine(mv.blkPerLine) {
	mv.dim.w = mv.dim.h = 0;
	mv.blkPerLine = 0;
}

BppImage::BppImage(const BppImage &src) :
img(src.img), dim(src.dim), blkPerLine(src.blkPerLine) { }

BppImage::BppImage(
	const char *data
) {
	// data starts with width, then height, both as little endian shorts
	dim.w = data[0] | (data[1] << 8);
	dim.h = data[2] | (data[3] << 8);
	data += 4;
	// allocate space for image
	img.resize(bufferBlockSize(dim.w, dim.h));
	blkPerLine = bufferBlocksPerLine(dim.w);
	// move over image data one char at a time
	for (int y = 0; y < dim.h; ++y) {
		// find start of line in destination; may be more padded than source
		char *dest = (char*)bufferLine(y);
		for (int x = 0; x < dim.w; ++data, ++dest, x += 8) {
			// may copy past end of width, but space is allocated, so no problem
			*dest = *data;
		}
	}
}

BppImage::BppImage(
	const std::vector<char> &data
) {
	// assure a size large enough to hold the smallest image
	if (data.size() < 5) {
		DUDS_THROW_EXCEPTION(ImageTooSmallError());
	}
	// data starts with width, then height, both as little endian shorts
	dim.w = data[0] | (data[1] << 8);
	dim.h = data[2] | (data[3] << 8);
	// check input for adequate length
	if (data.size() < ((dim.w / 8 + ((dim.w % 8) ? 1 : 0)) * dim.h + 4)) {
		// too short
		DUDS_THROW_EXCEPTION(ImageTooSmallError() <<
			ImageErrorDimensions(dim)
		);
	}
	// allocate space for image
	img.resize(bufferBlockSize(dim.w, dim.h));
	blkPerLine = bufferBlocksPerLine(dim.w);
	std::vector<char>::const_iterator citer = data.cbegin() + 4;
	// move over image data one char at a time
	for (int y = 0; y < dim.h; ++y) {
		// find start of line in destination; may be more padded than source
		char *dest = (char*)bufferLine(y);
		for (int x = 0; x < dim.w; ++citer, ++dest, x += 8) {
			// may copy past end of width, but space is allocated, so no problem
			*dest = *citer;
		}
	}
}

std::shared_ptr<BppImage> BppImage::make(const char *data) {
	return std::make_shared<BppImage>(data);
}

std::shared_ptr<BppImage> BppImage::make(const std::vector<char> &data) {
	return std::make_shared<BppImage>(data);
}

BppImage BppImage::operator = (BppImage &&mv) {
	img = std::move(mv.img);
	dim = mv.dim;
	blkPerLine = mv.blkPerLine;
	mv.blkPerLine = 0;
	mv.dim.w = mv.dim.h = 0;
}

BppImage BppImage::operator = (const BppImage &src) {
	img = src.img;
	dim = src.dim;
	blkPerLine = src.blkPerLine;
}

void BppImage::swap(BppImage &other) {
	img.swap(other.img);
	std::swap(dim, other.dim);
	std::swap(blkPerLine, other.blkPerLine);
}

void BppImage::clear() {
	img.clear();
	dim.w = dim.h = 0;
	blkPerLine = 0;
}

void BppImage::resize(int width, int height) {
	if ((width < 0) || (height < 0)) {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(ImageDimensions(width, height))
		);
	}
	if ((width == 0) || (height == 0)) {
		clear();
	} else {
		/** @todo  Keep portion of image. */
		/** @bug   Image data is corrupt, but buffer is the correct size and usable. */
		img.resize(bufferBlockSize(dim.w, dim.h));
		blkPerLine = bufferBlocksPerLine(dim);
		dim.w = width;
		dim.h = height;
	}
}

const BppImage::PixelBlock *BppImage::buffer() const {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	return &(img[0]);
}

BppImage::PixelBlock *BppImage::buffer() {
	// re-use the const implementation
	return const_cast<BppImage::PixelBlock*>(((const BppImage*)this)->buffer());
}

void BppImage::bufferSpot(
	PixelBlock *(&addr),
	PixelBlock &mask,
	const ImageLocation &il
) {
	// bounds check
	if (!dim.withinBounds(il)) {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(dim) <<
			ImageErrorLocation(il)
		);
	}
	mask = 1 << (il.x % (sizeof(PixelBlock) * 8));
	addr = &(img[blkPerLine * il.y + (il.x / (sizeof(PixelBlock) * 8))]);
}

BppImage::Pixel BppImage::pixel(const ImageLocation &il, Direction dir) {
	if (dim.withinBounds(il)) {
		return Pixel(this, il, dir);
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(dim) <<
			ImageErrorLocation(il)
		);
	}
}

BppImage::ConstPixel BppImage::cpixel(
	const ImageLocation &il,
	Direction dir
) const {
	if (dim.withinBounds(il)) {
		return ConstPixel(this, il, dir);
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(dim) <<
			ImageErrorLocation(il)
		);
	}
}

BppImage::Pixel BppImage::begin() {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	return Pixel(this);
}

BppImage::Pixel BppImage::begin(Direction dir) {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	switch (dir) {
		case HorizInc:
			return Pixel(this, 0, 0, dir);
		case VertInc:
			return Pixel(this, dim.w - 1, 0, dir);
		case HorizDec:
			return Pixel(this, dim.w - 1, dim.h - 1, dir);
		case VertDec:
			return Pixel(this, 0, dim.h - 1, dir);
		default:
			DUDS_THROW_EXCEPTION(ImageError());
	}
}

BppImage::ConstPixel BppImage::cbegin() const {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	return ConstPixel(this);
}

BppImage::ConstPixel BppImage::cbegin(Direction dir) const {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	switch (dir) {
		case HorizInc:
			return ConstPixel(this, 0, 0, dir);
		case VertInc:
			return ConstPixel(this, dim.w - 1, 0, dir);
		case HorizDec:
			return ConstPixel(this, dim.w - 1, dim.h - 1, dir);
		case VertDec:
			return ConstPixel(this, 0, dim.h - 1, dir);
		default:
			DUDS_THROW_EXCEPTION(ImageError());
	}
}


bool BppImage::state(const ImageLocation &il) const {
	const PixelBlock *a;
	PixelBlock m;
	bufferSpot(a, m, il);
	return (*a & m) != 0;
}

void BppImage::state(const ImageLocation &il, bool s) {
	PixelBlock *a;
	PixelBlock m;
	bufferSpot(a, m, il);
	*a = (*a & ~m) | (s ? m : 0);
}

bool BppImage::togglePixel(const ImageLocation &il) {
	PixelBlock *a;
	PixelBlock m;
	bufferSpot(a, m, il);
	return ((*a = (*a & ~m) ^ m) & m) != 0;
}

void BppImage::blankImage(bool s) {
	PixelBlock v;
	if (s) {
		v = -1;
	} else {
		v = 0;
	}
	for (PixelBlock &b : img) {
		b = v;
	}
}

// -------------------------------------------------------------------

BppImage::ConstPixel::ConstPixel(
	const BppImage *img,
	const End
) : pos(-1, -1), blk(nullptr),
// ConstPixel stores a pointer to a non-const BppImage, even though  it will
// not modify the image, because it is the base class for Pixel
src(const_cast<BppImage*>(img))
{ }

BppImage::ConstPixel::ConstPixel(
	const BppImage *img,
	const ImageLocation &il,
	Direction d
) : dir(d),
// ConstPixel stores a pointer to a non-const BppImage, even though  it will
// not modify the image, because it is the base class for Pixel
src(const_cast<BppImage*>(img)) {
	// set the location; throw if out of bounds
	location(il);
	// do not request a buffer spot when making an end iterator
	if ((il.x != -1) && (il.y != -1)) {
		src->bufferSpot(blk, mask, pos.x, pos.y);
	}
}

BppImage::ConstPixel::ConstPixel(const BppImage::Pixel &p) :
ConstPixel((ConstPixel)p) { }

BppImage::ConstPixel &BppImage::ConstPixel::operator = (
	const BppImage::Pixel &p
) {
	return *this = (ConstPixel)p;
}

BppImage::ConstPixel &BppImage::ConstPixel::operator ++() {
	ImageDimensions dim = src->dimensions();
	/**
	 * @todo  All direction increments are suboptimal; can be improved.
	 *
	 * @todo  Maybe refactor; split incrmenets into functions to ease
	 *        implementation of decrement operator. Maybe work this into
	 *        ImageLocation?
	 */
	switch (dir) {
		case HorizInc:
			if (++pos.x >= dim.w) {
				if (++pos.y >= dim.h) {
					pos.x = pos.y = -1;
					blk = nullptr;
					break;
				}
				pos.x = 0;
				mask = 1;
				++blk;
			} else {
				mask <<= 1;
				if (!mask) {
					//src->bufferSpot(blk, mask, pos.x, pos.y);
					mask = 1;
					++blk;
				}
			}
			break;
		case VertInc:
			if (++pos.y >= dim.h) {
				if (--pos.x < 0) {
					pos.x = pos.y = -1;
					blk = nullptr;
					break;
				}
				pos.y = 0;
			}
			src->bufferSpot(blk, mask, pos.x, pos.y);
			break;
		case HorizDec:
			if (--pos.x < 0) {
				if (--pos.y < 0) {
					pos.x = pos.y = -1;
					blk = nullptr;
					break;
				}
				pos.x = dim.w - 1;
				mask = 1 << (pos.x % (sizeof(PixelBlock) * 8));
				--blk;
			} else {
				mask >>= 1;
				if (!mask) {
					//src->bufferSpot(blk, mask, pos.x, pos.y);
					mask = 1 << (pos.x % (sizeof(PixelBlock) * 8));
					--blk;
				}
			}
			break;
		case VertDec:
			if (--pos.y < 0) {
				if (++pos.x >= dim.w ) {
					pos.x = pos.y = -1;
					blk = nullptr;
					break;
				}
				pos.y = dim.h - 1;
			}
			src->bufferSpot(blk, mask, pos.x, pos.y);
			break;
		default:
			// PANIC!!!
			DUDS_THROW_EXCEPTION(ImageError());
	}
	return *this;
}

void BppImage::ConstPixel::location(const ImageLocation &il) {
	if (src->dimensions().withinBounds(il)) {
		pos = il;
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(src->dimensions()) <<
			ImageErrorLocation(il)
		);
	}
}

bool BppImage::ConstPixel::state() const {
	if (blk) {
		return (*blk & mask) != 0;
	} else {
		DUDS_THROW_EXCEPTION(ImageIteratorEndError());
	}
}

void BppImage::Pixel::state(bool s) {
	if (blk) {
		*blk = (*blk & ~mask) | (s ? mask : 0);
	} else {
		DUDS_THROW_EXCEPTION(ImageIteratorEndError());
	}
}

bool BppImage::Pixel::toggle() {
	if (blk) {
		return ((*blk = (*blk & ~mask) ^ mask) & mask) != 0;
	} else {
		DUDS_THROW_EXCEPTION(ImageIteratorEndError());
	}
}

} } } }
