/* Calf DSP Library
 * LV2 wrapper templates
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_LV2WRAP_H
#define CALF_LV2WRAP_H

#if USE_LV2

#include <string>
#include <vector>
#include <lv2.h>
#include <calf/giface.h>
#include <calf/lv2-midiport.h>
#include <calf/lv2_contexts.h>
#include <calf/lv2_event.h>
#include <calf/lv2_progress.h>
#include <calf/lv2_string_port.h>
#include <calf/lv2_uri_map.h>

namespace calf_plugins {

template<class Module>
struct lv2_instance: public plugin_ctl_iface, public progress_report_iface, public Module
{
    bool set_srate;
    int srate_to_set;
    LV2_MIDI *midi_data;
    LV2_Event_Buffer *event_data;
    LV2_URI_Map_Feature *uri_map;
    LV2_Event_Feature *event_feature;
    uint32_t midi_event_type;
    std::vector<int> message_params;
    LV2_Progress *progress_report_feature;
    lv2_instance()
    {
        for (int i=0; i < Module::in_count; i++)
            Module::ins[i] = NULL;
        for (int i=0; i < Module::out_count; i++)
            Module::outs[i] = NULL;
        for (int i=0; i < Module::param_count; i++)
            Module::params[i] = NULL;
        uri_map = NULL;
        midi_data = NULL;
        event_data = NULL;
        midi_event_type = 0xFFFFFFFF;
        set_srate = true;
        srate_to_set = 44100;
        get_message_context_parameters(message_params);
        progress_report_feature = NULL;
        // printf("message params %d\n", (int)message_params.size());
    }
    /// This, and not Module::post_instantiate, is actually called by lv2_wrapper class
    void post_instantiate()
    {
        if (progress_report_feature)
            Module::progress_report = this;
        Module::post_instantiate();
    }
    virtual parameter_properties *get_param_props(int param_no)
    {
        return &Module::param_props[param_no];
    }
    virtual float get_param_value(int param_no)
    {
        return *Module::params[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        *Module::params[param_no] = value;
    }
    virtual int get_param_count()
    {
        return Module::param_count;
    }
    virtual int get_param_port_offset() 
    {
        return Module::in_count + Module::out_count;
    }
    virtual const char *get_gui_xml() {
        return Module::get_gui_xml();
    }
    virtual line_graph_iface *get_line_graph_iface()
    {
        return dynamic_cast<line_graph_iface *>(this);
    }
    virtual bool activate_preset(int bank, int program) { 
        return false;
    }
    virtual const char *get_name()
    {
        return Module::get_name();
    }
    virtual const char *get_id()
    {
        return Module::get_id();
    }
    virtual const char *get_label()
    {
        return Module::get_label();
    }
    virtual int get_input_count() { return Module::in_count; }
    virtual int get_output_count() { return Module::out_count; }
    virtual bool get_midi() { return Module::support_midi; }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        Module::execute(cmd_no);
    }
    virtual void report_progress(float percentage, const std::string &message) {
        if (progress_report_feature)
            (*progress_report_feature->progress)(progress_report_feature->context, percentage, !message.empty() ? message.c_str() : NULL);
    }
    void send_configures(send_configure_iface *sci) { 
        Module::send_configures(sci);
    }
    uint32_t impl_message_run(const void *valid_inputs, void *output_ports) {
        for (unsigned int i = 0; i < message_params.size(); i++)
        {
            int pn = message_params[i];
            parameter_properties &pp = *get_param_props(pn);
            if ((pp.flags & PF_TYPEMASK) == PF_STRING
                && (((LV2_String_Data *)Module::params[pn])->flags & LV2_STRING_DATA_CHANGED_FLAG)) {
                printf("Calling configure on %s\n", pp.short_name);
                configure(pp.short_name, ((LV2_String_Data *)Module::params[pn])->data);
            }
        }
        return Module::message_run(valid_inputs, output_ports);
    }
    char *configure(const char *key, const char *value) { 
        // disambiguation - the plugin_ctl_iface version is just a stub, so don't use it
        return Module::configure(key, value);
    }
#if 0
    // the default implementation should be fine
    virtual void clear_preset() {
        // This is never called in practice, at least for now
        // However, it will change when presets are implemented
        for (int i=0; i < Module::param_count; i++)
            *Module::params[i] = Module::param_props[i].def_value;
        /*
        const char **p = Module::get_default_configure_vars();
        if (p)
        {
            for(; p[0]; p += 2)
                configure(p[0], p[1]);
        }
        */
    }
#endif
};

struct LV2_Calf_Descriptor {
    plugin_ctl_iface *(*get_pci)(LV2_Handle Instance);
};

template<class Module>
struct lv2_wrapper
{
    typedef lv2_instance<Module> instance;
    static LV2_Descriptor descriptor;
    static LV2_Calf_Descriptor calf_descriptor;
    static LV2MessageContext message_context;
    std::string uri;
    
