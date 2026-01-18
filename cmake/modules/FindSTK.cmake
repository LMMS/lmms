include(ImportedTargetHelpers)

# TODO CMake 3.18: Alias this target to something less hideous
find_package_config_mode_with_fallback(unofficial-libstk unofficial::libstk::libstk
	LIBRARY_NAMES "stk"
	INCLUDE_NAMES "stk/Stk.h"
	LIBRARY_HINTS "/usr/lib" "/usr/local/lib" "${CMAKE_INSTALL_PREFIX}/lib" "${CMAKE_FIND_ROOT_PATH}/lib"
	INCLUDE_HINTS "/usr/include" "/usr/local/include" "${CMAKE_INSTALL_PREFIX}/include" "${CMAKE_FIND_ROOT_PATH}/include"
	PREFIX STK
)

function(getListOfVarsStartingWith _prefix _varResult)
	get_cmake_property(_vars VARIABLES)
	string(REGEX MATCHALL "(^|;)${_prefix}[A-Za-z0-9_]*" _matchedVars "${_vars}")
	set(${_varResult} ${_matchedVars} PARENT_SCOPE)
endfunction()

message(STATUS "~~~~~ STK_LIBRARY: ${STK_LIBRARY}")
message(STATUS "~~~~~ STK_INCLUDE_DIRS: ${STK_INCLUDE_DIRS}")

getListOfVarsStartingWith("STK_" matchedVars)
foreach(_var IN LISTS matchedVars)
	message(STATUS "~~~~~ ${_var}=${${_var}}")
endforeach()

# Find STK rawwave path
if(STK_INCLUDE_DIRS)
	message(STATUS "~~~~~ include dirs exists")
	list(GET STK_INCLUDE_DIRS 0 STK_INCLUDE_DIR)
	message(STATUS "~~~~~ STK_INCLUDE_DIR=${STK_INCLUDE_DIR}")
	find_path(STK_RAWWAVE_ROOT
		NAMES silence.raw sinewave.raw
		HINTS "${STK_INCLUDE_DIR}/.." "${STK_INCLUDE_DIR}/../.."
		PATH_SUFFIXES share/stk/rawwaves share/libstk/rawwaves
	)
endif()

message(STATUS "~~~~~~~~~~~~~~~~~~~~~~")
getListOfVarsStartingWith("STK_" matchedVars)
foreach(_var IN LISTS matchedVars)
	message(STATUS "~~~~~ ${_var}=${${_var}}")
endforeach()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(STK
	REQUIRED_VARS STK_LIBRARY STK_INCLUDE_DIR STK_RAWWAVE_ROOT
	# STK doesn't appear to expose its version, so we can't pass it here
)
