/* Calf DSP plugin pack
 * LADSPA/DSSI/LV2 wrapper instantiation for all plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <config.h>
#include <calf/lv2wrap.h>
#include <calf/modules.h>
#include <calf/modules_comp.h>
#include <calf/modules_limit.h>
#include <calf/modules_dev.h>
#include <calf/modules_dist.h>
#include <calf/modules_eq.h>
#include <calf/modules_mod.h>
#include <calf/modules_synths.h>
#include <calf/organ.h>

using namespace calf_plugins;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if USE_LV2
// instantiate descriptor templates
template<class Module> LV2_Descriptor calf_plugins::lv2_wrapper<Module>::descriptor;
template<class Module> LV2_Calf_Descriptor calf_plugins::lv2_wrapper<Module>::calf_descriptor;
template<class Module> LV2_State_Interface calf_plugins::lv2_wrapper<Module>::state_iface;

extern "C" {

const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!(index--)) return &lv2_wrapper<name##_audio_module>::get().descriptor;
    #include <calf/modulelist.h>
    return NULL;
}

};

#endif

#if USE_JACK

extern "C" {

audio_module_iface *create_calf_plugin_by_name(const char *effect_name)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!strcasecmp(effect_name, jackname)) return new name##_audio_module;
    #include <calf/modulelist.h>
    return NULL;
}

}

#endif
