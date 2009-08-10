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

#include <ladspa.h>
#if USE_DSSI
#include <dssi.h>
#endif
#include "giface.h"

namespace calf_plugins {

template<class Module>
inline int calc_real_param_count()
{
    for (int i=0; i < Module::param_count; i++)
    {
        if ((Module::param_props[i].flags & PF_TYPEMASK) >= PF_STRING)
            return i;
    }
    return Module::param_count;
}

/// A template implementing plugin_ctl_iface for a given plugin
template<class Module>
struct ladspa_instance: public Module, public plugin_ctl_iface
{
    bool activate_flag;
#if USE_DSSI
    dssi_feedback_sender *feedback_sender;
#endif
    
    static int real_param_count()
    {
        static int _real_param_count = calc_real_param_count<Module>();
        return _real_param_count;
    }
    ladspa_instance()
    {
        for (int i=0; i < Module::in_count; i++)
            Module::ins[i] = NULL;
        for (int i=0; i < Module::out_count; i++)
            Module::outs[i] = NULL;
        int rpc = real_param_count();
        for (int i=0; i < rpc; i++)
            Module::params[i] = NULL;
        activate_flag = true;
#if USE_DSSI
        feedback_sender = NULL;
#endif
    }
    virtual parameter_properties *get_param_props(int param_no)
    {
        return &Module::param_props[param_no];
    }
    virtual float get_param_value(int param_no)
    {
        // XXXKF hack
        if (param_no >= real_param_count())
            return 0;
        return *Module::params[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        // XXXKF hack
        if (param_no >= real_param_count())
            return;
        *Module::params[param_no] = value;
    }
    virtual int get_param_count()
    {
        return real_param_count();
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
    virtual char *configure(const char *key, const char *value)
    {
#if USE_DSSI
        if (!strcmp(key, "OSC:FEEDBACK_URI"))
        {
            line_graph_iface *lgi = dynamic_cast<line_graph_iface *>(this);
            if (!lgi)
                return NULL;
            if (*value)
            {
                if (feedback_sender) {
                    delete feedback_sender;
                    feedback_sender = NULL;
                }
                feedback_sender = new dssi_feedback_sender(value, lgi, get_param_props(0), get_param_count());
            }
            else
            {
                if (feedback_sender) {
                    delete feedback_sender;
                    feedback_sender = NULL;
                }
            }
            return NULL;
        }
        else 
        if (!strcmp(key, "OSC:UPDATE"))
        {
            if (feedback_sender)
                feedback_sender->update();
            return NULL;
        }
        else
#endif
        if (!strcmp(key, "ExecCommand"))
        {
            if (*value)
            {
                execute(atoi(value));
            }
            return NULL;
        }
        return Module::configure(key, value);
    }
    virtual int get_input_count() { return Module::in_count; }
    virtual int get_output_count() { return Module::out_count; }
    virtual bool get_midi() { return Module::support_midi; }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        Module::execute(cmd_no);
    }
    virtual void send_configures(send_configure_iface *sci) { 
        Module::send_configures(sci);
    }
};

/// A wrapper class for plugin class object (there is only one ladspa_wrapper for many instances of the same plugin)
template<class Module>
struct ladspa_wrapper
{
    typedef ladspa_instance<Module> instance;
    
    /// LADSPA descriptor
    static LADSPA_Descriptor descriptor;
    /// LADSPA descriptor for DSSI (uses a different name for the plugin, otherwise same as descriptor)
    static LADSPA_Descriptor descriptor_for_dssi;
#if USE_DSSI
    /// Extended DSSI descriptor (points to descriptor_for_dssi for things like name/label/port info etc.)
    static DSSI_Descriptor dssi_descriptor;
    static DSSI_Program_Descriptor dssi_default_program;

    static std::vector<plugin_preset> *presets;
    static std::vector<DSSI_Program_Descriptor> *preset_descs;
#endif
    
