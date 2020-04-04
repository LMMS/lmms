// A dummy Carla interface
#include "CarlaNativePlugin.h"

const char* carla_get_library_filename() { return nullptr; }
const char* carla_get_library_folder() { return nullptr; }
const NativePluginDescriptor* carla_get_native_rack_plugin() { return nullptr; }
const NativePluginDescriptor* carla_get_native_patchbay_plugin() { return nullptr; }
const NativePluginDescriptor* carla_get_native_patchbay16_plugin() { return nullptr; }
const NativePluginDescriptor* carla_get_native_patchbay32_plugin() { return nullptr; }
const NativePluginDescriptor* carla_get_native_patchbay64_plugin() { return nullptr; }
const NativePluginDescriptor* carla_get_native_patchbay_cv_plugin() { return nullptr; }
CarlaBackend::CarlaEngine* carla_get_native_plugin_engine(const NativePluginDescriptor* desc, NativePluginHandle handle) { return nullptr; }
