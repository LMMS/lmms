# Adds two macros to be called before and after locale-specific cmake commands
# Supported platforms: Linux, macOS
#
# Copyright (c) 2017, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Usage:
#    INCLUDE(EnglishLocale)
#    USE_ENGLISH_LOCALE()
#    EXECUTE_PROCESS(...)
#    USE_DEFAULT_LOCALE()

MACRO(USE_ENGLISH_LOCALE)
	SET(LC_ALL_BACKUP "$ENV{LC_ALL}")
	SET(LANG_BACKUP "$ENV{LANG}")
	SET(ENV{LC_ALL} "C")
	SET(ENV{LANG} "en_US")
ENDMACRO()

MACRO(USE_DEFAULT_LOCALE)
	SET(ENV{LC_ALL} "${LC_ALL_BACKUP}")
	SET(ENV{LANG} "${LANG_BACKUP}")
ENDMACRO()
