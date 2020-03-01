# INSTALL_EXTERNAL_PROJECT: install a project created with ExternalProject_Add in the
# parent project's install time.
#
# Description:
# In a regular scenario, cmake will install external projects
# BEFORE actually building the parent project. Since the building
# process may use installed components from the project.
# We want to give the external project the ability to install
# files directly to the parent's install. Therefore, we have to
# manually trigger the install stage with the parent's INSTALL_PREFIX.
MACRO(INSTALL_EXTERNAL_PROJECT name)
	ExternalProject_Get_Property(${name} BINARY_DIR)

	install(CODE "include(\"${BINARY_DIR}/cmake_install.cmake\")")
ENDMACRO()

IF(LMMS_BUILD_WIN32 AND NOT LMMS_BUILD_WIN64)
	ADD_SUBDIRECTORY(RemoteVstPlugin)
ELSEIF(LMMS_BUILD_WIN64 AND MSVC)
	IF(NOT QT_32_PREFIX)
		SET(LMMS_MSVC_YEAR_FOR_QT ${LMMS_MSVC_YEAR})

		if(LMMS_MSVC_YEAR_FOR_QT EQUAL 2019)
			SET(LMMS_MSVC_YEAR_FOR_QT 2017) # Qt only provides binaries for MSVC 2017, but 2019 is binary compatible
		endif()

		GET_FILENAME_COMPONENT(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
		SET(QT_32_PREFIX "${QT_BIN_DIR}/../../msvc${LMMS_MSVC_YEAR_FOR_QT}")
	ENDIF()

	#TODO: qt5 installed using vcpkg: I don't know how to detect if the user built the x86 version of qt5 from here. At least not cleanly.
	#So for the moment, we'll allow the built.
	IF(NOT (IS_DIRECTORY ${QT_32_PREFIX} AND EXISTS ${QT_32_PREFIX}/bin/qmake.exe))
		MESSAGE(WARNING "No Qt 32 bit installation found at ${QT_32_PREFIX}. If you're using VCPKG you can ignore this message if you've built x86-windows version of qt5")
	ENDIF()

	ExternalProject_Add(RemoteVstPlugin32
		"${EXTERNALPROJECT_ARGS}"
		CMAKE_GENERATOR "${LMMS_MSVC_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM Win32
		#CMAKE_GENERATOR_TOOLSET "${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS
			"${EXTERNALPROJECT_CMAKE_ARGS}"
			"-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
			"-DCMAKE_PREFIX_PATH=${QT_32_PREFIX}"
	)

	INSTALL_EXTERNAL_PROJECT(RemoteVstPlugin32)
ELSEIF(LMMS_BUILD_LINUX)
	# Use winegcc
	INCLUDE(CheckWineGcc)
	CheckWineGcc(32 "${WINEGCC}" WINEGCC_WORKING)
	IF(NOT WINEGCC_WORKING)
		MESSAGE(WARNING "winegcc fails to complie 32-bit binaries, please make sure you have 32-bit GCC libraries")
		RETURN()
	ENDIF()
	ExternalProject_Add(RemoteVstPlugin32
		"${EXTERNALPROJECT_ARGS}"
		CMAKE_ARGS
			"${EXTERNALPROJECT_CMAKE_ARGS}"
			"-DCMAKE_CXX_COMPILER=${WINEGCC}"
			"-DCMAKE_CXX_FLAGS=-m32"
	)

	INSTALL(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/../32/RemoteVstPlugin32" "${CMAKE_CURRENT_BINARY_DIR}/../32/RemoteVstPlugin32.exe.so" DESTINATION "${PLUGIN_DIR}/32")

ELSEIF(CMAKE_TOOLCHAIN_FILE_32)
	ExternalProject_Add(RemoteVstPlugin32
		"${EXTERNALPROJECT_ARGS}"
		CMAKE_ARGS
			"${EXTERNALPROJECT_CMAKE_ARGS}"
			"-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_32}"
			"-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE_32}"
	)
	INSTALL_EXTERNAL_PROJECT(RemoteVstPlugin32)
ELSE()
	MESSAGE(WARNING "Can't build RemoteVstPlugin32, unknown environment. Please supply CMAKE_TOOLCHAIN_FILE_32 and optionally CMAKE_PREFIX_PATH_32")
	RETURN()
ENDIF()

