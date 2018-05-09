/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#ifndef BPPIMAGE_HPP
#define BPPIMAGE_HPP

#include <vector>
#include <cstdint>
#include <boost/exception/info.hpp>

namespace duds { namespace hardware { namespace devices { namespace displays {

/**
 * Stores a location within an image.
 * @todo  Maybe put location increment functions here? Will Need dimentions and
 *        direction to work.
 * @author  Jeff Jackowski
 */
struct ImageLocation {
	/**
	 * Horizontal coordinate.
	 */
	std::int16_t x;
	/**
	 * Vertical coordinate.
	 */
	std::int16_t y;
	/**
	 * Construct uninitialized.
	 */
	ImageLocation() = default;
	/**
	 * Construct with the given location.
	 */
	constexpr ImageLocation(std::int16_t px, std::int16_t py) : x(px), y(py) { }
	/**
	 * Obvious equality operator.
	 */
	constexpr bool operator == (const ImageLocation &il) const {
		return (x == il.x) && (y == il.y);
	}
	/**
	 * Obvious inequality operator.
	 */
	constexpr bool operator != (const ImageLocation &il) const {
		return (x != il.x) || (y != il.y);
	}
};

/**
 * Writes an ImageLocation object to a stream in human readable form.
 */
std::ostream &operator << (std::ostream &os, const ImageLocation &il);

/**
 * Swaps the values of two ImageLocation objects.
 */
inline void swap(ImageLocation &l0, ImageLocation &l1) {
	std::swap(l0.x, l1.x);
	std::swap(l0.y, l1.y);
}

/**
 * Stores the dimensions of an image.
 * @author  Jeff Jackowski
 */
struct ImageDimensions {
	/**
	 * Width
	 */
	std::int16_t w;
	/**
	 * Height
	 */
	std::int16_t h;
	/**
	 * Construct uninitialized.
	 */
	ImageDimensions() = default;
	/**
	 * Construct with the given dimensions.
	 */
	constexpr ImageDimensions(std::int16_t dw, std::int16_t dh) : w(dw), h(dh) { }
	/**
	 * Obvious equality operator.
	 */
	constexpr bool operator == (const ImageDimensions &id) const {
		return (w == id.w) && (h == id.h);
	}
	/**
	 * Obvious inequality operator.
	 */
	constexpr bool operator != (const ImageDimensions &id) const {
		return (w != id.w) || (h != id.h);
	}
	/**
	 * Returns true if the given location is within the bounds specified by
	 * this object.
	 */
	bool withinBounds(const ImageLocation &loc) const;
};

/**
 * Writes an ImageDimensions object to a stream in human readable form.
 */
std::ostream &operator << (std::ostream &os, const ImageDimensions &id);

/**
 * Swaps the values of two ImageDimensions objects.
 */
inline void swap(ImageDimensions &d0, ImageDimensions &d1) {
	std::swap(d0.w, d1.w);
	std::swap(d0.h, d1.h);
}

/**
 * The base class for errors related to the use of images.
 */
struct ImageError : virtual std::exception, virtual boost::exception { };
/**
 * Data with an image to parse was too short to hold the image.
 */
struct ImageTooSmallError : ImageError { };
/**
 * A problem with image bounds, such as the use of a location beyond the
 * image's dimensions.
 */
struct ImageBoundsError : ImageError { };
/**
 * Indicates the image has zero size when an operation requires some image
 * data.
 */
struct ImageZeroSizeError : ImageBoundsError { };
/**
 * The Pixel or ConstPixel iterator object was dereferenced when in at the end.
 */
struct ImageIteratorEndError : ImageBoundsError { };

/**
 * An image location relevant to the error.
 */
typedef boost::error_info<struct Info_ImageLocation, ImageLocation>
	ImageErrorLocation;
/**
 * Image dimensions relevant to the error.
 */
typedef boost::error_info<struct Info_ImageDimensions, ImageDimensions>
	ImageErrorDimensions;


/**
 * An image that uses a single bit to represent the state of each pixel; a
 * black @b or white picture.
 *
 * The image data is stored as a vector of PixelBlock objects. These are
 * pointer-sized integers. The LSb of the first PixelBlock represents the 
 * left-most pixel of the top-most row. Each succesive bit and PixelBlock moves
 * to the right. PixelBlocks do not span rows, so unused space will fill the
 * higher value bits of the right-most PixelBlock at the end of each row.
 *
 * @author  Jeff Jackowski
 */
class BppImage : public std::enable_shared_from_this<BppImage> {
public:
	typedef std::uintptr_t  PixelBlock;
private:
	/**
	 * The image data.
	 */
	std::vector<PixelBlock> img;
	/**
	 * The dimensions of the image.
	 */
	ImageDimensions dim;
	/**
	 * Number of @ref PixelBlock "PixelBlocks" used for each horizontal line.
	 * A whole number is always used, save for the case of a zero size image.
	 */
	int blkPerLine;
public:
	/**
	 * Returns the size of an image buffer as the number of
	 * @ref PixelBlock PixelBlocks needed to store an image of the specified
	 * size.
	 */
	static constexpr std::size_t bufferBlockSize(int w, int h) {
		return (w / (sizeof(PixelBlock) * 8) +
		((w % (sizeof(PixelBlock) * 8)) ? 1 : 0)) * h;
	}
	/**
	 * Returns the size of an image buffer in bytes needed for the specified
	 * image size.
	 */
	static constexpr std::size_t bufferByteSize(int w, int h) {
		return bufferBlockSize(w, h) * sizeof(PixelBlock);
	}
	/**
	 * Returns the number of PixelBlock objects that will be used for each
	 * horizontal line of an image of the indicated width. There may be unused
	 * space in the more significant bits of the last PixelBlock in the line.
	 */
	static constexpr int bufferBlocksPerLine(int width) {
		return width / (sizeof(PixelBlock) * 8) +
		((width % (sizeof(PixelBlock) * 8)) ? 1 : 0);
	}
	/**
	 * Returns the number of PixelBlock objects that will be used for each
	 * horizontal line of an image of the indicated width. There may be unused
	 * space in the more significant bits of the last PixelBlock in the line.
	 */
	static constexpr int bufferBlocksPerLine(ImageDimensions dim) {
		return dim.w / (sizeof(PixelBlock) * 8) +
		((dim.w % (sizeof(PixelBlock) * 8)) ? 1 : 0);
	}
	/**
	 * Controls the direction ConstPixel and Pixel objects will move across
	 * the image when the object is incremented. The direction will be the
	 * reverse when decremented. All options are HorizInc rotated by a multiple
	 * of 90 degrees, and are given in clockwise order.
	 * @note  If the listing is in alphabetical order, it is not in
	 *        clockwise order. The values increment in clockwise order.
	 */
	enum Direction {
		/**
		 * The X coordinate will be incremented until reaching the width
		 * limit. When the limit is reached, X will be changed to zero and
		 * Y will be incremented. If Y passes the height limit, the position
		 * will be changed to (-1,-1), the end condition.
		 */
		HorizInc,
		/**
		 * The Y coordinate will be incremented. When the maximum height is
		 * reached, Y will be set to zero and X decremented. If X would
		 * go negative, the position will instead be changed to (-1,-1).
		 */
		VertInc,
		/**
		 * The X coordinate will be decremented until reaching zero. When
		 * zero is reached, X will be changed to the maximum width and
		 * Y will be decremented. If Y would go negative, the position
		 * will instead be changed to (-1,-1).
		 */
		HorizDec,
		/**
		 * The Y coordinate will be decremented. When zero is reached, Y
		 * will be set to the maximum height and X incremented. If X passes
		 * the width limit, the position will instead be changed to (-1,-1).
		 */
		VertDec
	};

