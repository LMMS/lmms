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
#include <calf/lv2_event.h>
#include <calf/lv2_state.h>
#include <calf/lv2_progress.h>
#include <calf/lv2_uri_map.h>
#include <string.h>

namespace calf_plugins {

struct lv2_instance: public plugin_ctl_iface, public progress_report_iface
{
    const plugin_metadata_iface *metadata;
    audio_module_iface *module;
    bool set_srate;
    int srate_to_set;
    LV2_Event_Buffer *event_data;
    LV2_URI_Map_Feature *uri_map;
    LV2_Event_Feature *event_feature;
    uint32_t midi_event_type;
    LV2_Progress *progress_report_feature;
    float **ins, **outs, **params;
    int out_count;
    int real_param_count;
    lv2_instance(audio_module_iface *_module)
    {
        module = _module;
        module->get_port_arrays(ins, outs, params);
        metadata = module->get_metadata_iface();
        out_count = metadata->get_output_count();
        real_param_count = metadata->get_param_count();
        
        uri_map = NULL;
        event_data = NULL;
        progress_report_feature = NULL;
        midi_event_type = 0xFFFFFFFF;

        srate_to_set = 44100;
        set_srate = true;
    }
    /// This, and not Module::post_instantiate, is actually called by lv2_wrapper class
    void post_instantiate()
    {
        if (progress_report_feature)
            module->set_progress_report_iface(this);
        module->post_instantiate();
    }
    virtual bool activate_preset(int bank, int program) { 
        return false;
    }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        module->execute(cmd_no);
    }
    virtual void report_progress(float percentage, const std::string &message) {
        if (progress_report_feature)
            (*progress_report_feature->progress)(progress_report_feature->context, percentage, !message.empty() ? message.c_str() : NULL);
    }
    void send_configures(send_configure_iface *sci) { 
        module->send_configures(sci);
    }
    void impl_restore(LV2_State_Retrieve_Function retrieve, void *callback_data)
    {
        const char *const *vars = module->get_metadata_iface()->get_configure_vars();
        if (!vars)
            return;
        assert(uri_map);
        uint32_t string_type = uri_map->uri_to_id(uri_map->callback_data, NULL, "http://lv2plug.in/ns/ext/atom#String");
        assert(string_type);
        for (unsigned int i = 0; vars[i]; i++)
        {
            const uint32_t key   = uri_map->uri_to_id(uri_map->callback_data, NULL, vars[i]);
            size_t         len   = 0;
            uint32_t       type  = 0;
            uint32_t       flags = 0;
            const void *ptr = (*retrieve)(callback_data, key, &len, &type, &flags);
            if (ptr)
            {
                if (type != string_type)
                    fprintf(stderr, "Warning: type is %d, expected %d\n", (int)type, (int)string_type);
                printf("Calling configure on %s\n", vars[i]);
                configure(vars[i], std::string((const char *)ptr, len).c_str());
            }
            else
                configure(vars[i], NULL);
        }
    }
    char *configure(const char *key, const char *value) { 
        // disambiguation - the plugin_ctl_iface version is just a stub, so don't use it
        return module->configure(key, value);
    }
    
    void process_events(uint32_t &offset) {
        struct LV2_Midi_Event: public LV2_Event {
            unsigned char data[1];
        };
        unsigned char *data = (unsigned char *)(event_data->data);
        for (uint32_t i = 0; i < event_data->event_count; i++) {
            LV2_Midi_Event *item = (LV2_Midi_Event *)data;
            uint32_t ts = item->frames;
            // printf("Event: timestamp %d subframes %d type %d vs %d\n", item->frames, item->subframes, item->type, mod->midi_event_type);
            if (ts > offset)
            {
                module->process_slice(offset, ts);
                offset = ts;
            }
            if (item->type == midi_event_type) 
            {
                // printf("Midi message %x %x %x %x %d\n", item->data[0], item->data[1], item->data[2], item->data[3], item->size);
                int channel = item->data[0] & 15;
                switch(item->data[0] >> 4)
                {
                case 8: module->note_off(channel, item->data[1], item->data[2]); break;
                case 9: module->note_on(channel, item->data[1], item->data[2]); break;
                case 11: module->control_change(channel, item->data[1], item->data[2]); break;
                case 12: module->program_change(channel, item->data[1]); break;
                case 13: module->channel_pressure(channel, item->data[1]); break;
                case 14: module->pitch_bend(channel, item->data[1] + 128 * item->data[2] - 8192); break;
                }
            }
            else
            if (item->type == 0 && event_feature)
                event_feature->lv2_event_unref(event_feature->callback_data, item);
            // printf("timestamp %f item size %d first byte %x\n", item->timestamp, item->size, item->data[0]);
            data += ((sizeof(LV2_Event) + item->size + 7))&~7;
        }
    }

