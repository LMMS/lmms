/* Calf DSP Library
 * Example audio modules - LADSPA/DSSI/LV2 wrapper instantiation
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <calf/ladspa_wrap.h>
#include <calf/lv2wrap.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/modules_small.h>

using namespace calf_plugins;

#if USE_LADSPA
template<class Module>
LADSPA_Descriptor ladspa_wrapper<Module>::descriptor;

template<class Module>
LADSPA_Descriptor ladspa_wrapper<Module>::descriptor_for_dssi;

#if USE_DSSI

template<class Module>
DSSI_Descriptor ladspa_wrapper<Module>::dssi_descriptor;

template<class Module>
DSSI_Program_Descriptor ladspa_wrapper<Module>::dssi_default_program;

template<class Module>
std::vector<plugin_preset> *ladspa_wrapper<Module>::presets;

template<class Module>
std::vector<DSSI_Program_Descriptor> *ladspa_wrapper<Module>::preset_descs;
#endif
#endif

#if USE_LV2
// instantiate descriptor templates
template<class Module> LV2_Descriptor calf_plugins::lv2_wrapper<Module>::descriptor;
template<class Module> LV2_Calf_Descriptor calf_plugins::lv2_wrapper<Module>::calf_descriptor;
template<class Module> LV2MessageContext calf_plugins::lv2_wrapper<Module>::message_context;

extern "C" {

const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!(index--)) return &lv2_wrapper<name##_audio_module>::get().descriptor;
    #include <calf/modulelist.h>
#ifdef ENABLE_EXPERIMENTAL
    return lv2_small_descriptor(index);
#else
    return NULL;
#endif
}

};

#endif

#if USE_LADSPA
extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!isSynth && !(Index--)) return &ladspa_wrapper<name##_audio_module>::get().descriptor;
    #include <calf/modulelist.h>
    return NULL;
}

};

#if USE_DSSI
extern "C" {

const DSSI_Descriptor *dssi_descriptor(unsigned long Index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!(Index--)) return &calf_plugins::ladspa_wrapper<name##_audio_module>::get().dssi_descriptor;
    #include <calf/modulelist.h>
    return NULL;
}

};
#endif

#endif