	/**
	 * Can be used as an end iterator to avoid making a whole iterator.
	 * ConstPixel used to contain a std::shared_ptr<BppImage>, which made this
	 * more useful.
	 */
	struct EndPixel { };
	class Pixel;
	/**
	 * A forward iterator that visits each location of the image.
	 */
	class ConstPixel {
	public:
		struct End { };
	protected:
		/**
		 * The image to operate upon.
		 * This is a non-const pointer because this class serves as the base
		 * class for Pixel. No functions in this class will ever modify @a src.
		 */
		BppImage *src;
		// allow for quick referencing of data; recalc of img array index and
		// bit position is not always required
		/**
		 * The PixelBlock containing the referenced pixel.
		 */
		PixelBlock *blk;
		/**
		 * The mask used to isolate the referenced pixel.
		 */
		PixelBlock mask;
		/**
		 * The location of the referenced pixel on the source image.
		 */
		ImageLocation pos;
		/**
		 * The direction to move when incremented.
		 */
		Direction dir;
	public:
		/**
		 * Construct a ConstPixel to nowhere.
		 */
		ConstPixel() : blk(nullptr), pos(-1, -1) { }
		/**
		 * Construct a ConstPixel to nowhere.
		 */
		ConstPixel(const EndPixel) : ConstPixel() { }
		/**
		 * Obvious copy constructor.
		 */
		ConstPixel(const ConstPixel &) = default;
		/**
		 * Constructs a ConstPixel to reference the same location of the same
		 * image as the given Pixel object.
		 */
		ConstPixel(const Pixel &p);
		/**
		 * Construct a ConstPixel to be the end iterator of the given image.
		 */
		ConstPixel(const BppImage *img, const End e);
		/**
		 * Construct a ConstPixel to reference the requested location of the
		 * image.
		 * @throw ImageBoundsError
		 */
		ConstPixel(
			const BppImage *img,
			const ImageLocation &il,
			Direction d = HorizInc
		);
		/**
		 * Construct a ConstPixel to reference the requested location of the
		 * image.
		 * @throw ImageBoundsError
		 */
		ConstPixel(
			const BppImage *img,
			int x = 0,
			int y = 0,
			Direction d = HorizInc
		) : ConstPixel(img, ImageLocation(x, y), d) { }
		/**
		 * Obvious assignment operator.
		 */
		ConstPixel &operator = (const ConstPixel &) = default;
		/**
		 * Assigns a ConstPixel to reference the same location of the same
		 * image as the given Pixel object.
		 */
		ConstPixel &operator = (const Pixel &p);
		/**
		 * Assigns a ConstPixel to nowhere.
		 */
		ConstPixel &operator = (const EndPixel) {
			pos.x = pos.y = -1;
			blk = nullptr;
			return *this;
		}
		/**
		 * Returns the direction used for incrementing.
		 */
		Direction direction() const {
			return dir;
		}
		/**
		 * Changes the direction used for incrementing.
		 */
		void direction(Direction d) {
			dir = d;
		}
		/**
		 * Returns the state of the referenced pixel.
		 * @throw ImageIteratorEndError
		 */
		bool state() const;
		/* *
		 * Works like adding a number to increment the ConstPixel and calling
		 * state() on the result, but without making a new ConstPixel object.
		 * @todo  This requires an implementation similar to operator+(int).
		 */
		//bool stateAtOffset(int inc) const;
		/**
		 * Obvious equality operator.
		 */
		bool operator == (const ConstPixel &cp) const {
			return (src == cp.src) && (pos == cp.pos);
		}
		/**
		 * True if this object is an end iterator or an iterator to nowhere.
		 */
		bool operator == (const EndPixel) const {
			return (pos.x == -1) && (pos.y == -1);
		}
		/**
		 * Obvious inequality operator.
		 */
		bool operator != (const ConstPixel &cp) const {
			return (src != cp.src) || (pos != cp.pos);
		}
		/**
		 * False if this object is an end iterator or an iterator to nowhere.
		 */
		bool operator != (const EndPixel) const {
			return (pos.x != -1) || (pos.y != -1);
		}
		/**
		 * Pre-increment operator.
		 */
		ConstPixel &operator ++();
		/**
		 * Post-increment operator. This involves the copying the ConstPixel.
		 */
		ConstPixel operator ++(int) {
			ConstPixel cp(*this);
			operator++();
			return cp;
		}
		//operator +(int);
		//operator +=(int);  ?
		/**
		 * Returns the horizontal coordinate of the referenced pixel.
		 */
		int x() const {
			return pos.x;
		}
		/**
		 * Returns the vertical coordinate of the referenced pixel.
		 */
		int y() const {
			return pos.y;
		}
		/**
		 * Returns the coordinates of the referenced pixel.
		 */
		const ImageLocation &location() const {
			return pos;
		}
		/**
		 * Changes the location referenced by this ConstPixel.
		 * @throw  ImageBoundsError
		 */
		void location(const ImageLocation &il);
		/**
		 * Changes the location referenced by this ConstPixel.
		 * @throw  ImageBoundsError
		 */
		void location(int x, int y) {
			location(ImageLocation(x, y));
		}
		/* *
		 * Changes the coordinate perpendicular to the set direction as if by
		 * incrementing this pixel by an amount equal to @a l multiplied by
		 * the width, for HorizInc and HorizDec, or the height, for VertInc
		 * and VertDec. This means that with direction HorizInc starting at
		 * position (x,y), line(l) will change the position to (x,y+l) if y+l
		 * is within the height bound.
		 */
		//void line(int l);