    ladspa_wrapper() 
    {
        int ins = Module::in_count;
        int outs = Module::out_count;
        int params = ladspa_instance<Module>::real_param_count();
        ladspa_plugin_info &plugin_info = Module::plugin_info;
        descriptor.UniqueID = plugin_info.unique_id;
        descriptor.Label = plugin_info.label;
        descriptor.Name = strdup((std::string(plugin_info.name) + " LADSPA").c_str());
        descriptor.Maker = plugin_info.maker;
        descriptor.Copyright = plugin_info.copyright;
        descriptor.Properties = Module::rt_capable ? LADSPA_PROPERTY_HARD_RT_CAPABLE : 0;
        descriptor.PortCount = ins + outs + params;
        descriptor.PortNames = new char *[descriptor.PortCount];
        descriptor.PortDescriptors = new LADSPA_PortDescriptor[descriptor.PortCount];
        descriptor.PortRangeHints = new LADSPA_PortRangeHint[descriptor.PortCount];
        int i;
        for (i = 0; i < ins + outs; i++)
        {
            LADSPA_PortRangeHint &prh = ((LADSPA_PortRangeHint *)descriptor.PortRangeHints)[i];
            ((int *)descriptor.PortDescriptors)[i] = i < ins ? LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO
                                                  : i < ins + outs ? LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
                                                                   : LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
            prh.HintDescriptor = 0;
            ((const char **)descriptor.PortNames)[i] = Module::port_names[i];
        }
        for (; i < ins + outs + params; i++)
        {
            LADSPA_PortRangeHint &prh = ((LADSPA_PortRangeHint *)descriptor.PortRangeHints)[i];
            parameter_properties &pp = Module::param_props[i - ins - outs];
            ((int *)descriptor.PortDescriptors)[i] = 
                LADSPA_PORT_CONTROL | (pp.flags & PF_PROP_OUTPUT ? LADSPA_PORT_OUTPUT : LADSPA_PORT_INPUT);
            prh.HintDescriptor = LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW;
            ((const char **)descriptor.PortNames)[i] = pp.name;
            prh.LowerBound = pp.min;
            prh.UpperBound = pp.max;
            switch(pp.flags & PF_TYPEMASK) {
                case PF_BOOL: 
                    prh.HintDescriptor |= LADSPA_HINT_TOGGLED;
                    prh.HintDescriptor &= ~(LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW);
                    break;
                case PF_INT: 
                case PF_ENUM: 
                    prh.HintDescriptor |= LADSPA_HINT_INTEGER;
                    break;
                default: {
                    int defpt = (int)(100 * (pp.def_value - pp.min) / (pp.max - pp.min));
                    if ((pp.flags & PF_SCALEMASK) == PF_SCALE_LOG)
                        defpt = (int)(100 * log(pp.def_value / pp.min) / log(pp.max / pp.min));
                    if (defpt < 12)
                        prh.HintDescriptor |= LADSPA_HINT_DEFAULT_MINIMUM;
                    else if (defpt < 37)
                        prh.HintDescriptor |= LADSPA_HINT_DEFAULT_LOW;
                    else if (defpt < 63)
                        prh.HintDescriptor |= LADSPA_HINT_DEFAULT_MIDDLE;
                    else if (defpt < 88)
                        prh.HintDescriptor |= LADSPA_HINT_DEFAULT_HIGH;
                    else
                        prh.HintDescriptor |= LADSPA_HINT_DEFAULT_MAXIMUM;
                }
            }
            if (pp.def_value == 0 || pp.def_value == 1 || pp.def_value == 100 || pp.def_value == 440 ) {
                prh.HintDescriptor &= ~LADSPA_HINT_DEFAULT_MASK;
                if (pp.def_value == 1)
                    prh.HintDescriptor |= LADSPA_HINT_DEFAULT_1;
                else if (pp.def_value == 100)
                    prh.HintDescriptor |= LADSPA_HINT_DEFAULT_100;
                else if (pp.def_value == 440)
                    prh.HintDescriptor |= LADSPA_HINT_DEFAULT_440;
                else
                    prh.HintDescriptor |= LADSPA_HINT_DEFAULT_0;
            }
            switch(pp.flags & PF_SCALEMASK) {
                case PF_SCALE_LOG:
                    prh.HintDescriptor |= LADSPA_HINT_LOGARITHMIC;
                    break;
            }
        }
        descriptor.ImplementationData = this;
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.run_adding = NULL;
        descriptor.set_run_adding_gain = NULL;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
#if USE_DSSI
        memcpy(&descriptor_for_dssi, &descriptor, sizeof(descriptor));
        descriptor_for_dssi.Name = strdup((std::string(plugin_info.name) + " DSSI").c_str());
        memset(&dssi_descriptor, 0, sizeof(dssi_descriptor));
        dssi_descriptor.DSSI_API_Version = 1;
        dssi_descriptor.LADSPA_Plugin = &descriptor_for_dssi;
        dssi_descriptor.configure = cb_configure;
        dssi_descriptor.get_program = cb_get_program;
        dssi_descriptor.select_program = cb_select_program;
        if (Module::support_midi)
            dssi_descriptor.run_synth = cb_run_synth;
        
        presets = new std::vector<plugin_preset>;
        preset_descs = new std::vector<DSSI_Program_Descriptor>;

        preset_list plist_tmp, plist;
        plist.load_defaults(true);
        plist_tmp.load_defaults(false);
        plist.presets.insert(plist.presets.end(), plist_tmp.presets.begin(), plist_tmp.presets.end());
        
        // XXXKF this assumes that plugin name in preset is case-insensitive equal to plugin label
        // if I forget about this, I'll be in a deep trouble
        dssi_default_program.Bank = 0;
        dssi_default_program.Program = 0;
        dssi_default_program.Name = "default";

        int pos = 1;
        for (unsigned int i = 0; i < plist.presets.size(); i++)
        {
            plugin_preset &pp = plist.presets[i];
            if (strcasecmp(pp.plugin.c_str(), descriptor.Label))
                continue;
            DSSI_Program_Descriptor pd;
            pd.Bank = pos >> 7;
            pd.Program = pos++;
            pd.Name = pp.name.c_str();
            preset_descs->push_back(pd);
            presets->push_back(pp);
        }
        // printf("presets = %p:%d name = %s\n", presets, presets->size(), descriptor.Label);
        
#endif
    }

