FIND_PACKAGE(Git)
IF(GIT_FOUND AND NOT FORCE_VERSION)
	SET(MAJOR_VERSION 0)
	SET(MINOR_VERSION 0)
	SET(PATCH_VERSION 0)

	# If this is a GitHub Actions pull request build, get the pull request
	# number from the environment and add it to the build metadata
	if("$ENV{GITHUB_REF}" MATCHES "refs/pull/([0-9]+)/merge")
		list(APPEND BUILD_METADATA "pr${CMAKE_MATCH_1}")
		# Parse hash from merge description
		# e.g. "Merge abc1234 into def4567"
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" log -n 1 --format=%s
			OUTPUT_VARIABLE COMMIT_HASH
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			TIMEOUT 10
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		# If successful, use the first 7 characters to mimic github's hash style
		if(COMMIT_HASH)
			string(SUBSTRING "${COMMIT_HASH}" 6 7 COMMIT_HASH)
		endif()
	endif()

	# Look for git tag information (e.g. Tagged: "v1.0.0", Untagged: "v1.0.0-123-a1b2c3d")
	# Untagged format: [latest tag]-[number of commits]-[latest commit hash]
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
	# Untagged versions contain at least 2 dashes, giving 3 strings on split.
	# Hence, for untagged versions TAG_LIST_LENGTH = [dashes in latest tag] + 3.
	# Corollary: if TAG_LIST_LENGTH <= 2, the version must be tagged.
	IF(TAG_LIST_LENGTH GREATER 0)
		# Set FORCE_VERSION to TAG_LIST[0], strip any 'v's to get MAJ.MIN.PAT
		LIST(GET TAG_LIST 0 FORCE_VERSION)
		STRING(REPLACE "v" "" FORCE_VERSION "${FORCE_VERSION}")
		# Split FORCE_VERSION on '.' and populate MAJOR/MINOR/PATCH_VERSION
		STRING(REPLACE "." ";" MAJ_MIN_PAT "${FORCE_VERSION}")
		LIST(GET MAJ_MIN_PAT 0 MAJOR_VERSION)
		LIST(GET MAJ_MIN_PAT 1 MINOR_VERSION)
		LIST(GET MAJ_MIN_PAT 2 PATCH_VERSION)
	ENDIF()
	# 1 dash total: Dash in latest tag, no additional commits => pre-release
	IF(TAG_LIST_LENGTH EQUAL 2)
		# Get the pre-release stage
		LIST(GET TAG_LIST 1 VERSION_STAGE)
		list(APPEND PRERELEASE_DATA "${VERSION_STAGE}")
	# 2 dashes: Assume untagged with no dashes in latest tag name => stable + commits
	ELSEIF(TAG_LIST_LENGTH EQUAL 3)
		# Get the number of commits and latest commit hash
		LIST(GET TAG_LIST 1 EXTRA_COMMITS)
		# Prefer PR hash from above if present
		if(NOT COMMIT_HASH)
			list(GET TAG_LIST 2 COMMIT_HASH)
			# Mimic github's hash style
			string(SUBSTRING "${COMMIT_HASH}" 1 7 COMMIT_HASH)
		endif()
		list(APPEND PRERELEASE_DATA "${EXTRA_COMMITS}")
		list(APPEND BUILD_METADATA "${COMMIT_HASH}")
		# Bump the patch version, since a pre-release (as specified by the extra
		# commits) compares lower than the main version alone
		MATH(EXPR PATCH_VERSION "${PATCH_VERSION}+1")
		# Reassemble the main version using the new patch version
		set(FORCE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
	# 3 dashes: Assume untagged with 1 dash in latest tag name => pre-release + commits
	ELSEIF(TAG_LIST_LENGTH EQUAL 4)
		# Get the pre-release stage, number of commits, and latest commit hash
		LIST(GET TAG_LIST 1 VERSION_STAGE)
		LIST(GET TAG_LIST 2 EXTRA_COMMITS)
		# Prefer PR hash from above if present
        if(NOT COMMIT_HASH)
			list(GET TAG_LIST 3 COMMIT_HASH)
			# Mimic github's hash style
			string(SUBSTRING "${COMMIT_HASH}" 1 7 COMMIT_HASH)
		endif()
		list(APPEND PRERELEASE_DATA "${VERSION_STAGE}")
		list(APPEND PRERELEASE_DATA "${EXTRA_COMMITS}")
		list(APPEND BUILD_METADATA "${COMMIT_HASH}")
	ENDIF()

	# If there is any pre-release data, append it after a hyphen
	if(PRERELEASE_DATA)
		string(REPLACE ";" "." PRERELEASE_DATA "${PRERELEASE_DATA}")
		set(FORCE_VERSION "${FORCE_VERSION}-${PRERELEASE_DATA}")
	endif()

	# If there is any build metadata, append it after a plus
	if(BUILD_METADATA)
		string(REPLACE ";" "." BUILD_METADATA "${BUILD_METADATA}")
		set(FORCE_VERSION "${FORCE_VERSION}+${BUILD_METADATA}")
	endif()
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