		/**
		 * Retreives a pixel's state; used to allow ConstPixel to be
		 * dereferenced like any other iterator to get a bool for the pixel.
		 */
		class ConstBoolProxy {
			const ConstPixel *cp;
		public:
			ConstBoolProxy(const ConstPixel *p) : cp(p) { }
			/**
			 * Returns the pixel's state.
			 * @throw ImageIteratorEndError
			 */
			operator bool () const {
				return cp->state();
			}
		};
		/**
		 * Dereferences the ConstPixel; provides the state of the pixel.
		 */
		ConstBoolProxy operator*() const {
			return ConstBoolProxy(this);
		}
		
		// incomplete attempt at fulfilling all standard iterator requirements
		//typedef std::forward_iterator_tag iterator_category;
		typedef bool value_type;
		//typedef ? difference_type;
		//typedef ConstBoolProxy pointer; // doesn't seem quite right
		//typedef ? reference;
	};
	/**
	 * A forward and output iterator that visits each location of the image.
	 */
	class Pixel : public ConstPixel {
	public:
		/**
		 * Construct a Pixel to reference the requested location of the
		 * image.
		 * @throw ImageBoundsError
		 */
		Pixel(
			const BppImage *img,
			const ImageLocation &il,
			Direction d = HorizInc
		) : ConstPixel(img, il, d) { }
		/**
		 * Construct a Pixel to reference the requested location of the
		 * image.
		 * @throw ImageBoundsError
		 */
		Pixel(
			const BppImage *img,
			int x = 0,
			int y = 0,
			Direction d = HorizInc
		) : ConstPixel(img, ImageLocation(x, y), d) { }
		/**
		 * Obvious assignment operator.
		 */
		Pixel &operator = (const Pixel &p) = default;
		/**
		 * Assigns a Pixel to nowhere.
		 */
		Pixel &operator = (const EndPixel) {
			pos.x = pos.y = -1;
			blk = nullptr;
			return *this;
		}
		/**
		 * Sets the state of the pixel.
		 */
		void state(bool s);
		/**
		 * Returns the state of the referenced pixel.
		 * @throw ImageIteratorEndError
		 */
		bool state() const {
			return ConstPixel::state();
		}
		/**
		 * Changes the state of the referenced pixel.
		 * @throw   ImageIteratorEndError
		 * @return  The new state of the pixel.
		 */
		Pixel &operator = (bool s) {
			state(s);
			return *this;
		}
		/**
		 * Clears (make false) the referenced pixel.
		 * @throw ImageIteratorEndError
		 */
		void clear() {
			state(false);
		}
		/**
		 * Sets (make true) the referenced pixel.
		 * @throw ImageIteratorEndError
		 */
		void set() {
			state(true);
		}
		/**
		 * Toggles the state of the pixel. This works like state(!state()),
		 * but is more efficient.
		 * @return  The new state of the pixel.
		 * @throw   ImageIteratorEndError
		 */
		bool toggle();

