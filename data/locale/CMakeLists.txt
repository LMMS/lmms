SET(QT_LUPDATE_EXECUTABLE "${Qt${QT_VERSION_MAJOR}_LUPDATE_EXECUTABLE}")
SET(QT_LRELEASE_EXECUTABLE "${Qt${QT_VERSION_MAJOR}_LRELEASE_EXECUTABLE}")

IF(QT_LUPDATE_EXECUTABLE STREQUAL "")
	EXECUTE_PROCESS(COMMAND "lupdate" "-help" RESULT_VARIABLE LUPDATE_FALLBACK OUTPUT_QUIET)
	IF(LUPDATE_FALLBACK EQUAL 0)
        	SET(QT_LUPDATE_EXECUTABLE lupdate)
        	SET(QT_LRELEASE_EXECUTABLE lrelease)
	ELSE()
		MESSAGE(WARNING "Cannot generate locales")
		RETURN()
	ENDIF()
ENDIF()


#
# rules for building localizations
#
SET(ts_targets "")
SET(qm_targets "")
SET(QM_FILES "")

FILE(GLOB lmms_LOCALES ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
LIST(SORT lmms_LOCALES)
FOREACH(_ts_file ${lmms_LOCALES})
	GET_FILENAME_COMPONENT(_ts_target "${_ts_file}" NAME)
	STRING(REPLACE ".ts" ".qm" _qm_file "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_target "${_ts_target}")
	ADD_CUSTOM_TARGET(${_ts_target}
		COMMAND "${QT_LUPDATE_EXECUTABLE}" -locations none -no-obsolete -I ${CMAKE_SOURCE_DIR}/include/ ${LMMS_SRCS}  ${LMMS_UIS} ${CMAKE_SOURCE_DIR}/plugins  -ts "\"${_ts_file}\""
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	add_custom_command(
		OUTPUT "${_qm_file}"
		COMMAND "${QT_LRELEASE_EXECUTABLE}" "${_ts_file}" -qm "${_qm_file}"
		MAIN_DEPENDENCY "${_ts_file}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		VERBATIM
	)
	add_custom_target("${_qm_target}" DEPENDS "${_qm_file}")
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

IF(BUNDLE_QT_TRANSLATIONS)
	FILE(GLOB QT_QM_FILES "${QT_TRANSLATIONS_DIR}/qt*.qm")
	LIST(SORT QT_QM_FILES)
ENDIF()

INSTALL(FILES ${QM_FILES} ${QT_QM_FILES} DESTINATION "${LMMS_DATA_DIR}/locale")
