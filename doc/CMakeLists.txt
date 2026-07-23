cmake_minimum_required(VERSION 3.27)

project(lmms)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules" ${CMAKE_MODULE_PATH})

find_package(Doxygen REQUIRED)
configure_file(Doxyfile.in Doxyfile @ONLY)
doxygen_add_docs(doc
	USE_STAMP_FILE
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Generating HTML documentation with Doxygen"
	CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

add_subdirectory(bash-completion)
