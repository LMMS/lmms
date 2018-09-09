set(LMMS_SRCS
	${LMMS_SRCS}
	core/lv2/control.cpp
  core/lv2/log.cpp
  core/lv2/lv2_evbuf.c
  core/lv2/Lva2Instrument.cpp
  core/lv2/Lv2Plugin.cpp
  core/lv2/state.cpp
  core/lv2/symap.c
  core/lv2/worker.cpp
  core/lv2/zix/ring.c

	PARENT_SCOPE
)
#FILE(GLOB LV2_INTERNALS_SOURCES ./*.h ./*.c ./*.cpp **/*.h **/*.c **/*.cpp)
#add_library(lv2internals STATIC
    #${LV2_INTERNALS_SOURCES}
#)

#target_include_directories(lv2internals PUBLIC
    #${CMAKE_CURRENT_SOURCE_DIR}
    #${CMAKE_CURRENT_SOURCE_DIR}/../../include
    #${CMAKE_SOURCE_DIR}
    #${LILV_INCLUDE_DIRS}
    #${SUIL_INCLUDE_DIRS}
    #${LV2_INCLUDE_DIRS}
#)

#target_link_libraries(lv2internals
  #${SUIL_LIBRARIES}
  #${LILV_LIBRARIES}
  #${LV2_LIBRARIES}
#)

#set_target_properties(lv2internals PROPERTIES LINKER_LANGUAGE CXX)
#INCLUDE(BuildPlugin)

## Disable C++11
##REMOVE_DEFINITIONS(-std=c++0x)
## Enable C++11 for CXXFLAGS only and not for Windows
##IF(NOT LMMS_BUILD_WIN32)
	##SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
##ENDIF()

#INCLUDE_DIRECTORIES(
    #${CMAKE_CURRENT_SOURCE_DIR}
    #${LILV_INCLUDE_DIRS}
    #${LV2_INCLUDE_DIRS}
    #${SUIL_INCLUDE_DIRS}
#)

#LINK_DIRECTORIES(
  #${SUIL_LIBRARIES}
  #${LV2_LIBRARIES}
  #${LILV_LIBRARIES}
#)

#BUILD_PLUGIN(lv2
  #control.cpp control.h
  #log.cpp log.h
  #lv2_evbuf.c lv2_evbuf.h
  #Lv2Instrument.cpp Lv2Instrument.h
  #Lv2Plugin.cpp Lv2Plugin.h
  #state.cpp
  #symap.c symap.h
  #worker.cpp worker.h
  #zix/common.h
  #zix/ring.c zix/ring.h
  #zix/sem.h
  #zix/thread.h
  ##EMBEDDED_RESOURCES
  ##"${CMAKE_CURRENT_SOURCE_DIR}/*.png"
  #)

##SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lv2")
