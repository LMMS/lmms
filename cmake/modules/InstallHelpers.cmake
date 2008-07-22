#
# install all files matching certain wildcards below ${LMMS_DATA_DIR}/<subdir>
#
# example:
#
#   INSTALL_DATA_SUBDIRS("samples" "*.ogg;*.wav;*.flac")
#
# Copyright (c) 2008 Tobias Doerffel
#


# helper-macro
MACRO(LIST_CONTAINS var value)
	SET(${var})
		FOREACH (value2 ${ARGN})
			IF (${value} STREQUAL ${value2})
				SET(${var} TRUE)
			ENDIF (${value} STREQUAL ${value2})
	ENDFOREACH (value2)
ENDMACRO(LIST_CONTAINS)


MACRO(INSTALL_DATA_SUBDIRS _subdir _wildcards)
	FOREACH(_wildcard ${_wildcards})
		FILE(GLOB_RECURSE files ${_wildcard})
		SET(SUBDIRS)

		FOREACH(_item ${files})
			GET_FILENAME_COMPONENT(_file "${_item}" PATH)
			STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" _file "${_file}")
			LIST_CONTAINS(contains _file ${SUBDIRS})
			IF(NOT contains)
				LIST(APPEND SUBDIRS "${_file}")
			ENDIF(NOT contains)
		ENDFOREACH(_item ${files})

		FOREACH(_item ${SUBDIRS})
			FILE(GLOB files "${_item}/${_wildcard}")
			FOREACH(_file ${files})
				INSTALL(FILES "${_file}" DESTINATION "${LMMS_DATA_DIR}/${_subdir}/${_item}/")
			ENDFOREACH(_file ${files})
		ENDFOREACH(_item ${SUBDIRS})
	ENDFOREACH(_wildcard ${_wildcards})
ENDMACRO(INSTALL_DATA_SUBDIRS)