		/**
		 * Retreives a pixel's state; used to allow Pixel to be
		 * dereferenced like any other iterator to get a bool for the pixel.
		 */
		class BoolProxy {
			Pixel *pix;
		public:
			BoolProxy(Pixel *p) : pix(p) { }
			/**
			 * Returns the pixel's state.
			 * @throw ImageIteratorEndError
			 */
			operator bool () {
				return pix->state();
			}
			/**
			 * Changes the pixel's state.
			 * @throw ImageIteratorEndError
			 */
			bool operator = (bool b) {
				pix->state(b);
				return b;
			}
		};
		/**
		 * Dereferences the ConstPixel; provides the state of the pixel.
		 */
		BoolProxy operator*() {
			return BoolProxy(this);
		}
	};
	/**
	 * Make an empty image with zero size.
	 */
	BppImage() : dim(0, 0), blkPerLine(0) { };
	/**
	 * Makes an image of the requested size with uninitialized image data.
	 */
	BppImage(const ImageDimensions &id);
	/**
	 * Makes an image of the requested size with uninitialized image data.
	 */
	BppImage(int width, int height) : BppImage(ImageDimensions(width, height)) { }
	/**
	 * Move constructor.
	 */
	BppImage(BppImage &&mv);
	/**
	 * Copy constructor.
	 */
	BppImage(const BppImage &src);
	/**
	 * Copies constant image data into a new image.
	 * @param data  The source data. It starts with the width and height, in
	 *              that order. Both are two bytes in little endian form.
	 *              Following that is the image data. The LSb of each byte is
	 *              for the pixel furthest left in the byte. If the width is
	 *              not evenly divisible by 8, the last byte of each line will
	 *              contain unused bits. Each line of the image will start on
	 *              a new byte.
	 */
	BppImage(const char *data);
	/**
	 * Copies constant image data into a new image with run-time bounds checks
	 * on accesses to @a data.
	 * @param data  The source data. It starts with the width and height, in
	 *              that order. Both are two bytes in little endian form.
	 *              Following that is the image data. The LSb of each byte is
	 *              for the pixel furthest left in the byte. If the width is
	 *              not evenly divisible by 8, the last byte of each line will
	 *              contain unused bits. Each line of the image will start on
	 *              a new byte.
	 * @throw ImageTooSmallError  The provided data was too small. Either there
	 *                            isn't enough data for the smallest possible
	 *                            image, or the indicated dimensions require
	 *                            more data.
	 */
	BppImage(const std::vector<char> &data);
	/**
	 * Convenience function to make a shared pointer to an image using the
	 * BppImage(const char *) constructor.
	 */
	static std::shared_ptr<BppImage> make(const char *data);
	/**
	 * Convenience function to make a shared pointer to an image using the
	 * BppImage(const char *) constructor.
	 */
	static std::shared_ptr<BppImage> make(const std::vector<char> &data);
	/**
	 * Move assignment.
	 */
	BppImage operator = (BppImage &&mv);
	/**
	 * Copy assignment.
	 */
	BppImage operator = (const BppImage &src);
	/**
	 * Swap two images.
	 */
	void swap(BppImage &other);
	/**
	 * Removes all image data.
	 * @post  The image has zero size. All ConstPixel and Pixel objects using
	 *        this object are invalid.
	 */
	void clear();
	/**
	 * Changes the size of the image.
	 * @post  The image data is invalid. Should change this.
	 */
	void resize(int width, int height);
	/**
	 * Returns true if there is no image data.
	 */
	bool empty() const {
		return img.empty();
	}
	/**
	 * Returns the number of @ref PixelBlock "PixelBlocks", not bytes, that
	 * make up the image buffer. This result includes space allocated but not
	 * used for the image data.
	 */
	std::size_t bufferSize() const {
		return img.size();
	}
	/**
	 * Returns the number of pixels that make up the image.
	 */
	std::size_t size() const {
		return dim.w * dim.h;
	}
	/**
	 * Returns the width of the image.
	 */
	int width() const {
		return dim.w;
	}
	/**
	 * Returns the height of the image.
	 */
	int height() const {
		return dim.h;
	}
	/**
	 * Returns the dimensions of the image.
	 */
	const ImageDimensions &dimensions() const {
		return dim;
	}
	/**
	 * Retuns a pointer to the start of image data.
	 * @throw ImageZeroSizeError
	 */
	const PixelBlock *buffer() const;
	/**
	 * Retuns a pointer to the start of image data.
	 * @throw ImageZeroSizeError
	 */
	PixelBlock *buffer();

