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
	GET_FILENAME_COMPONENT(QT_BIN [HKEY_CURRENT_USER\\Software\\Classes\\Applications\\QtProject.QtCreator.cpp\\shell\\Open\\Command] PATH)
	IF(QT_BIN MATCHES "/Tools")
		STRING(REPLACE "/Tools" ";" QT_BIN "${QT_BIN}")
		LIST(GET QT_BIN 0 QT_BIN)
		FILE(GLOB QT_VERSIONS "${QT_BIN}/5.*")
		LIST(SORT QT_VERSIONS)
		LIST(REVERSE QT_VERSIONS)
		LIST(GET QT_VERSIONS 0 QT_VERSION)
		STRING(REPLACE "//" "/"  QT_VERSION "${QT_VERSION}")
		MESSAGE("-- MSVC version reported as ${MSVC_VERSION}")
		MATH(EXPR QT_MSVC "2000 + (${MSVC_VERSION} - 600) / 100")
		IF(QT_MSVC STREQUAL "2013")
			SET(QT_MSVC "2015")
		ENDIF()
		IF(CMAKE_SYSTEM_PROCESSOR MATCHES 64)
			SET(QT_MSVC "${QT_MSVC}_64")
		ENDIF()
		SET(QT_PATH "${QT_VERSION}/msvc${QT_MSVC}")
		SET(QT_MISSING False)
	ELSE()
		MESSAGE("-- QtCreator does not appear to be installed")
	ENDIF()
ENDIF()
