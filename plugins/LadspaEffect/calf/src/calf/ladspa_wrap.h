/* Calf DSP Library
 * API wrappers for LADSPA/DSSI
 *
 * Copyright (C) 2007-2008 Krzysztof Foltman
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
#ifndef __CALF_LADSPA_WRAP_H
#define __CALF_LADSPA_WRAP_H

#if USE_LADSPA

#include <string.h>
#include <ladspa.h>
#if USE_DSSI
#include <dssi.h>
#endif
#include "giface.h"
#include "preset.h"

namespace calf_plugins {

struct ladspa_plugin_metadata_set;
/// A template implementing plugin_ctl_iface for a given plugin
struct ladspa_instance: public plugin_ctl_iface
{
    audio_module_iface *module;
    const plugin_metadata_iface *metadata;
    ladspa_plugin_metadata_set *ladspa;
    bool activate_flag;
    float **ins, **outs, **params;
#if USE_DSSI
    dssi_feedback_sender *feedback_sender;
#endif
    
    ladspa_instance(audio_module_iface *_module, ladspa_plugin_metadata_set *_ladspa, int sample_rate);
	virtual ~ladspa_instance();
    virtual const line_graph_iface *get_line_graph_iface() const { return module->get_line_graph_iface(); }
    virtual float get_param_value(int param_no);
    virtual void set_param_value(int param_no, float value);
    virtual bool activate_preset(int bank, int program);
    virtual char *configure(const char *key, const char *value);
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        module->execute(cmd_no);
    }
    virtual void send_configures(send_configure_iface *sci) { 
        module->send_configures(sci);
    }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { return module->send_status_updates(sui, last_serial); }
    void run(unsigned long SampleCount);
#if USE_DSSI
    /// Utility function: handle MIDI event (only handles a subset in this version)
    void process_dssi_event(snd_seq_event_t &event);
    void run_synth(unsigned long SampleCount, snd_seq_event_t *Events, unsigned long EventCount);
#endif
    virtual const plugin_metadata_iface *get_metadata_iface() const
    {
        return metadata;
    }
};

/// Set of metadata produced by LADSPA wrapper for LADSPA-related purposes
struct ladspa_plugin_metadata_set
{
    /// LADSPA descriptor
    LADSPA_Descriptor descriptor;
    /// LADSPA descriptor for DSSI (uses a different name for the plugin, otherwise same as descriptor)
    LADSPA_Descriptor descriptor_for_dssi;
#if USE_DSSI
    /// Extended DSSI descriptor (points to descriptor_for_dssi for things like name/label/port info etc.)
    DSSI_Descriptor dssi_descriptor;
    DSSI_Program_Descriptor dssi_default_program;

    std::vector<plugin_preset> *presets;
    std::vector<DSSI_Program_Descriptor> *preset_descs;
#endif
    
    int input_count, output_count, param_count;
    const plugin_metadata_iface *metadata;
    
    ladspa_plugin_metadata_set();
    void prepare(const plugin_metadata_iface *md, LADSPA_Handle (*cb_instantiate)(const struct _LADSPA_Descriptor * Descriptor, unsigned long sample_rate));
    void prepare_dssi();
    ~ladspa_plugin_metadata_set();
};

/// A wrapper class for plugin class object (there is only one ladspa_wrapper singleton for many instances of the same plugin)
template<class Module>
struct ladspa_wrapper
{
    static ladspa_plugin_metadata_set output;
    
private:
    ladspa_wrapper(const plugin_metadata_iface *md)
    {
        output.prepare(md, cb_instantiate);
    }

public:
    /// LADSPA instantiation function (create a plugin instance)
    static LADSPA_Handle cb_instantiate(const struct _LADSPA_Descriptor * Descriptor, unsigned long sample_rate)
    {
        return new ladspa_instance(new Module, &output, sample_rate);
    }

    /// Get a wrapper singleton - used to prevent initialization order problems which were present in older versions
    static ladspa_plugin_metadata_set &get() { 
        static ladspa_wrapper instance(new typename Module::metadata_class);
        return instance.output;
    }
};

};

#endif

#endif