    ~ladspa_wrapper()
    {
        delete []descriptor.PortNames;
        delete []descriptor.PortDescriptors;
        delete []descriptor.PortRangeHints;
#if USE_DSSI
        presets->clear();
        preset_descs->clear();
        delete presets;
        delete preset_descs;
#endif
    }

    /// LADSPA instantiation function (create a plugin instance)
    static LADSPA_Handle cb_instantiate(const struct _LADSPA_Descriptor * Descriptor, unsigned long sample_rate)
    {
        instance *mod = new instance();
        mod->set_sample_rate(sample_rate);
        mod->post_instantiate();
        return mod;
    }

#if USE_DSSI
    /// DSSI get program descriptor function; for 0, it returns the default program (from parameter properties table), for others, it uses global or user preset
    static const DSSI_Program_Descriptor *cb_get_program(LADSPA_Handle Instance, unsigned long index) {
        if (index > presets->size())
            return NULL;
        if (index)
            return &(*preset_descs)[index - 1];
        return &dssi_default_program;
    }
    
    /// DSSI select program function; for 0, it sets the defaults, for others, it sets global or user preset
    static void cb_select_program(LADSPA_Handle Instance, unsigned long Bank, unsigned long Program) {
        instance *mod = (instance *)Instance;
        unsigned int no = (Bank << 7) + Program - 1;
        // printf("no = %d presets = %p:%d\n", no, presets, presets->size());
        if (no == -1U) {
            int rpc = ladspa_instance<Module>::real_param_count();
            for (int i =0 ; i < rpc; i++)
                *mod->params[i] = Module::param_props[i].def_value;
            return;
        }
        if (no >= presets->size())
            return;
        plugin_preset &p = (*presets)[no];
        // printf("activating preset %s\n", p.name.c_str());
        p.activate(mod);
    }
    
#endif
    
