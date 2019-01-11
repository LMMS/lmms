# FindFluidSynth.cmake
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT
#
# This package will define
# FLUIDSYNTH_FOUND
# FLUIDSYNTH_LIBRARIES
# FLUIDSYNTH_INCLUDE_DIRS

# Try pkgconfig
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(FLUIDSYNTH_PKG libfluidsynth >= 1.0.7)
endif(PKG_CONFIG_FOUND)

find_path(FLUIDSYNTH_INCLUDE_DIR 
	NAMES fluidsynth.h
	PATHS FLUIDSYNTH_PKG_INCLUDE_DIRS)
find_library(FLUIDSYNTH_LIBRARY
	NAMES fluidsynth libfluidsynth
	PATHS FLUIDSYNTH_PKG_LIBRARY_DIRS)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(FluidSynth DEFAULT_MSG FLUIDSYNTH_LIBRARY FLUIDSYNTH_INCLUDE_DIR)

set(FLUIDSYNTH_LIBRARIES ${FLUIDSYNTH_LIBRARY})
set(FLUIDSYNTH_INCLUDE_DIRS ${FLUIDSYNTH_INCLUDE_DIR})

mark_as_advanced(FLUIDSYNTH_LIBRARY FLUIDSYNTH_INCLUDE_DIR)
