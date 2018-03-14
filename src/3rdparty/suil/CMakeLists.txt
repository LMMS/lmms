add_library(suil STATIC
	suil/suil/suil.h
	suil/src/gtk2_in_qt5.cpp
	suil/src/host.c
	suil/src/instance.c
	suil/src/suil_internal.h
	suil/src/x11.c
	suil/src/x11_in_qt5.cpp
)

target_include_directories(suil PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/suil
	${CMAKE_CURRENT_SOURCE_DIR}/suil/build
	${GTK2_INCLUDE_DIRS}
)

target_link_libraries(suil
	${GTK2_LIBRARIES}
)

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
