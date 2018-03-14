#include "jalv_internal.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"

#define NS_EXT "http://lv2plug.in/ns/ext/"

LV2_Feature uri_map_feature      = { NS_EXT "uri-map", NULL };
LV2_Feature map_feature          = { LV2_URID__map, NULL };
LV2_Feature unmap_feature        = { LV2_URID__unmap, NULL };
LV2_Feature make_path_feature    = { LV2_STATE__makePath, NULL };
LV2_Feature sched_feature        = { LV2_WORKER__schedule, NULL };
LV2_Feature state_sched_feature  = { LV2_WORKER__schedule, NULL };
LV2_Feature safe_restore_feature = { LV2_STATE__threadSafeRestore, NULL };
LV2_Feature log_feature          = { LV2_LOG__log, NULL };
LV2_Feature options_feature      = { LV2_OPTIONS__options, NULL };
LV2_Feature def_state_feature    = { LV2_STATE__loadDefaultState, NULL };
