# If Carla was not provided by the system, make a dummy library instead
if(LMMS_HAVE_WEAKCARLA)
  SET(CARLA_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/backend
  )

  # force "lib" prefix
  IF(LMMS_BUILD_WIN32)
      SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")
  ENDIF()

  SET(CARLA_NATIVE_LIB carla_native-plugin)

  ADD_LIBRARY(${CARLA_NATIVE_LIB} SHARED DummyCarla.cpp)
  target_include_directories(${CARLA_NATIVE_LIB} SYSTEM PUBLIC ${CARLA_INCLUDE_DIRS})
  INSTALL(TARGETS ${CARLA_NATIVE_LIB}
    LIBRARY DESTINATION "${PLUGIN_DIR}/optional"
    RUNTIME DESTINATION "${PLUGIN_DIR}/optional"
  )
  SET(CARLA_LIBRARIES ${CARLA_NATIVE_LIB})
  # Set parent scope variables so carlarack and carlapatchbay can see them
  SET(CARLA_LIBRARIES ${CARLA_LIBRARIES} PARENT_SCOPE)
endif()

if(LMMS_USE_BUNDLED_CARLA)
  set(CARLA_BUNDLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/carla")
  set(CARLA_BUNDLED_BIN_DIR "${CARLA_BUNDLED_DIR}/bin")
  set(CARLA_BUNDLED_RESOURCES_DIR "${CARLA_BUNDLED_BIN_DIR}/resources")
  add_subdirectory("${CARLA_BUNDLED_DIR}/cmake" "${CMAKE_CURRENT_BINARY_DIR}/carla" EXCLUDE_FROM_ALL)

  set(CARLA_INCLUDE_DIRS
    ${CARLA_BUNDLED_DIR}/source
    ${CARLA_BUNDLED_DIR}/source/includes
    ${CARLA_BUNDLED_DIR}/source/utils
    ${CARLA_BUNDLED_DIR}/source/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/CarlaConfig
  )
  set(CARLA_LIBRARIES carla-native-plugin)
  set(CARLA_LIBRARY_DIRS "${CMAKE_BINARY_DIR}/plugins")
  set(CARLA_RPATH "${CMAKE_BINARY_DIR}/plugins")

  set(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} PARENT_SCOPE)
  set(CARLA_LIBRARIES ${CARLA_LIBRARIES} PARENT_SCOPE)
  set(CARLA_LIBRARY_DIRS ${CARLA_LIBRARY_DIRS} PARENT_SCOPE)
  set(CARLA_RPATH ${CARLA_RPATH} PARENT_SCOPE)

  if(NOT LMMS_CARLA_PYTHON_EXECUTABLE)
    message(FATAL_ERROR "LMMS_CARLA_PYTHON_EXECUTABLE is required for bundled Carla")
  endif()

  add_custom_command(
    OUTPUT "${CARLA_BUNDLED_BIN_DIR}/libcarla_frontend.so"
           "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin"
           "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin-patchbay"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CARLA_BUNDLED_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/plugins/libcarla_utils.so"
            "${CARLA_BUNDLED_BIN_DIR}/libcarla_utils.so"
    COMMAND ${CMAKE_COMMAND} -E env
            "LMMS_CARLA_PYTHON_EXECUTABLE=${LMMS_CARLA_PYTHON_EXECUTABLE}"
            ${CMAKE_MAKE_PROGRAM} clean
    COMMAND ${CMAKE_COMMAND} -E env
            "LMMS_CARLA_PYTHON_EXECUTABLE=${LMMS_CARLA_PYTHON_EXECUTABLE}"
            ${CMAKE_MAKE_PROGRAM}
    WORKING_DIRECTORY "${CARLA_BUNDLED_DIR}/source/frontend"
    DEPENDS carla-utils
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CARLA_BUNDLED_RESOURCES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/common"
            "${CARLA_BUNDLED_RESOURCES_DIR}/common"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/dialogs"
            "${CARLA_BUNDLED_RESOURCES_DIR}/dialogs"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/modgui"
            "${CARLA_BUNDLED_RESOURCES_DIR}/modgui"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/patchcanvas"
            "${CARLA_BUNDLED_RESOURCES_DIR}/patchcanvas"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/utils"
            "${CARLA_BUNDLED_RESOURCES_DIR}/utils"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CARLA_BUNDLED_DIR}/source/frontend/widgets"
            "${CARLA_BUNDLED_RESOURCES_DIR}/widgets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla-plugin"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla-plugin-patchbay"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin-patchbay"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/bigmeter-ui"
            "${CARLA_BUNDLED_RESOURCES_DIR}/bigmeter-ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/midipattern-ui"
            "${CARLA_BUNDLED_RESOURCES_DIR}/midipattern-ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/notes-ui"
            "${CARLA_BUNDLED_RESOURCES_DIR}/notes-ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/xycontroller-ui"
            "${CARLA_BUNDLED_RESOURCES_DIR}/xycontroller-ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_app.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_app.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_backend.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_backend.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_backend_qt.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_backend_qt.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_backend_qtweb.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_backend_qtweb.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_frontend.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_frontend.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_host.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_host.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_host_control.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_host_control.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_modgui.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_modgui.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_settings.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_settings.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_shared.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_shared.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_skin.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_skin.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_utils.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_utils.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/carla_widgets.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla_widgets.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/externalui.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/externalui.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ladspa_rdf.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ladspa_rdf.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/resources_rc.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/resources_rc.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_about.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_about.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_edit.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_edit.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_host.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_host.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_osc_connect.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_osc_connect.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_parameter.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_parameter.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_plugin_calf.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_plugin_calf.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_plugin_classic.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_plugin_classic.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_plugin_compact.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_plugin_compact.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_plugin_default.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_plugin_default.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_plugin_presets.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_plugin_presets.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_settings.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_settings.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_carla_settings_driver.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_carla_settings_driver.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_inputdialog_value.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_inputdialog_value.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_midipattern.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_midipattern.py"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CARLA_BUNDLED_DIR}/source/frontend/ui_xycontroller.py"
            "${CARLA_BUNDLED_RESOURCES_DIR}/ui_xycontroller.py"
    COMMAND sed -i "1s|.*|#!${LMMS_CARLA_PYTHON_EXECUTABLE}|"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin"
    COMMAND sed -i "1s|.*|#!${LMMS_CARLA_PYTHON_EXECUTABLE}|"
            "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin-patchbay"
    COMMENT "Building bundled Carla frontend resources"
    VERBATIM
  )

  add_custom_target(carla-bundled-frontend
    DEPENDS
      "${CARLA_BUNDLED_BIN_DIR}/libcarla_frontend.so"
      "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin"
      "${CARLA_BUNDLED_RESOURCES_DIR}/carla-plugin-patchbay"
  )
