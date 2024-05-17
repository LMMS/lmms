install(DIRECTORY icons DESTINATION "${DATA_DIR}/icons/hicolor")
install(FILES lmms.desktop DESTINATION "${DATA_DIR}/applications")
install(FILES lmms.xml DESTINATION "${DATA_DIR}/mime/packages")

# Preserve old CPack behavior
if(WANT_CPACK_TARBALL)
	message(STATUS "Skipping AppImage creation")
	return()
endif()

# Copy rawwaves into bundle
file(GLOB RAWWAVES "${STK_RAWWAVE_ROOT}/*.raw")
install(FILES ${RAWWAVES} DESTINATION share/stk)

install(FILES launch_lmms.sh DESTINATION bin)

# Standard CPack options
set(CPACK_GENERATOR "External" PARENT_SCOPE)
set(CPACK_EXTERNAL_ENABLE_STAGING true PARENT_SCOPE)
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}" PARENT_SCOPE)

# Custom vars to expose to Cpack
# must be prefixed with "CPACK_" per https://stackoverflow.com/a/46526757/3196753)
set(CPACK_CURRENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
set(CPACK_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(CPACK_BINARY_DIR "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
set(CPACK_SOURCE_DIR "${CMAKE_SOURCE_DIR}" PARENT_SCOPE)
set(CPACK_QMAKE_EXECUTABLE "${QT_QMAKE_EXECUTABLE}" PARENT_SCOPE)
set(CPACK_CARLA_LIBRARIES "${CARLA_LIBRARIES}" PARENT_SCOPE)
set(CPACK_WINE_32_LIBRARY_DIRS "${WINE_32_LIBRARY_DIRS}" PARENT_SCOPE)
set(CPACK_WINE_64_LIBRARY_DIRS "${WINE_64_LIBRARY_DIRS}" PARENT_SCOPE)
set(CPACK_PROJECT_NAME "${PROJECT_NAME}" PARENT_SCOPE)
set(CPACK_PROJECT_NAME_UCASE "${PROJECT_NAME_UCASE}" PARENT_SCOPE)
set(CPACK_PROJECT_VERSION "${VERSION}" PARENT_SCOPE)
set(CPACK_CMAKE_COMMAND "${CMAKE_COMMAND}" PARENT_SCOPE)

# TODO: Canidate for DetectMachine.cmake
if(IS_X86_64)
	set(CPACK_TARGET_ARCH x86_64 PARENT_SCOPE)
elseif(IS_X86)
	set(CPACK_TARGET_ARCH i386 PARENT_SCOPE)
elseif(IS_ARM64)
	set(CPACK_TARGET_ARCH aarch64 PARENT_SCOPE)
elseif(IS_ARM32)
	set(CPACK_TARGET_ARCH armhf PARENT_SCOPE)
else()
	set(CPACK_TARGET_ARCH unknown PARENT_SCOPE)
endif()

# Build tool "linuxdeployqt" is VERY difficult to work with due to Ubuntu 20.04 requirement
# - See myriad of complaints and issues here: https://github.com/probonopd/linuxdeployqt/issues/340
# - Build tool "linuxdeploy" (without the "qt") does NOT have these limitations
#
# If viable:
# - The below "WANT_LINUXDEPLOY_QT" can be removed, we move away from "linuxdeployqt" forever
#
# If not viable:
# - We can keep them both (yuck) and document the toggle/flag in root /CMakeLists.txt
#
# TODO: Remove this check; Default to LinuxDeploy; Remove LinuxDeployQt.cmake
#
if(WANT_LINUXDEPLOY_QT)
	set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/LinuxDeployQt.cmake" PARENT_SCOPE)
else()
	set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/LinuxDeploy.cmake" PARENT_SCOPE)
endif()

if(CMAKE_VERSION VERSION_LESS "3.19")
	message(WARNING "AppImage creation requires at least CMake 3.19")
endif()