    lv2_wrapper()
    {
        ladspa_plugin_info &info = Module::plugin_info;
        uri = "http://calf.sourceforge.net/plugins/" + std::string(info.label);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
        calf_descriptor.get_pci = cb_get_pci;
        message_context.message_connect_port = cb_connect;
        message_context.message_run = cb_message_run;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        unsigned long params = Module::param_count;
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
        else if (Module::support_midi && port == ins + outs + params) {
            mod->event_data = (LV2_Event_Buffer *)DataLocation;
        }
    }

    static void cb_activate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->set_srate = true;
    }
    
    static void cb_deactivate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance();
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->srate_to_set = (uint32_t)sample_rate;
        mod->set_srate = true;
        while(*features)
        {
            if (!strcmp((*features)->URI, LV2_URI_MAP_URI))
            {
                mod->uri_map = (LV2_URI_Map_Feature *)((*features)->data);
                mod->midi_event_type = mod->uri_map->uri_to_id(
                    mod->uri_map->callback_data, 
                    "http://lv2plug.in/ns/ext/event",
                    "http://lv2plug.in/ns/ext/midi#MidiEvent");
            }
            else if (!strcmp((*features)->URI, LV2_EVENT_URI))
            {
                mod->event_feature = (LV2_Event_Feature *)((*features)->data);
            }
            else if (!strcmp((*features)->URI, LV2_PROGRESS_URI))
            {
                mod->progress_report_feature = (LV2_Progress *)((*features)->data);
            }
            features++;
        }
        mod->post_instantiate();
        return mod;
    }
    static inline void zero_by_mask(Module *module, uint32_t mask, uint32_t offset, uint32_t nsamples)
    {
        for (int i=0; i<Module::out_count; i++) {
            if ((mask & (1 << i)) == 0) {
                dsp::zero(module->outs[i] + offset, nsamples);
            }
        }
    }
    static plugin_ctl_iface *cb_get_pci(LV2_Handle Instance)
    {
        return static_cast<plugin_ctl_iface *>(Instance);
    }

    static inline void process_slice(Module *mod, uint32_t offset, uint32_t end)
    {
        while(offset < end)
        {
            uint32_t newend = std::min(offset + MAX_SAMPLE_RUN, end);
            uint32_t out_mask = mod->process(offset, newend - offset, -1, -1);
            zero_by_mask(mod, out_mask, offset, newend - offset);
            offset = newend;
        }
    }

    static uint32_t cb_message_run(LV2_Handle Instance, const void *valid_inputs, void *outputs_written) {
        instance *mod = (instance *)Instance;
        return mod->impl_message_run(valid_inputs, outputs_written);
    }
    static void cb_run(LV2_Handle Instance, uint32_t SampleCount) {
        instance *const mod = (instance *)Instance;
        if (mod->set_srate) {
            mod->set_sample_rate(mod->srate_to_set);
            mod->activate();
            mod->set_srate = false;
        }
        mod->params_changed();
        uint32_t offset = 0;
        if (mod->event_data)
        {
            // printf("Event data: count %d\n", mod->event_data->event_count);
            struct LV2_Midi_Event: public LV2_Event {
                unsigned char data[1];
            };
            unsigned char *data = (unsigned char *)(mod->event_data->data);
            for (uint32_t i = 0; i < mod->event_data->event_count; i++) {
                LV2_Midi_Event *item = (LV2_Midi_Event *)data;
                uint32_t ts = item->frames;
                // printf("Event: timestamp %d subframes %d type %d vs %d\n", item->frames, item->subframes, item->type, mod->midi_event_type);
                if (ts > offset)
                {
                    process_slice(mod, offset, ts);
                    offset = ts;
                }
                if (item->type == mod->midi_event_type) 
                {
                    // printf("Midi message %x %x %x %x %d\n", item->data[0], item->data[1], item->data[2], item->data[3], item->size);
                    switch(item->data[0] >> 4)
                    {
                    case 8: mod->note_off(item->data[1], item->data[2]); break;
                    case 9: mod->note_on(item->data[1], item->data[2]); break;
                    case 11: mod->control_change(item->data[1], item->data[2]); break;
                    case 12: mod->program_change(item->data[1]); break;
                    case 14: mod->pitch_bend(item->data[1] + 128 * item->data[2] - 8192); break;
                    }
                }
                else
                if (item->type == 0 && mod->event_feature)
                    mod->event_feature->lv2_event_unref(mod->event_feature->callback_data, item);
                // printf("timestamp %f item size %d first byte %x\n", item->timestamp, item->size, item->data[0]);
                data += ((sizeof(LV2_Event) + item->size + 7))&~7;
            }
        }
        process_slice(mod, offset, SampleCount);
    }
    static void cb_cleanup(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    static const void *cb_ext_data(const char *URI) {
        if (!strcmp(URI, "http://foltman.com/ns/calf-plugin-instance"))
            return &calf_descriptor;
        if (!strcmp(URI, LV2_CONTEXT_MESSAGE))
            return &message_context;
        return NULL;
    }
    static lv2_wrapper &get() { 
        static lv2_wrapper instance;
        return instance;
    }
};

};

#endif
#endif
