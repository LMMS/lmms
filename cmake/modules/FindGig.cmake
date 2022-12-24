
# Attempt to find package libgig (vcpkg first), then fall back to pkg-config if not found
find_package(libgig ${Gig_FIND_VERSION} CONFIG QUIET)

if(libgig_FOUND)
	get_target_property(libgig_Location libgig::libgig LOCATION)
	get_target_property(libgig_Include_Path libgig::libgig INTERFACE_INCLUDE_DIRECTORIES)
	get_filename_component(libgig_Path LibGig_Location PATH)
else()
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBGIG_PKG gig)
	endif()
endif()

# Find the library and headers using the results from find_package or PkgConfig as a guide
find_path(LIBGIG_INCLUDE_DIR
	NAMES gig.h
	PATHS ${LIBGIG_PKG_INCLUDE_DIRS} "${libgig_Include_Path}/libgig"
)

# vcpkg finds libgig, pkg-config finds gig
find_library(LIBGIG_LIBRARY
	NAMES gig libgig
	PATHS ${LIBGIG_PKG_LIBRARY_DIRS} ${libgig_Path}
)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(Gig DEFAULT_MSG LIBGIG_LIBRARY LIBGIG_INCLUDE_DIR)

set(GIG_LIBRARY_DIRS ${LIBGIG_LIBRARY})
set(GIG_LIBRARIES ${LIBGIG_LIBRARY})
set(GIG_INCLUDE_DIRS ${LIBGIG_INCLUDE_DIR})

mark_as_advanced(LIBGIG_LIBRARY LIBGIG_LIBRARIES LIBGIG_INCLUDE_DIR LIBGIG_INCLUDE_DIRS)