    /// LADSPA port connection function
    static void cb_connect(LADSPA_Handle Instance, unsigned long port, LADSPA_Data *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        unsigned long params = ladspa_instance<Module>::real_param_count();
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = DataLocation;
            *mod->params[i] = Module::param_props[i].def_value;
        }
    }

    /// LADSPA activate function (note that at this moment the ports are not set)
    static void cb_activate(LADSPA_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->activate_flag = true;
    }
    
    /// utility function: zero port values if mask is 0
    static inline void zero_by_mask(Module *module, uint32_t mask, uint32_t offset, uint32_t nsamples)
    {
        for (int i=0; i<Module::out_count; i++) {
            if ((mask & (1 << i)) == 0) {
                dsp::zero(module->outs[i] + offset, nsamples);
            }
        }
    }

    /// LADSPA run function - does set sample rate / activate logic when it's run first time after activation
    static void cb_run(LADSPA_Handle Instance, unsigned long SampleCount) {
        instance *const mod = (instance *)Instance;
        if (mod->activate_flag)
        {
            mod->activate();
            mod->activate_flag = false;
        }
        mod->params_changed();
        process_slice(mod, 0, SampleCount);
    }
    
    /// utility function: call process, and if it returned zeros in output masks, zero out the relevant output port buffers
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

#if USE_DSSI
    /// DSSI "run synth" function, same as run() except it allows for event delivery
    static void cb_run_synth(LADSPA_Handle Instance, unsigned long SampleCount, 
            snd_seq_event_t *Events, unsigned long EventCount) {
        instance *const mod = (instance *)Instance;
        if (mod->activate_flag)
        {
            mod->activate();
            mod->activate_flag = false;
        }
        mod->params_changed();
        
        uint32_t offset = 0;
        for (uint32_t e = 0; e < EventCount; e++)
        {
            uint32_t timestamp = Events[e].time.tick;
            if (timestamp != offset)
                process_slice(mod, offset, timestamp);
            process_dssi_event(mod, Events[e]);
            offset = timestamp;
        }
        if (offset != SampleCount)
            process_slice(mod, offset, SampleCount);
    }
    
    /// DSSI configure function (named properties)
    static char *cb_configure(LADSPA_Handle Instance,
		       const char *Key,
		       const char *Value)
    {
        instance *const mod = (instance *)Instance;
        return mod->configure(Key, Value);
    }
    
    /// Utility function: handle MIDI event (only handles a subset in this version)
    static void process_dssi_event(Module *module, snd_seq_event_t &event)
    {
        switch(event.type) {
            case SND_SEQ_EVENT_NOTEON:
                module->note_on(event.data.note.note, event.data.note.velocity);
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                module->note_off(event.data.note.note, event.data.note.velocity);
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                module->program_change(event.data.control.value);
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                module->control_change(event.data.control.param, event.data.control.value);
                break;
            case SND_SEQ_EVENT_PITCHBEND:
                module->pitch_bend(event.data.control.value);
                break;
        }
    }
#endif

    /// LADSPA deactivate function
    static void cb_deactivate(LADSPA_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }

    /// LADSPA cleanup (delete instance) function
    static void cb_cleanup(LADSPA_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    
    /// Get a wrapper singleton - used to prevent initialization order problems which were present in older versions
    static ladspa_wrapper &get() { 
        static ladspa_wrapper instance;
        return instance;
    }
};

};

#endif

#endif
