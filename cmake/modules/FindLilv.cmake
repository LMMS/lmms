# Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(Lilv Lilv::lilv
	LIBRARY_NAMES "lilv" "lilv-0"
	INCLUDE_NAMES "lilv/lilv.h"
	PKG_CONFIG "lilv-0"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Lilv
	REQUIRED_VARS Lilv_LIBRARY Lilv_INCLUDE_DIRS
	VERSION_VAR Lilv_VERSION
)
