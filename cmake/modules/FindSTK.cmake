IF (MSVC)
	#try vcpkg
	FIND_PACKAGE(unofficial-libstk CONFIG)
	IF(unofficial-libstk_FOUND)
			GET_TARGET_PROPERTY(LibStk_Location_Debug unofficial::libstk::libstk LOCATION_DEBUG)
			GET_TARGET_PROPERTY(LibStk_Location_Release unofficial::libstk::libstk LOCATION_RELEASE)
			SET(STK_LIBRARY 
				$<$<CONFIG:Debug>:${LibStk_Location_Debug}> 
				$<$<NOT:$<CONFIG:Debug>>:${LibStk_Location_Release}>)
			GET_TARGET_PROPERTY(LibStk_Include_Path unofficial::libstk::libstk INTERFACE_INCLUDE_DIRECTORIES)
			SET(STK_INCLUDE_DIR "${LibStk_Include_Path}/stk")
	ENDIF(unofficial-libstk_FOUND)
ENDIF (MSVC)

IF(NOT unofficial-libstk_FOUND)
	FIND_PATH(STK_INCLUDE_DIR Stk.h /usr/include/stk /usr/local/include/stk ${CMAKE_INSTALL_PREFIX}/include/stk ${CMAKE_FIND_ROOT_PATH}/include/stk)
	FIND_LIBRARY(STK_LIBRARY NAMES stk PATH /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_FIND_ROOT_PATH}/lib) 
ENDIF()

IF (STK_INCLUDE_DIR AND STK_LIBRARY)
	SET(STK_FOUND TRUE)
ENDIF (STK_INCLUDE_DIR AND STK_LIBRARY)


IF (STK_FOUND)
	IF (NOT STK_FIND_QUIETLY)
		MESSAGE(STATUS "Found STK: ${STK_LIBRARY}")
	SET(HAVE_STK TRUE)
	ENDIF (NOT STK_FIND_QUIETLY)
ELSE (STK_FOUND)
	IF (STK_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find STK")
	ENDIF (STK_FIND_REQUIRED)
ENDIF (STK_FOUND)

