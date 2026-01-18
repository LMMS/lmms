# Copyright (c) 2025-2026 Dalton Messmer <messmer.dalton/at/gmail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(lv2 lv2::lv2
	LIBRARY_NAMES "lv2"
	INCLUDE_NAMES "lv2/core/lv2.h"
	PKG_CONFIG "lv2"
	PREFIX LV2
)

find_package_config_mode_with_fallback(serd serd::serd
	LIBRARY_NAMES "serd-0"
	INCLUDE_NAMES "serd/serd.h"
	PKG_CONFIG "serd-0"
)

find_package_config_mode_with_fallback(zix zix::zix
	LIBRARY_NAMES "zix" "zix-0"
	INCLUDE_NAMES "zix/zix.h"
	PKG_CONFIG "zix-0"
)

find_package_config_mode_with_fallback(sord sord::sord
	LIBRARY_NAMES "sord" "sord-0"
	INCLUDE_NAMES "sord/sord.h"
	PKG_CONFIG "sord-0"
	DEPENDS zix::zix serd::serd
)

find_package_config_mode_with_fallback(sratom sratom::sratom
	LIBRARY_NAMES "sratom" "sratom-0"
	INCLUDE_NAMES "sratom/sratom.h"
	PKG_CONFIG "sratom-0"
	DEPENDS lv2::lv2 serd::serd sord::sord
)

find_package_config_mode_with_fallback(Lilv Lilv::lilv
	LIBRARY_NAMES "lilv" "lilv-0"
	INCLUDE_NAMES "lilv/lilv.h"
	PKG_CONFIG "lilv-0"
	DEPENDS lv2::lv2 serd::serd sord::sord sratom::sratom zix::zix
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Lilv
	REQUIRED_VARS Lilv_LIBRARY Lilv_INCLUDE_DIRS
	VERSION_VAR Lilv_VERSION
)
