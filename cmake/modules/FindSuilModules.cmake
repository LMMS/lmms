# Copyright (c) 2024 Tres Finocchiaro
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# This module defines
# Suil_MODULES: List of full paths to Suil modules (e.g. "/usr/lib/suil-0/libsuil_x11.so;...")
# Suil_MODULES_PREFIX: Only the directory name of the Suil_MODULES path (e.g. "suil-0")

pkg_check_modules(Suil QUIET suil-0)

if(Suil_FOUND)
	if(APPLE)
		set(_lib_ext "dylib")
	elseif(WIN32)
		set(_lib_ext "dll")
	else()
		set(_lib_ext "so")
	endif()

	# Isolate -- if needed -- the first suil library path (e.g. "/usr/lib/libsuil-0.so")
	list(GET Suil_LINK_LIBRARIES 0 _lib)
	if(EXISTS "${_lib}")
		# Isolate -- if needed -- the first suil library name (e.g. "suil-0")
		list(GET Suil_LIBRARIES 0 _modules_prefix)
		get_filename_component(_lib_dir "${_lib}" DIRECTORY)
		# Construct modules path (e.g. "/usr/lib/suil-0")
		set(_modules_dir "${_lib_dir}/${_modules_prefix}")
		if(IS_DIRECTORY "${_modules_dir}")
			set(Suil_MODULES_PREFIX "${_modules_prefix}")
			file(GLOB Suil_MODULES "${_modules_dir}/*.${_lib_ext}")
			list(SORT Suil_MODULES)
		endif()
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SuilModules
	REQUIRED_VARS Suil_MODULES Suil_MODULES_PREFIX
)