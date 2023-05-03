# Colors and converts .svg files to .png files
# Also creates platform-specific themed files (.ico, icns)
#
# Copyright (c) 2019, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

FIND_PACKAGE(ImageMagick COMPONENTS convert)

# Prefer EXECUTE_PROCESS over PKG_CHECK_MODULES per https://apple.stackexchange.com/q/169601/147537
EXECUTE_PROCESS(COMMAND rsvg-convert --version ERROR_QUIET OUTPUT_VARIABLE RSVG_FOUND)

# Fallback to gimp on Windows
IF(LMMS_BUILD_WIN32 AND NOT (ImageMagick_FOUND AND RSVG_FOUND))
	MESSAGE(STATUS "Branding tools are missing looking for GIMP...")
	GET_FILENAME_COMPONENT(GIMP_REG [HKEY_CLASSES_ROOT\\GIMP2.xcf\\shell\\open\\command] ABSOLUTE)
	STRING(REPLACE "\"" ";" GIMP_REG ${GIMP_REG})
	LIST(LENGTH GIMP_REG GIMP_REG_SIZE)
	IF(GIMP_REG_SIZE GREATER 1)
		LIST(GET GIMP_REG 1 GIMP_GUI)
		STRING(REPLACE "/gimp-" "/gimp-console-" GIMP_CONSOLE "${GIMP_GUI}")
		EXECUTE_PROCESS(COMMAND "${GIMP_CONSOLE}" --version ERROR_QUIET OUTPUT_VARIABLE GIMP_FOUND)
	ENDIF()

	# Fallback on gimp-console alias (e.g. scoop)
	IF(NOT GIMP_FOUND)
		EXECUTE_PROCESS(COMMAND gimp-console --version ERROR_QUIET OUTPUT_VARIABLE GIMP_FOUND)
		IF(GIMP_FOUND)
			SET(GIMP_CONSOLE gimp-console)
		ENDIF()
	ENDIF()

	IF(GIMP_FOUND)
		MESSAGE(STATUS "GIMP found: ${GIMP_CONSOLE}")
	ENDIF()
ENDIF()

IF((ImageMagick_FOUND AND RSVG_FOUND) OR GIMP_FOUND)
	# Run only if the LMMS_RELEASE_TYPE has changed
	IF(NOT LMMS_RELEASE_TYPE_CACHE STREQUAL "${LMMS_RELEASE_TYPE}")
		SET(LMMS_RELEASE_TYPE_CACHE "${LMMS_RELEASE_TYPE}" CACHE STRING "" FORCE)
	ELSE()
		MESSAGE(STATUS "Skipping rebranding, LMMS_RELEASE_TYPE hasn't changed")
		RETURN()
	ENDIF()
ELSE()
	MESSAGE(WARNING "Tools \"convert\" and/or \"rsvg-convert\" and/or \"gimp-console\" are missing.  Releases will use the default branding instead.")
	RETURN()
ENDIF()


# Reads _src file, replacing any text matching _search
# with that from _replace, writes to _dest
MACRO(SEARCH_AND_REPLACE _src _dest _search _replace)
	FILE(READ "${_src}" _data)
	# Expand quoted lists, cmake won't
	SET(_search_list ${_search})
	SET(_replace_list ${_replace})
	LIST(LENGTH _search_list _search_max)
	MATH(EXPR _search_max "${_search_max}-1")
	FOREACH(i RANGE ${_search_max})
		LIST(GET _search_list ${i} _search_part)
		LIST(GET _replace_list ${i} _replace_part)
		STRING(REPLACE "${_search_part}" "${_replace_part}" _data "${_data}")
	ENDFOREACH()
	FILE(WRITE "${_dest}" "${_data}")
ENDMACRO()

