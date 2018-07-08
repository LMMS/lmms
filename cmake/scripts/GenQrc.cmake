# GenQrcScript.cmake - Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>

INCLUDE(CMakeParseArguments)

FILE(REMOVE ${OUT_FILE})
MACRO(OUT STRING)
	FILE(APPEND ${OUT_FILE} "${STRING}\n")
ENDMACRO()

IF(NOT DEFINED RC_PREFIX)
	SET(RC_PREFIX "/")
ENDIF()

# Write qrc file
OUT("<RCC>")
OUT("	<qresource prefix=\"${RC_PREFIX}\">")
FOREACH(VAR ${FILES})
	GET_FILENAME_COMPONENT(FILENAME ${VAR} NAME)
	IF(IS_ABSOLUTE ${VAR})
		OUT("		<file alias=\"${FILENAME}\">${VAR}</file>")
	ELSE()
		OUT("		<file alias=\"${FILENAME}\">${DIR}/${VAR}</file>")
	ENDIF()
ENDFOREACH()
OUT("	</qresource>")
OUT("</RCC>")
