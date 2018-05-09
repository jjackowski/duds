/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
#include <fstream>
#include <iostream>
#include <iomanip>
#include <list>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <duds/general/NddArray.hpp>

using duds::general::NddArray;

struct ParsingError : virtual std::exception, virtual boost::exception { };
/**
 * An image name is not a legal C++ identifier.
 */
struct BadIdentifierError : ParsingError  { };
/**
 * The image dimensions are beyond the allowable range, or could not be parsed.
 */
struct BadDimensionsError : ParsingError  { };
/**
 * The specified image dimensions do not match the dimensions of the image data.
 */
struct DimensionMismatchError : ParsingError  { };
/**
 * An image definition is incomplete.
 */
struct IncompleteImageError : ParsingError  { };
/**
 * A comment starts but does not end.
 */
struct UnendingCommentError : ParsingError  { };
/**
 * Provides the line number of the error.
 */
typedef boost::error_info<struct Info_LineNumber, int> LineNumber;
/**
 * Provides the name of the image definition with an error.
 */
typedef boost::error_info<struct Info_ImageName, std::string> ImageName;

class Parser {
	/**
	 * A list of image data parsed from the input file. It is not a map so that
	 * the ordering of the images in the source file can be preserved in the
	 * output file.
	 */
	std::list< std::pair< std::string, NddArray<char> > >  images;
	/**
	 * Line of the source file being parsed; used for error messages.
	 */
	int line = 1;
	/**
	 * Reads from the stream and extracts any character that is whitespace or
	 * that is part of a comment. When it returns, @a is should either be
	 * positioned to read the next non-whitespace character, or is !good(),
	 * likely at EOF.
	 */
	void seekPastSpaceComments(std::istream &is) {
		int ex;
		do {
			ex = is.peek();
			// comment?
			if (ex == '/') {
				// check for comment to keep for output
				is.get();
				ex = is.peek();
				if (ex == '*') {
					// start recording the comment
					std::vector<char> cmt;
					cmt.reserve(1024);
					cmt.push_back('/');
					cmt.push_back('*');
					is.get();  // extract '*'
					// read to the end of the comment
					while (is.good() && ((ex != '*') || (is.peek() != '/'))) {
						cmt.push_back(ex = is.get());
						if (ex == '\n') {
							++line;
						}
					}
					// assure comment ended
					if ((ex == '*') && (is.peek() == '/')) {
						// the last slash hasn't been extracted and pushed
						cmt.push_back(is.get());
						// improve formatting
						cmt.push_back('\n');
						// store it so it can be added to generated output
						NddArray<char> carray;
						carray.copyFrom(cmt);
						images.emplace_back("", std::move(carray));
					} else {
						BOOST_THROW_EXCEPTION(UnendingCommentError() <<
							LineNumber(line)
						);
					}
				} else {
					if (ex != '\n') {
						// skip to next line
						is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					}
					++line;
				}
			}
			// not whitespace or comma?
			else if ((ex != ' ') && (ex != '\t') && (ex != '\n') && (ex != ',')) {
				// leave it for the caller
				return;
			}
			// is EOL?
			else if (ex == '\n') {
				// count it
				++line;
				// extract it
				is.get();
			} else {
				// extract and continue
				is.get();
			}
		} while (is.good());
	}
	/**
	 * Reads in a row of image data.
	 * @return  True if the end bracket that terminates the image data has been
	 *          reached.
	 */
	bool parseImageLine(std::istream &is, NddArray<char> &imgtxt, int w, int y) {
		int c;
		for (int x = 0; x < w; ++x) {
			// may start with newline, numbers, tabs
			do {
				c = is.get();
				if (!is.good()) {
					BOOST_THROW_EXCEPTION(IncompleteImageError() <<
						LineNumber(line)
					);
				}
				// end of line is end of this row, unless it hasn't started
				else if (c == '\n') {
					++line;
					if (x) {
						return false;
					}
				}
				// comment is end of this row, unless it hasn't started
				else if (c == '/') {
					++line;
					is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					if (x) {
						return false;
					}
				}
				// end bracket is end of image
				else if (c == '}') {
					return true;
				}
			} while (
				// # is ok for pixel
				(c != '#') &&
				// space is ok for pixel
				(c != ' ') &&
				// letters are ok for pixel
				!((c >= 'A') && (c <= 'Z')) && !((c >= 'a') && (c <= 'z'))
			);
			// found an image pixel
			if (c != ' ') {
				// image starts with all zeros; this spot is a one
				imgtxt({(unsigned)x,(unsigned)y}) = (char)c;
			}
		}
		// spaces are permitted after the end of the image data; useful
		// for adding comments to the right of the image
		do {
			c = is.get();
			if (!is.good()) {
				// EOF may occur; should find end bracket first
				return false;
			}
			// end of line is end of this row
			else if (c == '\n') {
				++line;
				return false;
			}
			// comment is end of this row
			else if (c == '/') {
				++line;
				is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				return false;
			}
			// end bracket is end of image
			else if (c == '}') {
				return true;
			}
		} while ((c == ' ') || (c == '\t'));
		// if running here, there is junk at the end of the line
		BOOST_THROW_EXCEPTION(DimensionMismatchError() <<
			LineNumber(line)
		);
	}
	/**
	 * Parses a dimension size for an image.
	 */
	int parseDim(std::istream &is) {
		seekPastSpaceComments(is);
		if (!is.good()) {
			BOOST_THROW_EXCEPTION(IncompleteImageError() <<
				LineNumber(line)
			);
		}
		int dim;
		is >> dim;
		if ((dim <= 0) || (dim > 0x7FFF)) {
			BOOST_THROW_EXCEPTION(BadDimensionsError() <<
				LineNumber(line)
			);
		}
		return dim;
	}
	/**
	 * Parses one image definition.
	 */
	void parseImage(std::istream &is) {
		// read in the name
		seekPastSpaceComments(is);
		if (!is.good()) {
			// got to the end of file before an image
			return; // perfectly fine; no error
		}
		std::string name;
		is >> name;
		// assure a good name
		if (!((name.front() >= 'A') && (name.front() <= 'Z')) &&
			!((name.front() >= 'a') && (name.front() <= 'z'))
		) {
			BOOST_THROW_EXCEPTION(BadIdentifierError() <<
				LineNumber(line) << ImageName(name)
			);
		}
		for (char c : name) {
			if (!((c >= 'A') && (c <= 'Z')) &&
				!((c >= 'a') && (c <= 'z')) &&
				!((c >= '0') && (c <= '9')) &&
				(c != '_')
			) {
				BOOST_THROW_EXCEPTION(BadIdentifierError() <<
					LineNumber(line) << ImageName(name)
				);
			}
		}
		// read in dimensions
		int width;
		int height;
		try {
			width = parseDim(is);
			height = parseDim(is);
		} catch (ParsingError &pe) {
			pe << ImageName(name);
			throw pe;
		}
		
		//std::cout << "Parsing " << name << " (" << width << ',' << height <<
		//')' << std::endl;
		
		// find the start if the image data
		int c;
		do {
			seekPastSpaceComments(is);
			if (!is.good()) {
				BOOST_THROW_EXCEPTION(IncompleteImageError() <<
					LineNumber(line) << ImageName(name)
				);
			}
			c = is.get();
			if (c == '\n') {
				++line;
			}
		} while (c != '{');
		// make the place for the image data
		NddArray<char> imgtxt({(unsigned)width, (unsigned)height});
		for (char &c : imgtxt) {
			c = 0;
		}
		// loop through all rows
		int row = 0;
		bool done = false;
		do {
			if (!is.good()) {
				BOOST_THROW_EXCEPTION(IncompleteImageError() <<
					LineNumber(line) << ImageName(name)
				);
			}
			// read in the next line
			try {
				done = parseImageLine(is, imgtxt, width, row++);
			} catch (duds::general::NddArrayError &nae) {
				nae << LineNumber(line) << ImageName(name);
				throw nae;
			} catch (ParsingError &pe) {
				pe << ImageName(name);
				throw pe;
			}
		} while (!done);
		// store image
		images.emplace_back(name, std::move(imgtxt));
	}
	static std::vector<unsigned char> makeData(const NddArray<char> &src) {
		std::vector<unsigned char> dest;
		// get the image dimensions
		std::size_t width = src.dim(0);
		std::size_t height = src.dim(1);
		// compute the image data size; account for padding ar end of rows
		std::size_t imgwidth = width / 8 + ((width % 8) ? 1 : 0);
		std::size_t length = imgwidth * height + 4;
		dest.reserve(length);
		// store the dimenstions
		dest.push_back(width & 0xFF);
		dest.push_back((width >> 8) & 0xFF);
		dest.push_back(height & 0xFF);
		dest.push_back((height >> 8) & 0xFF);
		// store the image data
		NddArray<char>::const_iterator siter = src.begin();
		for (int y = 0; y < height; ++y) {
			int mask = 1;
			unsigned char pix = 0;
			for (int x = 0; x < width; ++x, mask <<= 1, ++siter) {
				// next byte?
				if (mask > 0x80) {
					//std::cout << "** pushed full byte" << std::endl;
					mask = 1;
					dest.push_back(pix);
					pix = 0;
				}
				// set pixel?
				if (*siter) {
					pix |= mask;
				}
			}
			// store the last produced byte of the row
			dest.push_back(pix);
			//std::cout << "** pushed with mask = " << mask << std::endl;
		}
		// final size should match
		assert(dest.size() == length);
		return dest;
	}
	void writeImage(
		std::ostream &out,
		const std::pair< std::string, NddArray<char> > &p
	) const {
		//std::cout << "Outputing " << p.first << std::endl;
		std::vector<unsigned char> dest = makeData(p.second);
		// write out the image data to the file
		out << "const char " << p.first << '[' << dest.size() <<
		"] = {  // " << p.second.dim(0) << 'x' << p.second.dim(1) <<
		" BPP image\n\t" << std::hex;
		// column counter; 12 columns of bytes per line
		int col = 12;
		// used to coordinate ending the sequence
		std::size_t remain = dest.size();
		for (unsigned char byte : dest) {
			// output byte in hex
			out << "0x" << std::setw(2) << (int)byte;
			// more to go?
			if (--remain) {
				out << ',';
				// last column?
				if (!--col) {
					out << "\n\t";
					col = 12;
				} else {
					out << ' ';
				}
			}
		}
		// terminate sequence
		out << "\n};\n" << std::dec << std::endl;
	}
	void writeComment(std::ostream &out, const NddArray<char> &c) const {
		out.write(c.begin(), c.dim(0));
	}
public:
	Parser() = default;
	//Parser(std::string path) : in(path) { }
	void parse(std::istream &is) {
		// do not skip whitespace
		is.unsetf(std::ios::skipws);
		do {
			// parse loop
			parseImage(is);
		} while (is.good());
	}
	void writeCpp(std::ostream &out) const {
		out.fill('0');
		for (auto const &p : images) {
			if (p.first.empty()) {
				writeComment(out, p.second);
			} else {
				writeImage(out, p);
			}
		}
	}
	void writeLoadable(std::ostream &out) const {
		for (auto const &p : images) {
			// skip comments
			if (p.first.empty()) {
				continue;
			}
			//std::cout << "Outputing " << p.first << std::endl;
			std::vector<unsigned char> dest = makeData(p.second);
			// write out the image data to the file
			out << p.first << ' ';
			out.write((char*)&(dest[0]), dest.size());
		}
	}
};

