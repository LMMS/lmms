# Branding helpers
# - install_branding: Calls BrandingInstall.cmake at install() using code injection
# - add_branded_target: Calls BrandingInstall.cmake at compile using add_custom_target and cmake -P
# - calculate_branding: Calculates if this build needs branding, and what type (e.g. "Nightly"), sets vars accordingly
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(SetupBrandingEnv)
setup_public_env()

# Parameters to be exposed to BrandingInstall
set(LMMS_BRANDING_EXPORTS
	LMMS_BRANDING_NEEDED
	LMMS_BRANDING_COLOR
	LMMS_SOURCE_DIR
	LMMS_BINARY_DIR
	BRANDING_DEBUG
)

# The main branding install script
set(LMMS_BRANDING_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/modules/branding/BrandingInstall.cmake")

# Macro to call the branding script with parameters at install (macOS, Linux)
macro(install_branding)
	foreach(export IN LISTS LMMS_BRANDING_EXPORTS)
		string(APPEND export_cmds "set(${export} \"${${export}}\")\n")
	endforeach()
	install(CODE "${export_cmds}")
    install(SCRIPT "${LMMS_BRANDING_SCRIPT}")
endmacro()

# Macro to call the branding script with parameters at build time before target (Windows)
macro(add_branded_target target)
	foreach(export IN LISTS LMMS_BRANDING_EXPORTS)
		list(APPEND export_cmds "-D${export}=${${export}}")
	endforeach()

	add_custom_target(windows_branding
		COMMAND ${CMAKE_COMMAND}
		${export_cmds}
		-P "${LMMS_BRANDING_SCRIPT}"
		COMMENT "Applying dynamic branding"
		VERBATIM
	)
	add_dependencies(${target} windows_branding)

	# rc files work best with path relative to the project source
	file(RELATIVE_PATH LMMS_EXE_ICON "${LMMS_SOURCE_DIR}" "${CPACK_BRANDED_DIR}/icon.ico")
	file(RELATIVE_PATH LMMS_PROJ_ICON "${LMMS_SOURCE_DIR}" "${CPACK_BRANDED_DIR}/project.ico")
	target_compile_definitions(${target} PRIVATE
		LMMS_EXE_ICON="${LMMS_EXE_ICON}"
		LMMS_PROJ_ICON="${LMMS_PROJ_ICON}"
	)
endmacro()

# Check for branding dependencies, calculate branding type & color
# If successful:
# - Variable passed as type_var will be set to "alpha" or "nightly"
# - Variable passed as release_var will be set to "Alpha" or "Nightly"
# - Variable passed as color_var will be set to "blue" or "purple"
# - Variable passed as status_var will be set to a bash-colored "alpha" or "nightly"
function(calculate_branding version_var type_var release_var color_var status_var)
	# Check for a suitable rebranding tools
	include(SvgConvert)
	if(NOT SvgConvert_FOUND)
		message(WARNING "Dynamic branding is disabled: 'rsvg-convert', 'gimp' or 'inkscape' were not found.")
		return()
	endif()
	if(WIN32)
		include(MagickConvert)
		if(NOT MagickConvert_FOUND)
			message(WARNING "Dynamic branding is disabled: 'magick' was not found.")
			return()
		endif()
	elseif(APPLE)
		include(IconUtilConvert)
		if(NOT IconUtilConvert_FOUND)
			message(WARNING "Dynamic branding is disabled: 'iconutil' was not found")
			return()
		endif()
	endif()

	# Process version information to calculate stable|alpha|nightly
	# TODO: Test this calculation logic against different version patterns
	set(version "${${version_var}}")
	if(version MATCHES "\\+pr")
		set(${type_var} "draft")
		set(${release_var} "Draft")
		set(${color_var} "yellow")
	elseif(version MATCHES "\\+")
		set(${type_var} "nightly")
		set(${release_var} "Nightly")
		set(${color_var} "purple")
	elseif(version MATCHES "alpha")
		set(${type_var} "alpha")
		set(${release_var} "Alpha")
		set(${color_var} "blue")
	elseif(version MATCHES "beta")
		# "beta" branding uses same color as "alpha"
		set(${type_var} "beta")
		set(${release_var} "Beta")
        set(${color_var} "blue")
	else()
		# assume "Stable", don't set ${type_var}, don't set ${color_var}
		set(${release_var} "Stable" PARENT_SCOPE)
		message(WARNING "Dynamic branding is disabled: ${version}")
		return()
	endif()

	# Colorize output
	string(ASCII 27 Esc)
	set(reset "${Esc}[m")
	set(yellow "${Esc}[33m")
	set(purple "${Esc}[35m")
	set(blue "${Esc}[36m")
	set(${status_var} "${${${color_var}}}${${type_var}}${reset}")

	# Expose to calling script
	set(${type_var} "${${type_var}}" PARENT_SCOPE)
	set(${release_var} "${${release_var}}" PARENT_SCOPE)
	set(${color_var} "${${color_var}}" PARENT_SCOPE)
	set(${status_var} "${${status_var}}" PARENT_SCOPE)
endfunction()