# Attempts to set Qt5_DIR, Qt5Test_DIR by looking for a valid Qt5 install
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Windows:
#     [x] msvc:      %SystemDrive%/Qt/5.x.x/msvc20xx
#     [ ] mingw:     %SystemDrive%/Qt/5.x.x/mingwxx_xx/

# qtcreator + msvc
IF(MSVC)
	MESSAGE("-- Found MSVC version ${MSVC_VERSION}")
	# Assume that .cpp file associations were registered during install
	GET_FILENAME_COMPONENT(QT_BIN [HKEY_CURRENT_USER\\Software\\Classes\\Applications\\QtProject.QtCreator.cpp\\shell\\Open\\Command] PATH)
	IF(QT_BIN MATCHES "/Tools")
		STRING(REPLACE "//" "/"  QT_BIN "${QT_BIN}")
		MESSAGE("-- Found QtCreator installed ${QT_BIN}")
		STRING(REPLACE "/Tools" ";" QT_BIN "${QT_BIN}")
		LIST(GET QT_BIN 0 QT_BIN)
		FILE(GLOB QT_VERSIONS "${QT_BIN}/5.*")
		STRING(REPLACE "//" "/"  QT_VERSIONS "${QT_VERSIONS}")
		LIST(SORT QT_VERSIONS)
		LIST(REVERSE QT_VERSIONS)
		MESSAGE("-- Found Qt Frameworks ${QT_VERSIONS}")
		LIST(GET QT_VERSIONS 0 QT_VERSION)
		MESSAGE("-- Selecting Qt Framework ${QT_VERSION}")
		
		# Decode cryptic MSVC version info
		IF(MSVC_VERSION LESS 1900)
			MATH(EXPR MSVC_FOLDER "2000 + (${MSVC_VERSION} - 500) / 100")
			SET(MSVC_FOLDER "msvc${MSVC_FOLDER}")
		ELSEIF(MSVC_VERSION EQUAL 1900)
			SET(MSVC_FOLDER "msvc2015")
		ELSEIF(MSVC_VERSION GREATER 1909)
			SET(MSVC_FOLDER "msvc2017")
		ENDIF()
		
		MESSAGE("-- Found MSVC match ${QT_VERSION}/${MSVC_FOLDER}")

		IF(MSVC_FOLDER)
			IF(CMAKE_GENERATOR_PLATFORM MATCHES 64)
				SET(MSVC_FOLDER "${MSVC_FOLDER}_64")
			ENDIF()
			SET(QT_PATH "${QT_VERSION}/${MSVC_FOLDER}")
			SET(QT_MISSING False)
		ENDIF()
	ELSE()
		MESSAGE("-- Could not find QtCreator")
	ENDIF()
ENDIF()
