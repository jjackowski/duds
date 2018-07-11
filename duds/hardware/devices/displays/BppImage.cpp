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
	} else if ((width != dim.w) || (height != dim.h)) {
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

ImageLocation BppImage::startPosition(Direction dir) const {
	return startPosition(ImageLocation(0, 0), dim, dir);
}

ImageLocation BppImage::startPosition(
	const ImageLocation &origin,
	const ImageDimensions &size,
	Direction dir
) {
	switch (dir) {
		case HorizInc:
			return ImageLocation(origin.x, origin.y);
		case VertInc:
			return ImageLocation(origin.x + size.w - 1, origin.y);
		case HorizDec:
			return ImageLocation(origin.x + size.w - 1, origin.y + size.h - 1);
		case VertDec:
			return ImageLocation(origin.x, origin.y + size.h - 1);
		default:
			DUDS_THROW_EXCEPTION(ImageError());
	}
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
	return Pixel(this, startPosition(dir), dir);
}

BppImage::Pixel BppImage::begin(
	const ImageLocation &origin,
	const ImageDimensions &size,
	Direction dir
) {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	return Pixel(this, origin, size, startPosition(dir), dir);
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
	return ConstPixel(this, startPosition(dir), dir);
}

BppImage::ConstPixel BppImage::cbegin(
	const ImageLocation &origin,
	const ImageDimensions &size,
	Direction dir
) const {
	if (img.empty()) {
		DUDS_THROW_EXCEPTION(ImageZeroSizeError());
	}
	return ConstPixel(this, origin, size, startPosition(ImageLocation(0,0), size, dir), dir);
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

static bool opset(bool dest, bool src) {
	return src;
}

static bool opnot(bool dest, bool src) {
	return !src;
}

static bool opand(bool dest, bool src) {
	return dest && src;
}

static bool opor(bool dest, bool src) {
	return dest || src;
}

static bool opxor(bool dest, bool src) {
	return dest ^ src;
}

const BppImage::OpFunction BppImage::OpFunctions[OpTotal] = {
	opset,
	opnot,
	opand,
	opor,
	opxor
};

void BppImage::write(
	const BppImage * const src,
	const ImageLocation &destLoc,
	const ImageLocation &srcLoc,
	const ImageDimensions &srcSize,
	Direction srcDir,
	Operation op
) {
	if ((op < OpSet) || (op > OpXor)) {
		// bad data
		DUDS_THROW_EXCEPTION(ImageError());
	}
	// source iterator
	ConstPixel siter = src->cbegin(srcLoc, srcSize, srcDir);
	ImageDimensions destSize;
	if ((srcDir == VertInc) || (srcDir == HorizDec)) {
		// the iteration direction rotates the image 90 degrees; the dimensions
		// are swapped for writing to the destination image
		destSize = srcSize.swappedAxes();
	} else {
		destSize = srcSize;
	}
	// destination iterator
	Pixel diter = begin(destLoc, destSize);
	// iteratate over the images
	for (; siter != EndPixel(); ++diter, ++siter) {
		*diter = OpFunctions[op](*diter, *siter);
	}
}

void BppImage::write(
	const BppImage * const src,
	const ImageLocation &dest,
	Direction srcDir,
	Operation op
) {
	ImageDimensions s(src->dimensions());
	bool swapped = false;
	if ((srcDir == VertInc) || (srcDir == HorizDec)) {
		// the iteration direction rotates the image 90 degrees; the dimensions
		// are swapped for comaprisons to the destination image
		s.swapAxes();
		swapped = true;
	}
	// source dimensions clipped to available destination size
	ImageDimensions d(
		// (int16_t - int16_t) is type int
		std::min((int)s.w, dim.w - dest.x),
		std::min((int)s.h, dim.h - dest.y)
	);
	if (swapped) {
		d.swapAxes();
	}
	// do the writing
	write(src, dest, ImageLocation(0, 0), d, srcDir, op);
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
) : dir(d), orig(0, 0), dim(img->dimensions()),
// ConstPixel stores a pointer to a non-const BppImage, even though  it will
// not modify the image, because it is the base class for Pixel
src(const_cast<BppImage*>(img)) {
	// set the location; throw if out of bounds
	location(il);
	// do not request a buffer spot when making an end iterator
	if ((il.x != -1) && (il.y != -1)) {
		src->bufferSpot(blk, mask, pos);
	}
}

BppImage::ConstPixel::ConstPixel(
	const BppImage *img,
	const ImageLocation &o,
	const ImageDimensions &s,
	const ImageLocation &p,
	Direction d
) : dir(d),
// ConstPixel stores a pointer to a non-const BppImage, even though  it will
// not modify the image, because it is the base class for Pixel
src(const_cast<BppImage*>(img)) {
	// set the location, origin, and dimensions; throw if out of bounds
	origdimloc(o, s, p);
	/*  broken, but should it be fixed?
	// do not request a buffer spot when making an end iterator
	if ((il.x != -1) && (il.y != -1)) {
		src->bufferSpot(blk, mask, orig + pos);
	}
	*/
}

BppImage::ConstPixel::ConstPixel(const BppImage::Pixel &p) :
ConstPixel((ConstPixel)p) { }

BppImage::ConstPixel &BppImage::ConstPixel::operator = (
	const BppImage::Pixel &p
) {
	return *this = (ConstPixel)p;
}

BppImage::ConstPixel &BppImage::ConstPixel::operator ++() {
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
				// was fine when origin was (0,0) and dim matched the image
				//mask = 1;
				//++blk;
				src->bufferSpot(blk, mask, orig + pos);
			} else {
				mask <<= 1;
				// advance to next block of pixels?
				if (!mask) {
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
			src->bufferSpot(blk, mask, orig + pos);
			break;
		case HorizDec:
			if (--pos.x < 0) {
				if (--pos.y < 0) {
					pos.x = pos.y = -1;
					blk = nullptr;
					break;
				}
				pos.x = dim.w - 1;
				// was fine when origin was (0,0) and dim matched the image
				//mask = 1 << (pos.x % (sizeof(PixelBlock) * 8));
				//--blk;
				src->bufferSpot(blk, mask, orig + pos);
			} else {
				mask >>= 1;
				// advance to next block of pixels?
				if (!mask) {
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
			src->bufferSpot(blk, mask, orig + pos);
			break;
		default:
			// PANIC!!!
			DUDS_THROW_EXCEPTION(ImageError());
	}
	return *this;
}

void BppImage::ConstPixel::location(const ImageLocation &il) {
	if (dim.withinBounds(il)) {
		// store the location
		pos = il;
		// set internal data to use new location
		src->bufferSpot(blk, mask, orig + pos);
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(dim) <<
			ImageErrorLocation(il)
		);
	}
}

void BppImage::ConstPixel::origin(const ImageLocation &il) {
	if (src->dimensions().withinBounds(il + dim)) {
		// store the new origin
		orig = il;
		// update internal data to use new location
		src->bufferSpot(blk, mask, orig + pos);
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(dim) <<
			ImageErrorLocation(il)
		);
	}
}

void BppImage::ConstPixel::dimensions(const ImageDimensions &d) {
	if (d.withinBounds(pos) &&
		src->dimensions().withinBounds(pos + d)
	) {
		// store the new dimensions
		dim = d;
		// spot in image buffer unchanged
	} else {
		DUDS_THROW_EXCEPTION(ImageBoundsError() <<
			ImageErrorDimensions(d) <<
			ImageErrorLocation(pos)
		);
	}
}

static constexpr ImageLocation OneByOne(1, 1);

void BppImage::ConstPixel::origdimloc(
	const ImageLocation &o,
	const ImageDimensions &d,
	const ImageLocation &p
) {
	if (d.withinBounds(p) &&
		src->dimensions().withinBounds(o - OneByOne + d)
	) {
		// store the new data
		orig = o;
		dim = d;
		pos = p;
		// set internal data to use new location
		src->bufferSpot(blk, mask, orig + pos);
	} else {
		if (d.withinBounds(p)) {
			DUDS_THROW_EXCEPTION(ImageBoundsError() <<
				ImageErrorDimensions(src->dimensions()) <<
				ImageErrorLocation(o + d)
			);
		} else {
			DUDS_THROW_EXCEPTION(ImageBoundsError() <<
				ImageErrorDimensions(d) <<
				ImageErrorLocation(p)
			);
		}
	}
}

bool BppImage::ConstPixel::operator == (const ConstPixel &cp) const {
	// check for a possible end iterator
	if (!src || !cp.src) {
		// check positions only; dimensions and origin do not matter
		return (
			(pos.x == -1) && (pos.y == -1) &&
			(cp.pos.x == -1) && (cp.pos.y == -1)
		);
	}
	// all data must match
	return (src == cp.src) && (pos == cp.pos) &&
		(orig == cp.orig) && (dim == cp.dim);
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
