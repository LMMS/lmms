// A dummy Carla interface
#define BUILDING_CARLA
#include <CarlaNativePlugin.h>

#ifndef CARLA_PLUGIN_EXPORT
#define CARLA_PLUGIN_EXPORT CARLA_EXPORT
#endif

CARLA_PLUGIN_EXPORT const char* carla_get_library_filename() { return nullptr; }
CARLA_PLUGIN_EXPORT const char* carla_get_library_folder() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_rack_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_patchbay16_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_patchbay32_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_patchbay64_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_cv_plugin() { return nullptr; }
CARLA_PLUGIN_EXPORT CarlaBackend::CarlaEngine* carla_get_native_plugin_engine(const NativePluginDescriptor* desc, NativePluginHandle handle) { return nullptr; }
