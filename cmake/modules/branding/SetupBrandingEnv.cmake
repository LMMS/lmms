# Shared branding variables
#
# Copyright (c) 2025, Tres Finocchiaro, <tres.finocchiaro@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Public vars e.g. CPACK_BRANDED_DIR, CPACK_BRANDED_APP_DIR
macro(setup_public_env)
	# branded output directory
	set(CPACK_BRANDED_DIR "${LMMS_BINARY_DIR}/branded")
	set(CPACK_BRANDED_APP_DIR "${CPACK_BRANDED_DIR}/cpack")
endmacro()

# Scoped vars
macro(setup_env)
	# Will show command line output for troubleshooting purposes
	# e.g. "-DBRANDING_DEBUG=ON"
	# e.g. "export BRANDING_DEBUG=ON"
	if(DEFINED ENV{BRANDING_DEBUG})
		set(BRANDING_DEBUG "$ENV{BRANDING_DEBUG}")
	endif()
	if(BRANDING_DEBUG)
		set(COMMAND_ECHO STDOUT)
	else()
		set(COMMAND_ECHO NONE)
	endif()

	setup_public_env()
endmacro()