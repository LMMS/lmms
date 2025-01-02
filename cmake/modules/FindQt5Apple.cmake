# Attempts to set Qt5_DIR, Qt5Test_DIR by looking for a valid Qt5 install
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# macOS:
#     [x] homebrew:  /usr/local/Cellar/qt5/5.x.x/
#     [ ] macports:  /opt/local/libexec/qt5/bin/qmake
#     [x] qtcreator: $HOME/Qt/5.x.x/clang_64/

# homebrew
EXECUTE_PROCESS(
	COMMAND brew --version
	RESULT_VARIABLE BREW_MISSING
	OUTPUT_QUIET ERROR_QUIET
)

IF(NOT BREW_MISSING)
	EXECUTE_PROCESS(
		COMMAND brew --prefix qt5
		OUTPUT_VARIABLE QT_PATH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE QT_MISSING
	)
	IF(NOT QT_MISSING)
		RETURN()
	ENDIF()

	# also try qt@5.5
	EXECUTE_PROCESS(
		COMMAND brew --prefix qt@5.5
		OUTPUT_VARIABLE QT_PATH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE QT_MISSING
	)
	RETURN()
ENDIF()

# qtcreator + mac
EXECUTE_PROCESS(
	COMMAND /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister
	-dump OUTPUT_VARIABLE LSREGISTER_RAW
	RESULT_VARIABLE LSREGISTER_MISSING
)
IF(NOT LSREGISTER_MISSING)
	STRING(REGEX MATCHALL "path:( *)[^\n]*Qt Creator.app\n" LSREGISTER_LIST "${LSREGISTER_RAW}")
	LIST(LENGTH LSREGISTER_LIST QT_CREATOR_FOUND)
	IF(QT_CREATOR_FOUND)
		LIST(SORT LSREGISTER_LIST)
		LIST(GET LSREGISTER_LIST 0 QT_ROOT)
		STRING(REGEX REPLACE "path:( *)" "" QT_ROOT "${QT_ROOT}")
		GET_FILENAME_COMPONENT(QT_ROOT "${QT_ROOT}" DIRECTORY)
		STRING(REPLACE "Qt" ";" VERSION_PARTS "${QT_ROOT}")
		LIST(REVERSE VERSION_PARTS)
		LIST(GET VERSION_PARTS 0 QT_VERSION)
		SET(QT_PATH "${QT_ROOT}/${QT_VERSION}/clang_64")
		SET(QT_MISSING False)
	ENDIF()
	RETURN()
ENDIF()