    virtual float get_param_value(int param_no)
    {
        // XXXKF hack
        if (param_no >= real_param_count)
            return 0;
        return (*params)[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        // XXXKF hack
        if (param_no >= real_param_count)
            return;
        *params[param_no] = value;
    }
    virtual const plugin_metadata_iface *get_metadata_iface() const { return metadata; }
    virtual const line_graph_iface *get_line_graph_iface() const { return module->get_line_graph_iface(); }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { return module->send_status_updates(sui, last_serial); }
};

struct LV2_Calf_Descriptor {
    plugin_ctl_iface *(*get_pci)(LV2_Handle Instance);
};

template<class Module>
struct lv2_wrapper
{
    typedef lv2_instance instance;
    static LV2_Descriptor descriptor;
    static LV2_Calf_Descriptor calf_descriptor;
    static LV2_State_Interface state_iface;
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
        state_iface.save = cb_state_save;
        state_iface.restore = cb_state_restore;
        calf_descriptor.get_pci = cb_get_pci;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation)
    {
        instance *const mod = (instance *)Instance;
        const plugin_metadata_iface *md = mod->metadata;
        unsigned long ins = md->get_input_count();
        unsigned long outs = md->get_output_count();
        unsigned long params = md->get_param_count();
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
        else if (md->get_midi() && port == ins + outs + params) {
            mod->event_data = (LV2_Event_Buffer *)DataLocation;
        }
    }

    static void cb_activate(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        mod->set_srate = true;
    }
    
    static void cb_deactivate(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        mod->module->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance(new Module);
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
    static plugin_ctl_iface *cb_get_pci(LV2_Handle Instance)
    {
        return static_cast<plugin_ctl_iface *>(Instance);
    }

    static void cb_run(LV2_Handle Instance, uint32_t SampleCount)
    {
        instance *const inst = (instance *)Instance;
        audio_module_iface *mod = inst->module;
        if (inst->set_srate) {
            mod->set_sample_rate(inst->srate_to_set);
            mod->activate();
            inst->set_srate = false;
        }
        mod->params_changed();
        uint32_t offset = 0;
        if (inst->event_data)
        {
            inst->process_events(offset);
        }
        inst->module->process_slice(offset, SampleCount);
    }
    static void cb_cleanup(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    static const void *cb_ext_data(const char *URI)
    {
        if (!strcmp(URI, "http://foltman.com/ns/calf-plugin-instance"))
            return &calf_descriptor;
        if (!strcmp(URI, LV2_STATE_INTERFACE_URI))
            return &state_iface;
        return NULL;
    }
    static void cb_state_save(LV2_Handle Instance,
                          LV2_State_Store_Function store, LV2_State_Handle handle,
                          uint32_t flags, const LV2_Feature *const * features)
    {
        instance *const inst = (instance *)Instance;
        struct store_state: public send_configure_iface
        {
            LV2_State_Store_Function store;
            void *callback_data;
            instance *inst;
            uint32_t string_data_type;
            
            virtual void send_configure(const char *key, const char *value)
            {
                (*store)(callback_data,
                         inst->uri_map->uri_to_id(inst->uri_map->callback_data, NULL, key),
                         value,
                         strlen(value) + 1,
                         string_data_type,
                         LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
            }
        };
        // A host that supports State MUST support URI-Map as well.
        assert(inst->uri_map);
        store_state s;
        s.store = store;
        s.callback_data = handle;
        s.inst = inst;
        s.string_data_type = inst->uri_map->uri_to_id(inst->uri_map->callback_data, NULL, "http://lv2plug.in/ns/ext/atom#String");

        inst->send_configures(&s);
    }
    static void cb_state_restore(LV2_Handle Instance,
                                 LV2_State_Retrieve_Function retrieve, LV2_State_Handle callback_data,
                                 uint32_t flags, const LV2_Feature *const * features)
    {
        instance *const inst = (instance *)Instance;
        inst->impl_restore(retrieve, callback_data);
    }
    
    static lv2_wrapper &get() { 
        static lv2_wrapper *instance = new lv2_wrapper;
        return *instance;
    }
};

};

#endif
#endif
