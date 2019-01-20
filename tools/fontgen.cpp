/**
 * @file
 * Generates Bit-Per-Pixel Image text definition files from fonts avaialble
 * in the Linux kernel. Takes advantage of the C language by including the
 * kernel source file, so this source file must be modified and built for the
 * specific font. It is easiest to build if the kernel source file is modified
 * to not include linux/font.h, and not define a font_desc object.
 */
// modify this block for the font to convert
// mini_4x6 is public domain
#include "font_mini_4x6.c"
#define FONTDATA  fontdata_mini_4x6
constexpr char fname[] = "font_4x6.bppi";
constexpr int start  =  32;
constexpr int stop   = 255;
constexpr int width  =   4;
constexpr int height =   6;

// 6x10 font is GPL 2.0
/*
#include "font_6x10.c"
#define FONTDATA  fontdata_6x10
constexpr char fname[] = "font_6x10.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =   6;
constexpr int height =  10;
*/

// 6x11 font is GPL 2.0
/*
#include "font_6x11.c"
#define FONTDATA  fontdata_6x11
constexpr char fname[] = "font_6x11.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =   6;
constexpr int height =  11;
*/

// 7x14 font is GPL 2.0
/*
#include "font_7x14.c"
#define FONTDATA  fontdata_7x14
constexpr char fname[] = "font_7x14.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =   7;
constexpr int height =  14;
*/

// 8x8 font is GPL 2.0
/*
#include "font_8x8.c"
#define FONTDATA  fontdata_8x8
constexpr char fname[] = "font_8x8.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =   8;
constexpr int height =   8;
*/

// 8x16 font is GPL 2.0
/*
#include "font_8x16.c"
#define FONTDATA  fontdata_8x16
constexpr char fname[] = "font_8x16.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =   8;
constexpr int height =  16;
*/

// 10x18 font is GPL 2.0
/*
#include "font_10x18.c"
#define FONTDATA  fontdata_10x18
constexpr char fname[] = "font_10x18.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =  10;
constexpr int height =  18;
*/

// Sun 12x22 is GPL 2.0
/*
#include "font_sun12x22.c"
#define FONTDATA  fontdata_sun12x22
constexpr char fname[] = "font_12x22.bppi";
constexpr int start  =   1;
constexpr int stop   = 255;
constexpr int width  =  12;
constexpr int height =  22;
*/

// shouldn't need to modify the rest for a font unless a bug is found
#include <fstream>

int main(int argc, char *argv[]) {
	std::ofstream of(fname);
	const unsigned char *font =
		FONTDATA + start * height *
		((width >> 3) + ((width & 7) ? 1 : 0)); // bytes per character
	for (int glyph = start; glyph < stop; ++glyph) {
		of << "/* Character " << glyph;
		if (glyph >= 32) {
			of << ", glyph " << (char)glyph;
		}
		of << " */\n" << '\\' << glyph << ' ' << width << ' ' << height << "\n\t";
		for (int pos = 0; pos < width; ++pos) {
			of << (pos % 10);
		}
		of << " {\n";
		for (int line = 0; line < height; ++line) {
			of << (line % 10) << '\t';
			int bit = 0x80;
			for (int pos = 0; pos < width; ++pos) {
				if (*font & bit) {
					of << 'X';
				} else {
					of << ' ';
				}
				bit >>= 1;
				if (!bit) {
					bit = 0x80;
					++font;
				}
			}
			if (bit != 0x80) {
				++font;
			}
			// mark end of character
			of << " // " << line << '\n';
		}
		of << "\t}\n\n";
	}
}
