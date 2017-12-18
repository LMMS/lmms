INCLUDE(BuildPlugin)

if(PKG_CONFIG_FOUND AND NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
	pkg_check_modules(LIBSPA spa)

	if (${LIBSPA_FOUND})
		message(STATUS "SPA plugin enabled")
		include_directories(${LIBSPA_INCLUDE_DIRS})
		link_directories(${LIBSPA_LIBRARY_DIRS})
		
		set(SPA_SRCS core/spa/SpaInstrument.cpp PARENT_SCOPE)
	endif()
endif()

