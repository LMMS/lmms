FIND_PACKAGE(Git)
IF(GIT_FOUND AND NOT FORCE_VERSION)
	SET(MAJOR_VERSION 0)
	SET(MINOR_VERSION 0)
	SET(PATCH_VERSION 0)
	# Look for git tag information (e.g. Tagged: "v1.0.0", Non-tagged: "v1.0.0-123-a1b2c3d")
	# Non tagged format: [latest tag]-[number of commits]-[latest commit hash]
	EXECUTE_PROCESS(
		COMMAND "${GIT_EXECUTABLE}" describe --tags --match v[0-9].[0-9].[0-9]*
		OUTPUT_VARIABLE GIT_TAG
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		TIMEOUT 10
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	# Read: TAG_LIST = GIT_TAG.split("-")
	STRING(REPLACE "-" ";" TAG_LIST "${GIT_TAG}")
	# Read: TAG_LIST_LENGTH = TAG_LIST.length()
	LIST(LENGTH TAG_LIST TAG_LIST_LENGTH)
	# Non-tagged versions contain at least 2 dashes, and 2 dashes gives 3 strings
	# so we know that TAG_LIST_LENGTH = [dashes in latest tag] + 3.
	IF(TAG_LIST_LENGTH GREATER 0)
		# Set FORCE_VERSION to TAG_LIST[0], strip the initial "v" to get MAJ.MIN.PAT
		LIST(GET TAG_LIST 0 FORCE_VERSION)
		STRING(REPLACE "v" "" FORCE_VERSION "${FORCE_VERSION}")
		# Split FORCE_VERSION on "." and populate MAJOR/MINOR/PATCH_VERSION
		STRING(REPLACE "." ";" MAJ_MIN_PAT "${FORCE_VERSION}")
		LIST(GET MAJ_MIN_PAT 0 MAJOR_VERSION)
		LIST(GET MAJ_MIN_PAT 1 MINOR_VERSION)
		LIST(GET MAJ_MIN_PAT 2 PATCH_VERSION)
	ENDIF()
	# 1 dash total => prerelease with no additional commits
	IF(TAG_LIST_LENGTH EQUAL 2)
		LIST(GET TAG_LIST 1 VERSION_STAGE)
		SET(FORCE_VERSION "${FORCE_VERSION}-${VERSION_STAGE}")
	# 0 dashes in latest tag => stable release with additional commits
	# For example, 1.2.2 + 50 commits
	ELSEIF(TAG_LIST_LENGTH EQUAL 3)
		# Get the number of commits and latest commit hash
		LIST(GET TAG_LIST 1 EXTRA_COMMITS)
		LIST(GET TAG_LIST 2 COMMIT_HASH)
		# Bump the patch version
		MATH(EXPR PATCH_VERSION "${PATCH_VERSION}+1")
		# Set the version to MAJOR.MINOR.PATCH-EXTRA_COMMITS+COMMIT_HASH
		SET(FORCE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
		SET(FORCE_VERSION "${FORCE_VERSION}-${EXTRA_COMMITS}+${COMMIT_HASH}")
	# 1 dash in latest tag => pre-release with additional commits
	# For example, 1.3.0-alpha.1 with an additional 50 commits
	ELSEIF(TAG_LIST_LENGTH EQUAL 4)
		# Get prerelease stage, number of commits, and latest commit hash
		LIST(GET TAG_LIST 1 VERSION_STAGE)
		LIST(GET TAG_LIST 2 EXTRA_COMMITS)
		LIST(GET TAG_LIST 3 COMMIT_HASH)
		# Set the version to MAJOR.MINOR.PATCH-VERSION_STAGE.EXTRA_COMMITS+COMMIT_HASH
		SET(FORCE_VERSION "${FORCE_VERSION}-${VERSION_STAGE}")
		SET(FORCE_VERSION "${FORCE_VERSION}.${EXTRA_COMMITS}+${COMMIT_HASH}")
	ENDIF()
ENDIF()

IF(FORCE_VERSION STREQUAL "internal")
	# Use release info from /CMakeLists.txt
ELSEIF(FORCE_VERSION)
	STRING(REPLACE "." ";" VERSION_LIST "${FORCE_VERSION}")
	LIST(LENGTH VERSION_LIST VERSION_LENGTH)
	LIST(GET VERSION_LIST 0 VERSION_MAJOR)
	LIST(GET VERSION_LIST 1 VERSION_MINOR)
	LIST(GET VERSION_LIST 2 VERSION_RELEASE)
	SET(VERSION_STAGE "")
	SET(VERSION_BUILD 0)
	IF(VERSION_LENGTH GREATER 3)
		LIST(GET VERSION_LIST 3 VERSION_BUILD)
	ENDIF()

	STRING(REPLACE "-" ";" VERSION_LIST "${VERSION_RELEASE}")
	LIST(LENGTH VERSION_LIST VERSION_LENGTH)
	IF(VERSION_LENGTH GREATER 1)
		LIST(GET VERSION_LIST 0 VERSION_RELEASE)
		LIST(GET VERSION_LIST 1 VERSION_STAGE)
	ENDIF()

	SET(VERSION             "${FORCE_VERSION}")
ELSEIF(GIT_FOUND)
	MESSAGE(
"Could not get project version.  Using release info from /CMakeLists.txt"
	)
ELSE()
	MESSAGE("Git not found.  Using release info from /CMakeLists.txt")
ENDIF()



MESSAGE("\n"
	"Configuring ${PROJECT_NAME_UCASE}\n"
	"--------------------------\n"
	"* Project version             : ${VERSION}\n"
	"*   Major version             : ${VERSION_MAJOR}\n"
	"*   Minor version             : ${VERSION_MINOR}\n"
	"*   Release version           : ${VERSION_RELEASE}\n"
	"*   Stage version             : ${VERSION_STAGE}\n"
	"*   Build version             : ${VERSION_BUILD}\n"
        "*\n\n"
	"Optional Version Usage:\n"
	"--------------------------\n"
	"*   Override version:           -DFORCE_VERSION=x.x.x-x\n"
	"*   Ignore Git information:     -DFORCE_VERSION=internal\n"
)
