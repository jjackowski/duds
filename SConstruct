# This file is part of the DUDS project. It is subject to the BSD-style
# license terms in the LICENSE file found in the top-level directory of this
# distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
# No part of DUDS, including this file, may be copied, modified, propagated,
# or distributed except according to the terms contained in the LICENSE file.

import os
import platform
import subprocess

# CXX = 'distcc g++' might use distcc correctly

#####
# setup the build options
buildopts = Variables('localbuildconfig.py')
buildopts.Add('CCDBGFLAGS',
	'The flags to use with the compiler for debugging builds.',
	'-g -fno-common -O0')
	#'-g -fno-common -Og')
	# -Og should be good for debugging, but too often it prevents important
	# data structures from being inspected by gdb
buildopts.Add('CCOPTFLAGS',
	'The flags to use with the compiler for optimized non-debugging builds.',
	'-O2 -ffunction-sections -fno-common -ffast-math')
buildopts.Add('LINKDBGFLAGS',
	'The flags to use with the linker for debugging builds.',
	'')
buildopts.Add('LINKOPTFLAGS',
	'The flags to use with the linker for optimized builds.',
	'-Wl,--gc-sections')
buildopts.Add(PathVariable('BOOSTINC',
	'The directory containing the Boost header files, or "." for the system default.',
	'.')) #, PathVariable.PathAccept))
buildopts.Add(PathVariable('BOOSTLIB',
	'The directory containing the Boost libraries, or "." for the system default.',
	'.')) #, PathVariable.PathAccept))
buildopts.Add('BOOSTTOOLSET',
	'The toolset tag for Boost libraries; may be needed on Windows. Include a leading dash.',
	'')
#buildopts.Add('BOOSTABI',
#	'The ABI tag for Boost libraries. Include a leading dash.',
#	'')
buildopts.Add('BOOSTTAG',
	'Additional tags for Boost libraries. The libraries must support threading. Include leading dashes.',
	'-mt')
buildopts.Add('BOOSTVER',
	'The version tag for Boost libraries; may be needed on Windows. Include a leading dash.',
	'')
buildopts.Add(PathVariable('EIGENINC',
	'The directory containing the Eigen header files.',
	'/usr/include/eigen3/', PathVariable.PathAccept))

puname = platform.uname()

#####
# create environment for tools
toolenv = Environment(
	variables = buildopts,
	PSYS = puname[0].lower(),
	PARCH = puname[4].lower(),
	BOOSTABI = '',
	CCFLAGS = '$CCDBGFLAGS',
	LINKFLAGS = '$LINKDBGFLAGS',
	BINAPPEND = '-tools',
	BUILDTYPE = 'dbg',
	# include paths
	CPPPATH = [
		#'$BOOSTINC',
		'#/.'
	],
	# options passed to C++ compiler
	CXXFLAGS = [
		'-std=gnu++14'  # allow gcc extentions to C++, like __int128
		#'-std=c++11'    # no GNU extensions, no 128-bit integer
	],
	LIBS = [
		'libboost_program_options${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
	]
)

#####
# tool build

tools = SConscript('tools/SConscript', exports = 'toolenv', duplicate=0,
	variant_dir = toolenv.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}/tools'))
# TODO:  loop through results and add then to build alias 'tools'

#####
# bit-per-pixel image compiler

def BppiCppBuild(target, source, env):
	return subprocess.call([
		tools['bppic'].path,
		source[0].path,
		'-c',
		target[0].path
	]) != 0
bppiCppBuilder = Builder(action = BppiCppBuild,
	src_suffix = '.bppi', suffix = '.h')

def BppiArcBuild(target, source, env):
	return subprocess.call([
		tools['bppic'].path,
		source[0].path,
		'-a',
		target[0].path
	]) != 0
bppiArcBuilder = Builder(action = BppiArcBuild,
	src_suffix = '.bppi', suffix = '.bppia')


