# Try config mode first
find_package(unofficial-libstk CONFIG QUIET)

if(TARGET unofficial::libstk::libstk)
	# Extract details for find_package_handle_standard_args
	get_target_property(STK_LIBRARY unofficial::libstk::libstk LOCATION)
	get_target_property(STK_INCLUDE_DIR unofficial::libstk::libstk INTERFACE_INCLUDE_DIRECTORIES)
else()
	find_path(STK_INCLUDE_DIR
		NAMES stk/Stk.h
		PATH /usr/include /usr/local/include "${CMAKE_INSTALL_PREFIX}/include" "${CMAKE_FIND_ROOT_PATH}/include"
	)

	find_library(STK_LIBRARY
		NAMES stk
		PATH /usr/lib /usr/local/lib "${CMAKE_INSTALL_PREFIX}/lib" "${CMAKE_FIND_ROOT_PATH}/lib"
	)

	if(STK_INCLUDE_DIR AND STK_LIBRARY)
		# Yes, this target name is hideous, but it matches that provided by vcpkg
		add_library(unofficial::libstk::libstk UNKNOWN IMPORTED)
		set_target_properties(unofficial::libstk::libstk PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${STK_INCLUDE_DIR}"
			IMPORTED_LOCATION "${STK_LIBRARY}"
		)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(STK
	REQUIRED_VARS STK_LIBRARY STK_INCLUDE_DIR
)
