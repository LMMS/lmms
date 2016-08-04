FIND_PACKAGE(Git)
IF(GIT_FOUND AND NOT FORCE_VERSION)
	# Look for git tag information (e.g. Stable: "v1.0.0", Non-stable: "v1.0.0-123-a1b2c3d4")
	EXECUTE_PROCESS(
		COMMAND "${GIT_EXECUTABLE}" describe --tags --match v[0-9].[0-9].[0-9]*
		OUTPUT_VARIABLE GIT_TAG
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		TIMEOUT 1
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REPLACE "-" ";" TAG_LIST "${GIT_TAG}")
	LIST(LENGTH TAG_LIST TAG_LIST_LENGTH)
	IF(TAG_LIST_LENGTH EQUAL 1)
		# Stable build, FORCE_VERSION=x.x.x
		LIST(GET TAG_LIST 0 FORCE_VERSION)
		STRING(REPLACE "v" "" FORCE_VERSION "${FORCE_VERSION}")
	ELSEIF(TAG_LIST_LENGTH EQUAL 3)
		# Non-stable build, FORCE_VERSION=x.x.x-hash
		LIST(GET TAG_LIST 0 FORCE_VERSION)
		LIST(GET TAG_LIST 2 COMMIT_HASH)
		STRING(REPLACE "v" "" FORCE_VERSION "${FORCE_VERSION}")
		STRING(REPLACE "g" "" COMMIT_HASH "${COMMIT_HASH}")
		SET(FORCE_VERSION "${FORCE_VERSION}-${COMMIT_HASH}")
	ENDIF()
ENDIF()

IF(FORCE_VERSION STREQUAL "internal")
	# Use release info from /CMakeLists.txt
ELSEIF(FORCE_VERSION)
	STRING(REPLACE "." ";" VERSION_LIST "${FORCE_VERSION}")
	STRING(REPLACE "-" ";" VERSION_LIST "${VERSION_LIST}")
	LIST(LENGTH VERSION_LIST VERSION_LENGTH)
	LIST(GET VERSION_LIST 0 VERSION_MAJOR)
	LIST(GET VERSION_LIST 1 VERSION_MINOR)
	LIST(GET VERSION_LIST 2 VERSION_PATCH)
	IF(VERSION_LENGTH GREATER 3)
		LIST(GET VERSION_LIST 3 VERSION_SUFFIX)
	ENDIF()
	SET(VERSION             "${FORCE_VERSION}")
ELSE()
	MESSAGE("Git not found.  Using release info from /CMakeLists.txt")
ENDIF()



MESSAGE("\n"
	"Configuring ${PROJECT_NAME_UCASE}\n"
	"--------------------------\n"
	"* Build version               : ${VERSION}\n"
	"*   Major version             : ${VERSION_MAJOR}\n"
	"*   Minor version             : ${VERSION_MINOR}\n"
	"*   Patch version             : ${VERSION_PATCH}\n"
	"*   Suffix version            : ${VERSION_SUFFIX}\n"
        "*\n\n"
	"Optional Version Usage:\n"
	"--------------------------\n"
	"*   Override version:           -DFORCE_VERSION=x.x.x-x\n"
	"*   Disable hash suffix:        -DFORCE_VERSION=internal\n"
)