#####
# create the template build environment
env = Environment(
	variables = buildopts,
	PSYS = puname[0].lower(),
	PARCH = puname[4].lower(),
	BOOSTABI = '',
	#CC = 'distcc armv6j-hardfloat-linux-gnueabi-gcc',
	#CXX = 'distcc armv6j-hardfloat-linux-gnueabi-g++',
	# include paths
	CPPPATH = [
		#'$BOOSTINC',
		'#/.'
	],
	# options passed to C and C++ (?) compiler
	CCFLAGS = [
		# flags always used with compiler
	],
	# options passed to C++ compiler
	CXXFLAGS = [
		'-std=gnu++14'  # allow gcc extentions to C++, like __int128
		#'-std=c++11'    # no GNU extensions, no 128-bit integer
	],
	# macros
	CPPDEFINES = [
	],
	# options passed to the linker; using gcc to link
	LINKFLAGS = [
		# flags always used with linker
	],
	LIBPATH = [
		'$BOOSTLIB'
	],
	LIBS = [  # required libraries
		# a boost library must be fisrt
		'libboost_date_time${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_thread${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_serialization${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_wserialization${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		'libboost_system${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'m',
	]
)
env.Append(BUILDERS = {
	'BppiArc' : bppiArcBuilder,
	'BppiCpp' : bppiCppBuilder,
})


#####
# Debugging build enviornment
dbgenv = env.Clone(LIBS = [ ])  # no libraries; needed for library check
dbgenv.AppendUnique(
	CCFLAGS = '$CCDBGFLAGS',
	LINKFLAGS = '$LINKDBGFLAGS',
	BINAPPEND = '-dbg',
	BUILDTYPE = 'dbg',
)

# While SCons has a nice way of dealing with configurations, it doesn't
# automatically add things in a way that works for the different build
# enviornments here, unless the configuration is done once for each, which
# doesn't seem to be necessary.
# These libraries will be linked with a test program or will be deemed to not
# exist, so this doesn't work with header-only libraries.
optionalLibs = {
	# key is the macro, value is the library
	'LIBBOOST_FILESYSTEM' :
		'libboost_filesystem${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
	'LIBBOOST_TEST' :
		'libboost_unit_test_framework${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
	#'LIBBOOST_PROGRAM_OPTIONS' :
	#	'libboost_program_options${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
}
# Similar to above, but only for the debug build.
optionalDbgLibs = { }

#####
# extra cleaning
if GetOption('clean'):
	Execute(Delete(Glob('*~') + [
		'duds/BuildConfig.h',
		'config.log',
		# The following items make for a more complete clean, but if different
		# hosts with different targets use the same build files (NFS or the
		# like), they should both be deleted. Delting .sconsign.dblite will
		# force everything to be rebuilt. Just deleting .conf/... will cause
		# the host of the given target to fail during the configuration check
		# further below because SCons will not remake the source files for the
		# check, but will attempt to compile them anyway (file not found). It
		# is an SCons bug.
		#'.sconsign.dblite',
		#env.subst('.conf/${PSYS}-${PARCH}')
	] ))
	env['Use_Eigen'] = False
	dbgenv['Use_Eigen'] = False
	env['Use_GpioDevPort'] = True
	dbgenv['Use_GpioDevPort'] = True

#####
# configure the build
else:
	#####
	# Configuration for Boost libraries
	conf = Configure(dbgenv, config_h = 'duds/BuildConfig.h',
		conf_dir = env.subst('.conf/${PSYS}-${PARCH}')) #, help=False)
	# check for debugging versions of the Boost libraries, then for a non-debug version
	dbgenv['BOOSTABI'] = '-gd'
	if not conf.CheckLib(dbgenv.subst(env['LIBS'][0]), language = 'C++', autoadd=0):
		dbgenv['BOOSTABI'] = '-d'
		if not conf.CheckLib(dbgenv.subst(env['LIBS'][0]), language = 'C++', autoadd=0):
			dbgenv['BOOSTABI'] = ''
			if not conf.CheckLib(dbgenv.subst(env['LIBS'][0]), language = 'C++', autoadd=0):
				print 'No suitable Boost libraries could be found.'
				# really lame to not give help when there is a config problem,
				# but that is what is going to happen; either exit here or get
				# errors in the script, both of which result in no help.
				#if GetOption('help'):
				#	Help(buildopts.GenerateHelpText(env, cmp))
				#Exit(1)
				if not GetOption('help'):
					Exit(1)
	# check optional libraries
	remlibs = [ ]
	for mac, lib in optionalLibs.iteritems():
		if conf.CheckLib(dbgenv.subst(lib), language = 'C++', autoadd=0):
			conf.Define('HAVE_' + mac, 1, 'optional library')
		else:
			remlibs.append(mac)
	for mac in remlibs:
		del optionalLibs[mac]
	#
	# Linking to python looks bothersome. The Boost libraries don't follow the
	# same conventions as the other Boost libraries, and not all systems seem
	# to use the documented naming convention in the Boost documentation. Maybe
	# this will be cleared up sometime after Boost 1.54.
	#
	# check for Boost's Python library
	#if conf.CheckLib(dbgenv.subst('libboost_python${BOOSTTOOLSET}${BOOSTTAG}-py${BOOSTABI}${BOOSTVER}'), language = 'C++', autoadd=1):
	#	dbgenv['USE_PYTHON'

	# Boost stacktrace (broken in 1.65.0, maybe earlier; fixed in 1.65.1)
	# includes push_options.pp, but the file isn't installed.
	# Also, the dl library is required.
	dbgenv.Append(CPPDEFINES = 'BOOST_STACKTRACE_USE_ADDR2LINE')
	if conf.CheckCXXHeader('boost/stacktrace.hpp') and \
	conf.CheckLib('dl', language = 'C++', autoadd=0):
		optionalDbgLibs['DUDS_ERRORS_VERBOSE'] = 'dl'
	# remove the macro used for the test
	dbgenv['CPPDEFINES'] = env['CPPDEFINES']

	# Eigen
	dbgenv.Append(CPPPATH = '$EIGENINC')
	if conf.CheckCXXHeader('Eigen/Geometry'):
		env['Use_Eigen'] = True
		dbgenv['Use_Eigen'] = True
	else:
		env['Use_Eigen'] = False
		dbgenv['Use_Eigen'] = False
	# remove the Eigen path; only add where needed
	dbgenv['CPPPATH'] = env['CPPPATH']

	# 128-bit integer support
	if conf.CheckType('__int128', language = 'C++'):
		conf.Define('HAVE_INT128', 1, 'A 128-bit integer type is available.')
	
	# Linux support for GPIO control through a character device
	if conf.CheckCHeader('linux/gpio.h'):
		env['Use_GpioDevPort'] = True
		dbgenv['Use_GpioDevPort'] = True
	else:
		env['Use_GpioDevPort'] = False
		dbgenv['Use_GpioDevPort'] = False

	dbgenv = conf.Finish()

# add back the libraries
dbgenv['LIBS'] = env['LIBS']
# put in additional debug libs
for mac, lib in optionalDbgLibs.iteritems():
	dbgenv.Append(
		CPPDEFINES = mac,
		LIBS = lib
	)
# remove Boost unit test library; should only be added for test programs
if 'LIBBOOST_TEST' in optionalLibs:
	del optionalLibs['LIBBOOST_TEST']
	havetestlib = True
else:
	havetestlib = False

#####
# Optimized build enviornment
optenv = env.Clone()
optenv.AppendUnique(
	CCFLAGS = '$CCOPTFLAGS',
	CPPDEFINES = 'NDEBUG',
	LINKFLAGS = '$LINKOPTFLAGS',
	BINAPPEND = '',
	BUILDTYPE = 'opt'
)

#####
# library build

if not GetOption('help'):
	envs = [ dbgenv, optenv ]
	#trgs = [ [ ], [ ] ]
	
	for env in envs:
		# add in optional libraries
		for mac, lib in optionalLibs.iteritems():
			env.Append(
				#CPPDEFINES = ('HAVE_' + mac, 1),
				LIBS = lib
			)
		# build library
		libs = SConscript('duds/SConscript', exports = 'env', duplicate=0,
			variant_dir = env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}/lib'))
		env.Clean(libs, env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}/lib'))
		Alias('libs-' + env['BUILDTYPE'], libs)
		# sample programs
		samps = SConscript('samples/SConscript', exports = 'env libs tools', duplicate=0,
			variant_dir = env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}/samples'))
		Alias('samples-' + env['BUILDTYPE'], samps)
		# test programs
		if havetestlib: #'LIBBOOST_TEST' in optionalLibs:
			tests = SConscript('tests/SConscript', exports = 'env libs tools', duplicate=0,
				variant_dir = env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}/tests'))
			Alias('tests-' + env['BUILDTYPE'], tests)
	
	Alias('libs', 'libs-dbg')
	Alias('samples', 'samples-dbg')
	if havetestlib:
		Alias('tests', 'tests-dbg')
	Default('libs-dbg')

#####
# setup help text for the build options
Help(buildopts.GenerateHelpText(env, cmp))
if GetOption('help'):
	print 'Build target aliases:'
	print '  libs-dbg    - All libraries; debugging build. This is the default.'
	print '  libs-opt    - All libraries; optimized build.'
	print '  libs        - Same as libs-dbg'
	print '  samples-dbg - All sample programs; debugging build.'
	print '  samples-opt - All sample programs; optimized build.'
	print '  samples     - Same as samples-dbg.'
	if havetestlib:
		print '  tests-dbg   - All unit test programs; debugging build.'
		print '  tests-opt   - All unit test programs; optimized build.'
		print '  tests       - Same as tests-dbg.'