	// C++ containers are going with data() for underlying buffer
	const std::vector<PixelBlock> &data() const {
		return img;
	}

	// improve error handling; move to cpp
	const PixelBlock *bufferLine(int py) const {
		return &(img[blkPerLine * py]);
	}
	PixelBlock *bufferLine(int py) {
		return &(img[blkPerLine * py]);
	}
	/**
	 * Returns the number of PixelBlock objects per row in the image data.
	 */
	int blocksPerLine() const {
		return blkPerLine;
	}
	/**
	 * Provides the location of the specified pixel inside the image data.
	 * A class for the result of bufferSpot() is not provided because it is
	 * more useful when combined with a coordinate, and the ConstPixel and
	 * Pixel classes do that.
	 * @param addr  The address of the PixelBlock that contains the requested
	 *              pixel.
	 * @param mask  A bitmask that will be set to identify the pixel inside
	 *              the PixelBlock.
	 * @param il    The location to find within the image data.
	 */
	void bufferSpot(
		PixelBlock *(&addr),
		PixelBlock &mask,
		const ImageLocation &il
	);
	/**
	 * Provides the location of the specified pixel inside the image data.
	 * A class for the result of bufferSpot() is not provided because it is
	 * more useful when combined with a coordinate, and the ConstPixel and
	 * Pixel classes do that.
	 * @param addr  The address of the PixelBlock that contains the requested
	 *              pixel.
	 * @param mask  A bitmask that will be set to identify the pixel inside
	 *              the PixelBlock.
	 * @param x     The X coordinate.
	 * @param y     The Y coordinate.
	 */
	void bufferSpot(PixelBlock *(&addr), PixelBlock &mask, int x, int y) {
		bufferSpot(addr, mask, ImageLocation(x, y));
	}
	/**
	 * Provides the location of the specified pixel inside the image data.
	 * A class for the result of bufferSpot() is not provided because it is
	 * more useful when combined with a coordinate, and the ConstPixel and
	 * Pixel classes do that.
	 * @param addr  The address of the PixelBlock that contains the requested
	 *              pixel.
	 * @param mask  A bitmask that will be set to identify the pixel inside
	 *              the PixelBlock.
	 * @param il    The location to find within the image data.
	 */
	void bufferSpot(
		const PixelBlock *(&addr),
		PixelBlock &mask,
		const ImageLocation &il
	) const {
		// the implementation is the same, save for a couple of const
		const_cast<BppImage*>(this)->bufferSpot(
			const_cast<PixelBlock*&>(addr),
			mask,
			il
		);
	}

