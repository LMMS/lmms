# A wrapper around platform-dependent FindQt5 scripts
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Each FindQt5<Platform> script must set:
# - QT_DIR: A valid Qt5 install location
# - QT_MISSING: False to indicate Qt5 has been found
#
# Sets the following cmake variables to be used by FIND_PACKAGE:
# - Qt5_DIR     <CURRENT_SCOPE>
# - Qt5Test_DIR <CURRENT_SCOPE>
FIND_PACKAGE(Qt5Core QUIET)
IF(NOT Qt5Core_FOUND)
	INCLUDE(EnglishLocale)
	USE_ENGLISH_LOCALE()

	SET(QT_MISSING True)

	IF(APPLE)
		INCLUDE(FindQt5Apple)
	ELSEIF(MSVC)
		INCLUDE(FindQt5Windows)
	ELSE()
		INCLUDE(FindQt5Linux)
	ENDIF()

	USE_DEFAULT_LOCALE()
	IF(NOT QT_MISSING)
		MESSAGE("-- Qt found: ${QT_PATH}")
		SET(Qt5_DIR "${QT_PATH}/lib/cmake/Qt5/")
		SET(Qt5Test_DIR "${QT_PATH}/lib/cmake/Qt5Test")
	ENDIF()
ENDIF()
