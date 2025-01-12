# StaticDependencies.cmake - adds features similar to interface properties that
#                            are only transitive over static dependencies.
#
# Copyright (c) 2024 Dominic Clark
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

define_property(TARGET
	PROPERTY STATIC_COMPILE_DEFINITIONS
	BRIEF_DOCS "Compile definitions to be used by targets linking statically to this one"
	FULL_DOCS "Behaves similarly to INTERFACE_COMPILE_DEFINITIONS, but only over static dependencies."
		"Effectively becomes private once an executable module is reached."
)

define_property(TARGET
	PROPERTY STATIC_LINK_LIBRARIES
	BRIEF_DOCS "Link libraries to be included in targets linking statically to this one"
	FULL_DOCS "Behaves similarly to INTERFACE_LINK_LIBRARIES, but only over static dependencies."
		"Effectively becomes private once an executable module is reached."
)

# Link a target statically to a set of libraries. Forward the given arguments to
# `target_link_libraries`, but also perform two additional functions. Firstly,
# ensure that the given libraries will be linked into any module that also
# links the given target (allowing, for example, object libraries and private
# static libraries to be used transitively). Secondly, add any static compile
# definitions from the given libraries to the set of compile definitions used
# to build the given target.
#
# This function must be used in order for static requirements as defined in this
# module to be inherited transitively. Using `target_link_libraries` instead
# will break the chain for static link libraries and static compile definitions.
#
# Usage:
#	target_static_libraries(
#		<target>                     # The target to which to add the libraries
#		[<PRIVATE|PUBLIC|INTERFACE>] # Optionally, the scope to use for the following libraries
#		<item>...                    # The libraries to which to link
#		[<PRIVATE|PUBLIC|INTERFACE> <item>...]...
#	)
function(target_static_libraries target)
	# Target types that have a link step
	set(linked_target_types "MODULE_LIBRARY" "SHARED_LIBRARY" "EXECUTABLE")
	# Possible scopes for dependencies
	set(scopes "PRIVATE" "PUBLIC" "INTERFACE")

	get_target_property(target_type "${target}" TYPE)
	set(scope "")

	# Iterate over the dependencies (and possibly scopes) that we were given
	foreach(dependency IN LISTS ARGN)
		# If we have a scope, store it so we can apply it to upcoming libraries
		if(dependency IN_LIST scopes)
			set(scope "${dependency}")
			continue()
		endif()

		# Link the target to the current dependency. (Note: `${scope}` is
		# unquoted so that the argument disappears if no scope was given.)
		target_link_libraries("${target}" ${scope} "${dependency}")

		# Store the dependency so it can be linked in with this target later
		set_property(
			TARGET "${target}"
			APPEND
			PROPERTY STATIC_LINK_LIBRARIES
			"${dependency}"
		)

		# If the dependency is a target, it may have some of our custom
		# properties defined on it, so we have a bit more work to do
		if(TARGET "${dependency}")
			# Ensure it makes sense to link statically to this dependency
			get_target_property(dependency_type "${dependency}" TYPE)
			if(dependency_type IN_LIST linked_target_types)
				message(SEND_ERROR "Cannot link statically to shared module ${dependency}")
			endif()

			# Transitively include static definitions and libraries
			set(defs "$<TARGET_GENEX_EVAL:${dependency},$<TARGET_PROPERTY:${dependency},STATIC_COMPILE_DEFINITIONS>>")
			set(libs "$<TARGET_GENEX_EVAL:${dependency},$<TARGET_PROPERTY:${dependency},STATIC_LINK_LIBRARIES>>")
			set_property(
				TARGET "${target}"
				APPEND
				PROPERTY STATIC_COMPILE_DEFINITIONS
				"${defs}"
			)
			set_property(
				TARGET "${target}"
				APPEND
				PROPERTY STATIC_LINK_LIBRARIES
				"${libs}"
			)

			# Add the dependency's transitive static compile definitions.
			# (Note: definitions are private so dynamically linked dependents
			# won't pick them up; a static dependent will have them set when
			# this function is called for it.)
			target_compile_definitions("${target}" PRIVATE "${defs}")

			# If the target has a link step, add the transitive static
			# dependencies. (Note: we use `LINK_ONLY` so the caller can still
			# control usage requirements through the normal use of scopes. Only
			# transitive dependencies are needed here: the direct dependency was
			# added earlier on. We have to append to `LINK_LIBRARIES` directly,
			# rather than use `target_link_libraries(PRIVATE)`, in order to
			# remain compatible with the scopeless signature of that command.)
			if(target_type IN_LIST linked_target_types)
				set_property(
					TARGET "${target}"
					APPEND
					PROPERTY LINK_LIBRARIES
					"$<LINK_ONLY:${libs}>"
				)
			endif()
		endif()
	endforeach()
endfunction()

# Add compile definitions to a target only for use with dependents linking
# statically to it.
#
# Behaves like `target_compile_definitions(INTERFACE)`, except the definitions
# will not be inherited beyond any executable module into which the target is
# actually linked. The definitions are added to the `STATIC_COMPILE_DEFINITIONS`
# property on the target.
#
# Usage:
#	target_static_definitions(
#		<target>        # The target to which to add the definitions
#		<definition>... # The definitions to add to the target
#	)
function(target_static_definitions target)
	set_property(
		TARGET "${target}"
		APPEND
		PROPERTY STATIC_COMPILE_DEFINITIONS
		"${ARGN}"
	)
endfunction()