	Pixel pixel(const ImageLocation &il, Direction dir = HorizInc);
	Pixel pixel(int x, int y, Direction dir = HorizInc) {
		return pixel(ImageLocation(x, y), dir);
	}
	ConstPixel cpixel(const ImageLocation &il, Direction dir = HorizInc) const;
	ConstPixel cpixel(int x, int y, Direction dir = HorizInc) const {
		return cpixel(ImageLocation(x, y), dir);
	}
	ConstPixel operator () (int x, int y) const {
		return cpixel(x, y);
	}
	/**
	 * Returns a Pixel (iterator) to the upper left of the image.
	 */
	Pixel begin();
	/**
	 * Returns a Pixel (iterator) to the start of the image for the given
	 * direction. Different directions start in a different location, but will
	 * iterate over the whole image.
	 */
	Pixel begin(Direction dir);
	/**
	 * Returns a ConstPixel (iterator) to the upper left of the image.
	 */
	ConstPixel cbegin() const;
	/**
	 * Returns a ConstPixel (iterator) to the start of the image for the given
	 * direction. Different directions start in a different location, but will
	 * iterate over the whole image.
	 */
	ConstPixel cbegin(Direction dir) const;
	/**
	 * Returns a ConstPixel (iterator) to the upper left of the image.
	 */
	ConstPixel begin() const {
		return cbegin();
	}
	/**
	 * Convenience function that returns EndPixel, which can be used as an end
	 * iterator.
	 */
	static EndPixel endPixel() {
		return EndPixel();
	}
	/**
	 * Returns a Pixel end iterator.
	 */
	Pixel end();
	/**
	 * Returns a ConstPixel end iterator.
	 */
	ConstPixel cend() const {
		return ConstPixel(this, -1, -1);
	}
	/**
	 * Returns a ConstPixel end iterator.
	 */
	ConstPixel end() const {
		return cend();
	}
	bool state(const ImageLocation &il) const;
	bool state(int x, int y) const {
		return state(ImageLocation(x, y));
	}
	void state(const ImageLocation &il, bool state);
	void state(int x, int y, bool s) {
		state(ImageLocation(x, y), s);
	}
	void clearPixel(const ImageLocation &il) {
		state(il, false);
	}
	void clearPixel(int x, int y) {
		state(x, y, false);
	}
	void setPixel(const ImageLocation &il) {
		state(il, true);
	}
	void setPixel(int x, int y) {
		state(x, y, true);
	}
	bool togglePixel(const ImageLocation &il);
	bool togglePixel(int x, int y) {
		return togglePixel(ImageLocation(x, y));
	}
	/**
	 * Blanks the image using the given state.
	 * @post  All pixels will be set to @a state.
	 */
	void blankImage(bool state = false);
	// add ability to specifiy operation: set, or, and, xor
	//void write(const std::shared_ptr<BppImage> &img, int dx, int dy, int sx, int sy, int w, int h);
};

inline void swap(BppImage &bi0, BppImage &bi1) {
	bi0.swap(bi1);
}

} } } }

#endif        //  #ifndef BPPIMAGE_HPP
