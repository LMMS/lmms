
macro(CHECK_CXX_PREPROCESSOR VAR DIRECTIVE)
	string(RANDOM DEFINED_KEY)
	string(RANDOM UNDEFINED_KEY)

	set(TMP_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/CxxTmp/src.cpp")
	SET(SRC "
		#if ${DIRECTIVE}
		#error ${DEFINED_KEY}
		#else
		#error ${UNDEFINED_KEY}
		#endif
		")
	file(WRITE ${TMP_FILENAME} "${SRC}")
	try_compile(RESULT_VAR
		${CMAKE_CURRENT_BINARY_DIR}
		${TMP_FILENAME}
		OUTPUT_VARIABLE OUTPUT_VAR
	)

	if(OUTPUT_VAR MATCHES ${DEFINED_KEY})
		set(${VAR} ON)
	elseif(OUTPUT_VAR MATCHES ${UNDEFINED_KEY})
		set(${VAR} OFF)
	else()
		message(FATAL_ERROR "Can't determine if \"${DIRECTIVE}\" is true.")
	endif()
endmacro()


macro(CHECK_CXX_DEFINE VAR DEFINE)
	CHECK_CXX_PREPROCESSOR(${VAR} "defined(${DEFINE})")
endmacro()
