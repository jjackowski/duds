# DUDS
C++17 application level style access to low level hardware.

The real documentation is generated by Doxygen and Graphviz. Doxygen output, but not Graphviz (missing many useful graphs), is updated after every commit and available at [Codedocs.xyz](https://codedocs.xyz/jjackowski/duds/).

What follows is a copy of a small part of that documentation.

# Distributed Update of Data from Something

A silly name, because I'm not good at making up names, that attempts to describe the ultimate goal for this family of libraries. At present, DUDS only has a single library with functionality for time and interfacing with hardware through the Linux kernel. It aspires to be a set of libraries that include functionality for representing measurements taken from hardware, sending that data over a network, and doing so in a multi-platform environment. At the moment, it is more about using low-level hardware with a high-level interface.

# Goals

- Be a well designed C++ happy library that is multi-platform where applicable.
- Take advantage of C++17.
- Support multi-threaded programs.
- Support dynamic resource usage.
- Favor correctness over performance.
- Favor flexibility and capability over ease of use.
- Consist of namespaces with well defined purposes that are as independent as sensible.
- Offer robust error reporting and handling.
- Have unambiguous time stamps.
- Track accuracy, precision, resolution, and origin of sample data.
- Have good reference documentation.
- Allow for internationalization support.

# Audience

The audience, users of the library, include:
- Intermediate and experienced C++ developers
- Developers who want to mix high-level application code with use of low-level gizmos

The audience is not intended to include:
- Beginning developers
- Developers unfamiliar with C++
- Developers making high performance applications
- Developers who avoid error handling

While it is not intentional to exclude people from using this library, the library isn't intended to be a perfect fit for everyone or every use. For instance, it isn't made with the goal of being super easy to use as a top priority, and the documentation assumes a familiarity with C++.

# Supported Platforms

Right now, only Linux. Hardware access is only tested on Raspberry Pis, but the library does build on AMD64, and the test program works fine on both. The hardware access code is not specific to the Raspberry Pi. I've been using Gentoo on the Raspberry Pis and have not tested Raspbian.

# Supported devices

- Displays
	- HD44780 (SPLC780D)
	- ST7920
- Instruments
	- Accelerometers
		- FXOS8700CQ
		- LSM9DS1
	- Brightness
		- APDS9301
		- ISL29125
		- TSL2591
	- Current
		- INA219
	- Gyroscopes
		- LSM9DS1
	- Humidity
		- AM2320
	- Magetometers
		- FXOS8700CQ
		- LSM9DS1
	- Power
		- INA219
	- Temperature sensors and thermal cameras
		- AM2320
		- AMG88xx
		- MCP9808
	- Voltage
		- INA219

# Build

- [SCons](http://scons.org/) 2.3 to 3.1 or so (the build isn't very picky about the version)
  - Python 2.7 or 3.x
- Build configuration options: run "scons -h"
  - The configuration can be changed for one build by specifying an option as an argument to scons.
  - To keep a configuration saved, set variables with the same name of the configuration options in Python syntax in localbuildconfig.py in the same directory as SConstruct.
- gcc 9.2.x, 7.3.x
  - Other versions are untested. Older ones will likely fail.
  - Any output for ARM processors from gcc 7.1 and newer will not link correctly with any output from earlier versions of gcc. There are *huge* warnings about this with gcc 6.4.0 and 7.3.0.
- [Boost](http://www.boost.org/) version 1.62 or newer
  - Boost libraries used:
    - Date_time
    - Exception (header only)
    - Filesystem (optional)
    - Multiprecision (header only)
    - Program options (sample programs only)
    - Property tree (header only)
    - Serialization
    - Signals2 (header only)
    - Stacktrace (optional; header + dl library; Boost 1.65.1 or newer)
    - System
    - Unit test framework (optional; used for test programs, not DUDS library)
    - Variant (header only)
  - Build configuration options include Boost directories and variations on the names of the Boost library binaries.
  - Default configuration is for a regular Boost build that is installed to default paths. This works on Gentoo Linux when Boost is installed by Portage.
  - Raspberry Pi OS, Raspbian, Debian, and Ubuntu need build configuration setting "BOOSTTAG=" (equals empty string) given to SCons. This changes the expected name of the Boost libraries; these Linux distributions do not use the default names from the Boost build.
  - Windows will need several build options because there is no standard install that I know about. DUDS isn't useful on Windows at the moment, so don't worry about it.
- Build host
  - Linux
    - Raspberry Pi OS, Raspbian, Debian, and Ubuntu need build configuration setting "BOOSTTAG=" (equals empty string) given to SCons. This changes the expected name of the Boost libraries; these Linux distributions do not use the default names from the Boost build.
    - Gentoo works with the default configuration on a normal Gentoo install.
    - Parallel builds work best with 1GB RAM per job and some swap space.
      - Using "-j4" on a Raspberry Pi 2 or 3 isn't helpful. Using "-j2" needs swap space, but may be faster than "-j1", the default.
    - Builds tested mostly on armv6 and amd64.
    - Time for a complete debug build:
      - Raspberry Pi Zero: about two and a half hours
      - Raspberry Pi 3 Model B: getting close to half an hour with -j1 (default).

# Build targets

The default build target is the DUDS library made for debugging. Sample programs are built with the "samples" target, and the tests program (one program right now) with the "tests" target. All those targets make debug builds. There are also targets with names that end in either "-dbg", or "-opt", for debug or optimized builds respectively. Build from the root directory of DUDS by running scons. The binaries will be placed in a directory under bin named for the target and build. For example:
- duds
  - bin
    - linux-armv6l-dbg
      - samples (built with target "samples" or "samples-dbg")
    - linux-armv6l-opt
      - samples (built with target "samples-opt")
    - linux-x86_64-dbg

There is currently no target for installation. Despite using Linux since 2001, I'm not really sure how software is typically installed.

# License

The license for almost all the code is the two-clause simplified BSD license. The exceptions are duds/general/SignExtend.hpp and duds/general/ReverseBits.hpp, which are in the public domain.

All fonts, save for the 4x6 font, are licensed under GPL 2.0. The 4x6 font is public domain.
