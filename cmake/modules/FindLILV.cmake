# Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(lilv lilv::lilv
	LIBRARY_NAMES "lilv" "lilv-0"
	INCLUDE_NAMES "lilv/lilv.h"
	PKG_CONFIG "lilv-0"
	PREFIX LILV
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LILV
	REQUIRED_VARS LILV_LIBRARY LILV_INCLUDE_DIRS
	VERSION_VAR LILV_VERSION
)
