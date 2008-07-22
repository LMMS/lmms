# - Try to find precompiled headers support for GCC 3.4 and 4.x
# Once done this will define:
#
# Variable:
#   PCHSupport_FOUND
#
# Macro:
#   ADD_PRECOMPILED_HEADER

IF(CMAKE_COMPILER_IS_GNUCXX)
	EXEC_PROGRAM(
		${CMAKE_CXX_COMPILER} 
		ARGS                    --version 
		OUTPUT_VARIABLE _compiler_output)
	STRING(REGEX REPLACE ".* ([0-9]\\.[0-9]\\.[0-9]) .*" "\\1" 
		gcc_compiler_version ${_compiler_output})
	#MESSAGE("GCC Version: ${gcc_compiler_version}")
	IF(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
		SET(PCHSupport_FOUND TRUE)
	ELSE(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
		IF(gcc_compiler_version MATCHES "3\\.4\\.[0-9]")
			SET(PCHSupport_FOUND TRUE)
		ENDIF(gcc_compiler_version MATCHES "3\\.4\\.[0-9]")
	ENDIF(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
	IF(PCHSupport_FOUND)
		ADD_DEFINITIONS( -DUSE_PCH=1 )
	ELSE(PCHSupport_FOUND)
		ADD_DEFINITIONS( -DUSE_PCH=0 )
	ENDIF(PCHSupport_FOUND)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)


# ADD_PRECOMPILED_HEADER( targetName HEADERS _inputs )
MACRO(ADD_PRECOMPILED_HEADER _targetName _inputs )
	FOREACH (_current_FILE ${ARGN})

		GET_FILENAME_COMPONENT(_name ${_current_FILE} NAME)
		SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_current_FILE}")
		SET(_outdir "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch")
		MAKE_DIRECTORY(${_outdir})
		SET(_output "${_outdir}/${CMAKE_BUILD_TYPE}.c++")
		STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
		SET(_compiler_FLAGS ${${_flags_var_name}})

		GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS "-I${item}")
		ENDFOREACH(item)

		GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
		LIST(APPEND _compiler_FLAGS ${_directory_flags})

		# some hacks for Qt4 - Tobias Doerffel, 2008
		STRING(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" CD)
		GET_DIRECTORY_PROPERTY(_directory_flags ${CD} )
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS "-D${item}")
		ENDFOREACH(item)
		GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_DEFINITIONS)
		FOREACH(item ${_directory_flags})
			LIST(APPEND _compiler_FLAGS "-D${item}")
		ENDFOREACH(item)

		SEPARATE_ARGUMENTS(_compiler_FLAGS)
		#MESSAGE("_compiler_FLAGS: ${_compiler_FLAGS}")
		#message("${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header -o ${_output} ${_source}")
		ADD_CUSTOM_COMMAND(
			OUTPUT ${_output}
			COMMAND ${CMAKE_CXX_COMPILER}
			${_compiler_FLAGS}
			${COMPILE_DEFINITIONS}
			-x c++-header
			-o ${_output} ${_source}
			DEPENDS ${_source} )
		ADD_CUSTOM_TARGET(${_targetName}_gch DEPENDS ${_output})
		ADD_DEPENDENCIES(${_targetName} ${_targetName}_gch)
		#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-include ${_name} -Winvalid-pch -H")
		#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${_name} -Winvalid-pch")
		SET_TARGET_PROPERTIES(${_targetName} PROPERTIES
			COMPILE_FLAGS "-include ${_name} -Winvalid-pch"
			)
	ENDFOREACH (_current_FILE)        
ENDMACRO(ADD_PRECOMPILED_HEADER)

# ADD_PRECOMPILED_HEADER_INPLACE( targetName HEADERS _inputs )
MACRO(ADD_PRECOMPILED_HEADER_INPLACE _targetName _inputs )
	STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
	SET(_compiler_FLAGS ${${_flags_var_name}})

	GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
	FOREACH(item ${_directory_flags})
		LIST(APPEND _compiler_FLAGS "-I${item}")
	ENDFOREACH(item)

	GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	LIST(APPEND _compiler_FLAGS ${_directory_flags})

	SEPARATE_ARGUMENTS(_compiler_FLAGS)
	#MESSAGE("_compiler_FLAGS: ${_compiler_FLAGS}")
	MAKE_DIRECTORY("${CMAKE_CURRENT_BINARY_DIR}/gch")
	FOREACH (_current_FILE ${ARGN})

		GET_FILENAME_COMPONENT(_name ${_current_FILE} NAME)
		SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_current_FILE}")
		SET(_output "${CMAKE_CURRENT_BINARY_DIR}/gch/${_name}.gch")
		#message("${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header -o ${_output} ${_source}")
		ADD_CUSTOM_COMMAND(
			OUTPUT ${_output}
			COMMAND ${CMAKE_CXX_COMPILER}
			${_compiler_FLAGS}
			-x c++-header
			-fPIC
			-o ${_output} ${_source}
			DEPENDS ${_source} )
		LIST(APPEND GCH_FILES ${_output})
	ENDFOREACH (_current_FILE)        

	#MESSAGE("GCH_FILES: ${GCH_FILES}")
	FOREACH (GCH_FILE ${GCH_FILES})
		SET_DIRECTORY_PROPERTIES( PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${GCH_FILE} )
	ENDFOREACH (GCH_FILE)
	SEPARATE_ARGUMENTS(GCH_FILES)
	ADD_CUSTOM_TARGET(${_targetName}_gch DEPENDS ${GCH_FILES})
	ADD_DEPENDENCIES(${_targetName} ${_targetName}_gch)
	INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/gch/)
	SET_TARGET_PROPERTIES(${_targetName} PROPERTIES COMPILE_FLAGS "-Winvalid-pch")
ENDMACRO(ADD_PRECOMPILED_HEADER_INPLACE)