endif()

if(LMMS_HAVE_CARLA OR LMMS_HAVE_WEAKCARLA)
        # Mimic the missing "config.h"
        SET(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/CarlaConfig)
        SET(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} PARENT_SCOPE)

        INCLUDE(BuildPlugin)
        include_directories(SYSTEM ${CARLA_INCLUDE_DIRS})
        LINK_DIRECTORIES(${CARLA_LIBRARY_DIRS})
        LINK_LIBRARIES(${CARLA_LIBRARIES})
        BUILD_PLUGIN(carlabase Carla.cpp Carla.h
            MOCFILES Carla.h
            EMBEDDED_RESOURCES artwork-patchbay.png artwork-rack.png
            EXPORT_BASE_NAME carlabase
            LINK SHARED)
        if(LMMS_USE_BUNDLED_CARLA)
                add_dependencies(carlabase carla-utils carla-bundled-frontend)
                target_compile_definitions(carlabase PRIVATE LMMS_CARLA_RESOURCES_PATH="${CARLA_BUNDLED_RESOURCES_DIR}")
        endif()
        SET_TARGET_PROPERTIES(carlabase
                PROPERTIES SKIP_BUILD_RPATH TRUE
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH_USE_LINK_PATH TRUE
                INSTALL_RPATH "${CARLA_RPATH}")
        IF(LMMS_HAVE_WEAKCARLA)
                ADD_DEPENDENCIES(carlabase ${CARLA_NATIVE_LIB})
        ENDIF()
endif()
