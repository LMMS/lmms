/* Calf DSP Library
 * "Small" audio modules for modular synthesis
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_MODULES_SMALL_H
#define __CALF_MODULES_SMALL_H

#if USE_LV2

#include <lv2.h>
#include "plugininfo.h"
#include "lv2_polymorphic_port.h"
#include "lv2helpers.h"

namespace calf_plugins {

/// Empty implementations for plugin functions. Note, that some functions aren't virtual, because they're called via the particular
/// subclass via template wrappers (ladspa_small_wrapper<> etc), not via base class pointer/reference. On the other hand,
/// other functions are virtual when overhead is acceptable (instantiation time functions etc.)
class null_small_audio_module: public uri_map_access
{
public:
    uint32_t srate;
    double odsr;
    uint32_t poly_type_control, poly_type_audio;
    /// for polymorphic ports: "is audio" flags for first 32 ports (should be sufficient for most plugins)
    uint32_t poly_port_types;

    null_small_audio_module()
    : srate((uint32_t)-1)
    , odsr(0.)
    , poly_type_control(0)
    , poly_type_audio(0)
    , poly_port_types(0)
    {
    }

    /// Called when host changes type of the polymorphic port
    inline void set_port_type(uint32_t port, uint32_t type, void *type_data) {
        if (port >= 32)
            return;
        uint32_t port_mask = 1 << port;
        if (type == poly_type_control)
            poly_port_types &= ~port_mask;
        else if (type == poly_type_audio)
            poly_port_types |= port_mask;
        on_port_types_changed();
    }
    
    /// Returns 1 for audio ports and 0 for control ports
    inline unsigned int port_is_audio(unsigned int port) {
        return (poly_port_types >> port) & 1;
    }
        
    /// Returns (unsigned)-1 for audio ports and 0 for control ports
    inline unsigned int port_audio_mask(unsigned int port) {
        return 0 - ((poly_port_types >> port) & 1);
    }
        
    /// Returns (unsigned)-1 for audio ports and 0 for control ports
    static inline unsigned int port_audio_mask(unsigned int port, uint32_t poly_port_types) {
        return 0 - ((poly_port_types >> port) & 1);
    }
        
    virtual void on_port_types_changed() {}
    inline void set_bundle_path(const char *path) {}
    /// Called to map all the necessary URIs
    virtual void map_uris()
    {
        poly_type_control = map_uri(LV2_POLYMORPHIC_PORT_URI, "http://lv2plug.in/ns/lv2core#ControlPort");
        poly_type_audio = map_uri(LV2_POLYMORPHIC_PORT_URI, "http://lv2plug.in/ns/lv2core#AudioPort");
    }
    /// Called on instantiation with the list of LV2 features called
    virtual void use_features(const LV2_Feature *const *features) {
        while(*features)
        {
            use_feature((*features)->URI, (*features)->data);
            features++;
        }
    }
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    inline void activate() {}
    /// LADSPA-esque deactivate function
    inline void deactivate() {}
    /// Set sample rate for the plugin
    inline void set_sample_rate(uint32_t sr) { srate = sr; odsr = 1.0 / sr; }
    static inline const void *ext_data(const char *URI) { return NULL; }
};

/// Templatized version useful when the number of inputs and outputs is small
template<unsigned int Inputs, unsigned int Outputs>
class small_audio_module_base: public null_small_audio_module
{
public:
    enum { in_count = Inputs, out_count = Outputs };
    /// Input pointers
    float *ins[in_count]; 
    /// Output pointers
    float *outs[out_count];
};

template<class Module>
struct lv2_small_wrapper
{
    typedef Module instance;
    static LV2_Descriptor descriptor;
    std::string uri;
    static uint32_t poly_port_types;
    
    lv2_small_wrapper(const char *id)
    {
        uri = "http://calf.sourceforge.net/small_plugins/" + std::string(id);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
        
        plugin_port_type_grabber ptg(poly_port_types);
        Module::plugin_info(&ptg);
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
    }

    static void cb_activate(LV2_Handle Instance) {
        // Note the changed semantics (now more LV2-like)
        instance *const mod = (instance *)Instance;
        mod->activate();
    }
    
    static void cb_deactivate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }
    
    static uint32_t cb_set_type(LV2_Handle Instance, uint32_t port, uint32_t type, void *type_data)
    {
        instance *const mod = (instance *)Instance;
        mod->set_port_type(port, type, type_data);
        return 0;
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance();
        mod->poly_port_types = poly_port_types;
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->set_bundle_path(bundle_path);
        mod->use_features(features);
        mod->set_sample_rate((uint32_t)sample_rate);
        return mod;
    }
    
    static void cb_run(LV2_Handle Instance, uint32_t SampleCount) {
        instance *const mod = (instance *)Instance;
        mod->process(SampleCount);
    }
    
    static void cb_cleanup(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    
    static const void *cb_ext_data(const char *URI) {
        return Module::ext_data(URI);
    }
};

extern const LV2_Descriptor *lv2_small_descriptor(uint32_t index);

};

#endif

#endif
