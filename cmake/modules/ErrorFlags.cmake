# Shim the SYSTEM property for older CMake versions - we rely on this property
# to determine which set of error flags to use.
if(CMAKE_VERSION VERSION_LESS "3.25")
	define_property(TARGET
		PROPERTY SYSTEM
		INHERITED
		BRIEF_DOCS "Shim of built-in SYSTEM property for CMake versions less than 3.25"
		FULL_DOCS "Non-functional, but allows the property to be inherited properly."
			"See the CMake documentation at https://cmake.org/cmake/help/latest/prop_tgt/SYSTEM.html."
	)
endif()

# Allow the user to control whether to treat warnings as errors
option(USE_WERROR "Treat compiler warnings as errors" OFF)

# Compute the appropriate flags for the current compiler and options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	set(COMPILE_ERROR_FLAGS
		"-Wall" # Enable most warnings by default
	)
	set(THIRD_PARTY_COMPILE_ERROR_FLAGS
		"-w" # Disable all warnings
	)

	if(CMAKE_COMPILER_IS_GNUCXX)
		list(APPEND COMPILE_ERROR_FLAGS
			# The following warning generates false positives that are difficult
			# to work around, in particular when inlining calls to standard
			# algorithms performed on single-element arrays. See, for example,
			# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111273.
			"-Wno-array-bounds" # Permit out-of-bounds array subscripts
		)

		if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "11")
			list(APPEND COMPILE_ERROR_FLAGS
				# This has the same problems described above for "array-bounds".
				"-Wno-stringop-overread" # Permit string functions overreading the source
			)
		endif()
	endif()

	if(USE_WERROR)
		list(APPEND COMPILE_ERROR_FLAGS
			"-Werror" # Treat warnings as errors
		)
	endif()
elseif(MSVC)
	set(COMPILE_ERROR_FLAGS
		"/W2" # Enable some warnings by default
		"/external:W0" # Don't emit warnings for third-party code
		"/external:anglebrackets" # Consider headers included with angle brackets to be third-party
		"/external:templates-" # Still emit warnings from first-party instantiations of third-party templates
		# Silence "class X needs to have DLL-interface to be used by clients of
		# class Y" warnings. These aren't trivial to address, and don't pose a
		# problem for us since we build all modules with the same compiler and
		# options, and dynamically link the CRT.
		"/wd4251"
	)
	set(THIRD_PARTY_COMPILE_ERROR_FLAGS
		"/W0" # Disable all warnings
	)

	if(USE_WERROR)
		list(APPEND COMPILE_ERROR_FLAGS
			"/WX" # Treat warnings as errors
		)
	endif()

	# Silence deprecation warnings for the std::atomic_...<std::shared_ptr> family of functions.
	# TODO: Remove once C++20's std::atomic<std::shared_ptr> is fully supported.
	add_compile_definitions("_SILENCE_CXX20_OLD_SHARED_PTR_ATOMIC_SUPPORT_DEPRECATION_WARNING")
endif()

# Add the flags to the whole directory tree. We use the third-party flags for
# targets whose SYSTEM property is true, and the normal flags otherwise.
add_compile_options("$<IF:$<BOOL:$<TARGET_PROPERTY:SYSTEM>>,${THIRD_PARTY_COMPILE_ERROR_FLAGS},${COMPILE_ERROR_FLAGS}>")
