if (QT5)
	set (QT_LUPDATE_EXECUTABLE "${Qt5_LUPDATE_EXECUTABLE}")
	set (QT_LRELEASE_EXECUTABLE "${Qt5_LRELEASE_EXECUTABLE}")
endif ()

#
# rules for building localizations
#
SET(ts_targets "")
SET(qm_targets "")
SET(QM_FILES "")

FILE(GLOB lmms_LOCALES ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
FOREACH(_ts_file ${lmms_LOCALES})
	GET_FILENAME_COMPONENT(_ts_target "${_ts_file}" NAME)
	STRING(REPLACE ".ts" ".qm" _qm_file "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_target "${_ts_target}")
	ADD_CUSTOM_TARGET(${_ts_target}
		COMMAND "${QT_LUPDATE_EXECUTABLE}" -locations none -no-obsolete -I ${CMAKE_SOURCE_DIR}/include/ ${LMMS_SRCS} ${LMMS_INCLUDES} ${LMMS_UIS} `find "\"${CMAKE_SOURCE_DIR}/plugins/\"" -type f -name '*.cpp' -or -name '*.h'` -ts "\"${_ts_file}\""
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	ADD_CUSTOM_TARGET(${_qm_target}
		COMMAND "${QT_LRELEASE_EXECUTABLE}" "\"${_ts_file}\"" -qm "\"${_qm_file}\""
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	LIST(APPEND ts_targets "${_ts_target}")
	LIST(APPEND qm_targets "${_qm_target}")
	LIST(APPEND QM_FILES "${_qm_file}")
ENDFOREACH(_ts_file ${lmms_LOCALES})

ADD_CUSTOM_TARGET(update-locales)
FOREACH(_item ${ts_targets})
	ADD_DEPENDENCIES(update-locales "${_item}")
ENDFOREACH(_item ${ts_targets})

ADD_CUSTOM_TARGET(finalize-locales ALL)
FOREACH(_item ${qm_targets})
	ADD_DEPENDENCIES(finalize-locales "${_item}")
ENDFOREACH(_item ${qm_targets})

IF(LMMS_BUILD_WIN32)
	FILE(GLOB QT_QM_FILES "${QT_TRANSLATIONS_DIR}/qt*[^h].qm")
ENDIF(LMMS_BUILD_WIN32)

INSTALL(FILES ${QM_FILES} ${QT_QM_FILES} DESTINATION "${LMMS_DATA_DIR}/locale")
