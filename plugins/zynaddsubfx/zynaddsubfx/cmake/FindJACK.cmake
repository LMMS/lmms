#Find JACK Audio Connection Kit

include(LibFindMacros)
libfind_pkg_check_modules(JACK jack)
find_path(JACK_INCLUDE_DIR
    NAMES jack/jack.h
    PATHS ${JACK_INCLUDE_DIRS}
    )

find_library(JACK_LIBRARY
    NAMES jack
    PATHS ${JACK_LIBRARY_DIRS}
    )

set(JACK_PROCESS_INCLUDES JACK_INCLUDE_DIR)
set(JACK_PROCESS_LIBS JACK_LIBRARY)
libfind_process(JACK)