MACRO(GIMP_BATCH _source _width _height _output)
    MESSAGE(STATUS "Converting ${_source} -> ${_output} (${_width}x${_height})")
	# FIXME: autocrop causes issues with "windows-tile"
	# SET(_autocrop "(plug-in-autocrop RUN-NONINTERACTIVE image drawable)")
	SET(_autocrop "")
    EXECUTE_PROCESS(COMMAND "${GIMP_CONSOLE}" -i -b "(let* ((image (car (file-svg-load RUN-NONINTERACTIVE \"${_source}\" \"\" 72 (- 0 ${_width}) (- 0 ${_height}) 0))) (drawable (car (gimp-image-get-active-layer image)))) ${_autocrop} (gimp-file-save RUN-NONINTERACTIVE image drawable \"${_output}\" \"${_output}\") (gimp-image-delete image))" -b "(gimp-quit 0)" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
ENDMACRO()

SET(WINDOWS_SIZES 16;32;48;64;128)
SET(APPLE_SIZES 16;32;128;256;512)
SET(LINUX_SIZES 16;24;32;48;64;96;128)
# Fallback svg to use if an exact match can't be found
SET(FALLBACK_SIZE 64)
# Produce icons at 1x, 2x
SET(SIZE_MULTIPLIERS 1;2)

# File patterns for icons
SET(MULTIRES_PATTERNS icon;project)

# HTML color codes to search for
SET(SVG_GREEN "#27ab5f;#249a56;#34d07b;opacity=\".1\" fill=\"#fff\"")
SET(SVG_BLUE "#3992cb;#2b6fc5;#62a8d4;opacity=\".1\" fill=\"#fff\"")
SET(SVG_PURPLE "#5547bd;#493ba1;#7871c5;opacity=\".05\" fill=\"#fff\"")

SET(SVG_SEARCH "${SVG_GREEN}")

# Handle release type
IF(LMMS_RELEASE_TYPE STREQUAL Stable)
	SET(SVG_REPLACE "${SVG_GREEN}")
ELSEIF(LMMS_RELEASE_TYPE STREQUAL Beta)
	SET(SVG_REPLACE "${SVG_BLUE}")
ELSE() # Nightly
	SET(SVG_REPLACE "${SVG_PURPLE}")
ENDIF()

# Handle platform
IF(LMMS_BUILD_APPLE)
	SET(ICON_FORMAT icns)
	SET(ICON_SIZES "${APPLE_SIZES}")
	SET(ICON_DEST ../apple)
ELSEIF(LMMS_BUILD_WIN32)
	SET(ICON_FORMAT ico)
	SET(ICON_SIZES "${WINDOWS_SIZES}")
	SET(ICON_DEST ../nsis)
ELSE()
	SET(ICON_FORMAT all)
	SET(ICON_SIZES "${LINUX_SIZES}")
	SET(ICON_DEST ../linux)
ENDIF()

MESSAGE(STATUS "Creating LMMS_RELEASE_TYPE=\"${LMMS_RELEASE_TYPE}\" resources for \"${PROJECT_NAME} ${VERSION}\"...")

MESSAGE(STATUS "  Branding svg files at their native resolutions...")
FILE(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/svg")
FILE(COPY svg DESTINATION .)
FILE(GLOB svgs "${CMAKE_CURRENT_BINARY_DIR}/svg/*.svg")
FOREACH(svg ${svgs})
	# Replace colors within svg files
	SEARCH_AND_REPLACE("${svg}" "${svg}" "${SVG_SEARCH}" "${SVG_REPLACE}")

	# Calculate dimension from filename, store as ${svg_res}
	UNSET(svg_res)
	GET_FILENAME_COMPONENT(file_name "${svg}" NAME_WE)
	STRING(REPLACE "x" ";" name_split "${file_name}")
	LIST(LENGTH name_split name_split_max)
	IF(name_split_max GREATER 1)
		MATH(EXPR name_split_max "${name_split_max}-1")
		LIST(GET name_split ${name_split_max} svg_res)
	ENDIF()

	# Generate pngs
	FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/png")
	SET(png_out "png/${file_name}.png")
	IF(svg_res)
		# Calculate upscaled dimension
		IF(svg MATCHES "windows-tile")
			# Windows tile is always 4x
			MATH(EXPR res_out "${svg_res}*4")
			SET(png_out "png/${file_name}@4.png")
			# FIXME Make GIMP_BATCH conditonal
			GIMP_BATCH("${svg}" "${res_out}" "${res_out}" "${png_out}")
			EXECUTE_PROCESS(COMMAND rsvg-convert "${svg}" -w "${res_out}" -h "${res_out}" -o "${png_out}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
		ELSE()
			# Others are 1x and 2x
			FOREACH(mult ${SIZE_MULTIPLIERS})
				MATH(EXPR res_out "${svg_res}*${mult}")
				IF(NOT mult EQUAL 1)
					SET(png_out "png/${file_name}@${mult}.png")
				ENDIF()
				EXECUTE_PROCESS(COMMAND rsvg-convert "${svg}" -w "${res_out}" -h "${res_out}" -o "${png_out}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
			ENDFOREACH()
		ENDIF()
	ELSE()
		# Render at natural resolution
		EXECUTE_PROCESS(COMMAND rsvg-convert "${svg}" -o "${png_out}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	ENDIF()
ENDFOREACH()

# Handle orphaned dimensions where no specific svg exists (e.g. 128x128 on macOS)
MESSAGE(STATUS "  Branding missing files for this platform...")
FOREACH(size ${ICON_SIZES})
	# Check for missing sizes
	FOREACH(pattern ${MULTIRES_PATTERNS})
		SET(png_out "png/${pattern}_${size}x${size}.png")
		SET(lookfor "svg/${pattern}_${size}x${size}.svg")
		SET(fallback "svg/${pattern}_${FALLBACK_SIZE}x${FALLBACK_SIZE}.svg")
		# Generate missing dimension from fallback size
		IF(NOT EXISTS "${lookfor}")
			FOREACH(mult ${SIZE_MULTIPLIERS})
				MATH(EXPR res_out "${size}*${mult}")
				IF(NOT mult EQUAL 1)
					SET(png_out "png/${pattern}_${size}x${size}@${mult}.png")
				ENDIF()
				EXECUTE_PROCESS(COMMAND rsvg-convert "${fallback}" -w "${res_out}" -h "${res_out}" -o "${png_out}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
			ENDFOREACH()
		ENDIF()
	ENDFOREACH()
ENDFOREACH()

# Handle files needed at packaging time (.ico, .icns, etc)
MESSAGE(STATUS "  Branding ${ICON_FORMAT} file(s)...")
FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${ICON_DEST}")
FOREACH(pattern ${MULTIRES_PATTERNS})
	IF("${ICON_FORMAT}" STREQUAL icns)
		# Apple
		FILE(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/${pattern}.iconset")
		FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${pattern}.iconset") # .iconset suffix required
		FOREACH(res ${APPLE_SIZES})
			FOREACH(mult ${SIZE_MULTIPLIERS})
				SET(png_src "png/${pattern}_${res}x${res}.png")
				SET(png_dest "${pattern}.iconset/icon_${res}x${res}.png")
				IF(NOT mult EQUAL 1)
					SET(png_src "png/${pattern}_${res}x${res}@${mult}.png")
					SET(png_dest "${pattern}.iconset/icon_${res}x${res}@${mult}.png")
				ENDIF()
				# cmake's file(copy) can't copy and rename; configure_file() can
				CONFIGURE_FILE("${CMAKE_CURRENT_BINARY_DIR}/${png_src}" "${png_dest}" COPYONLY)
			ENDFOREACH()
		ENDFOREACH()
		# Create .icns file directly into packaging location
		EXECUTE_PROCESS(COMMAND iconutil -c icns "${pattern}.iconset" -o "${ICON_DEST}/${pattern}.icns" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
		MESSAGE(STATUS "    ${ICON_DEST}/${pattern}.icns")
	ELSEIF("${ICON_FORMAT}" STREQUAL ico)
		# Windows
		# Create .ico file
		EXECUTE_PROCESS(COMMAND convert "png/${pattern}_*.png" -background transparent "${ICON_DEST}/${pattern}.ico" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
		IF(pattern STREQUAL "icon")
			# Create nsis banner
			EXECUTE_PROCESS(COMMAND convert -size 150x57 canvas: nsis_canvas.png WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
			EXECUTE_PROCESS(COMMAND convert nsis_canvas.png "png/${pattern}_64x64.png" -gravity center -geometry -25+1 -composite -colors 256 "${ICON_DEST}/nsis_branding.bmp" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
			MESSAGE(STATUS "    ${ICON_DEST}/nsis_branding.bmp")
			# Rewrite the UWP tile manifest BackgroundColor
			SET(uwp_file "${CMAKE_CURRENT_BINARY_DIR}/${ICON_DEST}/lmms.VisualElementsManifest.xml")
			SEARCH_AND_REPLACE("${uwp_file}" "${uwp_file}" "${SVG_SEARCH}" "${SVG_REPLACE}")
			MESSAGE(STATUS "    ${uwp_file}")
		ENDIF()
	ELSE()
		MESSAGE(STATUS "Warning: Linux isn't generating themed files.")
	ENDIF()
ENDFOREACH()