int main(int argc, char *argv[])
try {
	std::string srcpath, cpppath, arcpath;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for BPP image compiler"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			(
				"input,i",
				boost::program_options::value<std::string>(&srcpath),
				"Source file"
			)
			(
				"cpp,c",
				boost::program_options::value<std::string>(&cpppath),
				"C++ output file"
			)
			(
				"arc,a",
				boost::program_options::value<std::string>(&arcpath),
				"BPP binary archive output file"
			)
		;
		boost::program_options::positional_options_description pod;
		pod.add("input", -1);
		boost::program_options::variables_map vm;
		boost::program_options::store(
			//boost::program_options::parse_command_line(argc, argv, optdesc),
			boost::program_options::command_line_parser(argc, argv).options(optdesc).positional(pod).run(),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help") || srcpath.empty()) {
			std::cout << "Bit-Per-Pixel image compiler\n" << argv[0] <<
			" [options]\n" << optdesc << std::endl;
			return 0;
		}
	}
	std::ifstream in;
	in.open(srcpath);
	if (!in.good()) {
		std::cerr << "ERROR: Could not open input file " << srcpath << '.'
		<< std::endl;
		return 1;
	}
	Parser p;
	try {
		p.parse(in);
	} catch (...) {
		std::cerr << "ERROR: Failed to parse input file " << srcpath << ".\n" <<
		boost::current_exception_diagnostic_information() << std::endl;
		return 1;
	}
	if (!arcpath.empty()) {
		std::ofstream out(arcpath);
		if (!out.good()) {
			std::cerr << "ERROR: Could not open output file " << arcpath << '.'
			<< std::endl;
			return 1;
		}
		// some simple header
		out << "BPPI";
		// should be little-endian version number, because everything else
		// about the image data is little-endian
		const char ver[4] = { 0, 0, 0, 0 };
		out.write(ver, 4);
		p.writeLoadable(out);
	}
	if (!cpppath.empty()) {
		std::ofstream out(cpppath);
		if (!out.good()) {
			std::cerr << "ERROR: Could not open output file " << cpppath << '.'
			<< std::endl;
			return 1;
		}
		out << "/*\n * Bit-Per-Pixel image data autogenerated by bppc from\n * " <<
		srcpath << "\n */\n\n";
		p.writeCpp(out);
	} else if (arcpath.empty()) {
		// output to stdout if no other output requested
		std::cout <<
		"/*\n * Bit-Per-Pixel image data autogenerated by bppc from\n * " <<
		srcpath << "\n */\n\n";
		p.writeCpp(std::cout);
	}
	return 0;
} catch (...) {
	std::cerr << "Bit-Per-Pixel image compiler failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
