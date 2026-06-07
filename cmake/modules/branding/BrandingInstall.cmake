# Provide branding steps
# - macOS, Linux: This script is intended to be run during install phase
# - Windows: Due to (.rc) resource files being part of the compile process must be done at either configure or compile time
# - Branding-related files will be outputted to build/branded
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_policy(SET CMP0011 NEW)

if(NOT LMMS_BRANDING_COLOR OR LMMS_BRANDING_COLOR STREQUAL "green")
	# Skip branding for stable releases
	message("Skipping dynamic branding: LMMS_BRANDING_COLOR=\"${LMMS_BRANDING_COLOR}\"")
	return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/branding" "${CMAKE_CURRENT_LIST_DIR}")
include(SetupBrandingEnv)
include(SvgRecolor)
include(SvgConvert)
setup_env()

# Search / replace
set(svg_patterns_search "svg_patterns_green")
set(svg_patterns_replace "svg_patterns_${LMMS_BRANDING_COLOR}")

# SVG color replacements
set(svg_patterns_green "#09b960;#009a4d;#00d36d;opacity=\".1\" fill=\"#fff\"")
set(svg_patterns_yellow "#fcab00;#d48e00;#ffc976;opacity=\".1\" fill=\"#fff\"")
set(svg_patterns_purple "#9884ff;#805bfe;#ada3ff;opacity=\".05\" fill=\"#fff\"")
set(svg_patterns_blue "#289fff;#0083e0;#6db6ff;opacity=\".1\" fill=\"#fff\"")

file(REMOVE_RECURSE "${CPACK_BRANDED_DIR}")

# Recolor SVGs
# recolor app icon
svg_recolor(
	"${svg_patterns_search}"
	"${svg_patterns_replace}"
	"${LMMS_SOURCE_DIR}/data/scalable/lmms.svg"
	lmms_svg_branded
)
# recolor project icon
svg_recolor(
	"${svg_patterns_search}"
	"${svg_patterns_replace}"
	"${LMMS_SOURCE_DIR}/data/scalable/project.svg"
	project_svg_branded
)
# recolor splash
svg_recolor(
	"${svg_patterns_search}"
	"${svg_patterns_replace}"
	"${LMMS_SOURCE_DIR}/data/scalable/splash.svg"
	splash_svg_branded
)

# Platform-specific steps
if(WIN32)
	set(THEMES_DIR data/themes)
	include(MagickConvert)
	# used only by nsis
	# - top banner
	set(temp_icon "${CPACK_BRANDED_DIR}/temp_icon.png")
	svg_convert(64 "${lmms_svg_branded}" "${temp_icon}")
	nsis_banner("${temp_icon}" "${CPACK_BRANDED_DIR}/nsis_branding.bmp" 150x57)
	file(REMOVE "${temp_icon}")
	# - left side banner
	#   - recolor welcome side banner
    svg_recolor(
    	"${svg_patterns_search}"
    	"${svg_patterns_replace}"
    	"${LMMS_SOURCE_DIR}/data/scalable/nsis_welcome.svg"
    	welcome_svg_rebranded
    )
    #   - convert welcome side banner to BMP3
    set(temp_welcome "${CPACK_BRANDED_DIR}/temp_welcome.png")
	svg_convert(164 "${welcome_svg_rebranded}" "${temp_welcome}")
	nsis_bmp("${temp_welcome}" "${CPACK_BRANDED_DIR}/nsis_welcome.bmp")
	file(REMOVE "${temp_welcome}")

	# used by both winrc and nsis
	ico_convert("${lmms_svg_branded}" "${CPACK_BRANDED_DIR}/icon.ico")

	# used only by winrc
	ico_convert("${project_svg_branded}" "${CPACK_BRANDED_DIR}/project.ico")
elseif(APPLE)
	set(THEMES_DIR Contents/share/lmms/themes)
	include(IconUtilConvert)
	icns_convert("${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/Contents/Resources/lmms.icns")
	icns_convert("${project_svg_branded}" "${CPACK_BRANDED_APP_DIR}/Contents/Resources/project.icns")

	# dmg background
	svg_recolor(
		"${svg_patterns_search}"
		"${svg_patterns_replace}"
		"${LMMS_SOURCE_DIR}/data/scalable/dmg_background.svg"
		dmg_svg_rebranded
	)
	set(bg_sizes 705 705@2)
	svg_convert("${bg_sizes}" "${dmg_svg_rebranded}" "${CPACK_BRANDED_DIR}/dmg_background@%mult%x.png")
else()
	# Linux, Unix
	set(THEMES_DIR usr/share/lmms/themes)
	# AppImage Icons: 256x256 default for Cinnamon Desktop https://forums.linuxmint.com/viewtopic.php?p=2585952
	svg_convert(256 "${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/lmms.png")
	svg_convert(256 "${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/.DirIcon")
	# /usr/share/icons
	set(xdg_sizes 16 16@2 24 24@2 32 32@2 48 48@2 64 64@2 128 128@2 256)
	svg_convert("${xdg_sizes}" "${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/usr/share/icons/hicolor/%size%x%size%@%mult%/%name%.png")
	svg_convert("${xdg_sizes}" "${project_svg_branded}" "${CPACK_BRANDED_APP_DIR}/usr/share/icons/hicolor/%size%x%size%@%mult%/%name%.png")
endif()

# Theme files
svg_convert(128 "${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/${THEMES_DIR}/default/icon.png")
svg_convert(32 "${lmms_svg_branded}" "${CPACK_BRANDED_APP_DIR}/${THEMES_DIR}/default/icon_small.png")
# TODO: add asymetric width/height support to svg_convert for gimp
svg_convert(681 "${splash_svg_branded}" "${CPACK_BRANDED_APP_DIR}/${THEMES_DIR}/default/splash.png")

# message(FATAL_ERROR "\n\n\n==== INTENTIONALLY INTERRUPTED ====\n\n\n")
