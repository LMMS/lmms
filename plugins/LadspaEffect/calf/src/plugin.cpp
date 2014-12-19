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
#include <calf/ladspa_wrap.h>
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if USE_LADSPA

ladspa_instance::ladspa_instance(audio_module_iface *_module, ladspa_plugin_metadata_set *_ladspa, int sample_rate)
{
    module = _module;
    metadata = module->get_metadata_iface();
    ladspa = _ladspa;
    
    module->get_port_arrays(ins, outs, params);
    
    activate_flag = true;
#if USE_DSSI
    feedback_sender = NULL;
#endif

    module->set_sample_rate(sample_rate);
    module->post_instantiate();
}

ladspa_instance::~ladspa_instance()
{
	delete module;
}

float ladspa_instance::get_param_value(int param_no)
{
    // XXXKF hack
    if (param_no >= ladspa->param_count)
        return 0;
    return *params[param_no];
}

void ladspa_instance::set_param_value(int param_no, float value)
{
    // XXXKF hack
    if (param_no >= ladspa->param_count)
        return;
    *params[param_no] = value;
}

bool ladspa_instance::activate_preset(int bank, int program)
{
    return false;
}

/// LADSPA run function - does set sample rate / activate logic when it's run first time after activation
void ladspa_instance::run(unsigned long SampleCount)
{
    if (activate_flag)
    {
        module->activate();
        activate_flag = false;
    }
    module->params_changed();
    module->process_slice(0, SampleCount);
}

#if USE_DSSI

void ladspa_instance::run_synth(unsigned long SampleCount, snd_seq_event_t *Events, unsigned long EventCount)
{
    if (activate_flag)
    {
        module->activate();
        activate_flag = false;
    }
    module->params_changed();
    
    uint32_t offset = 0;
    for (uint32_t e = 0; e < EventCount; e++)
    {
        uint32_t timestamp = Events[e].time.tick;
        if (timestamp != offset)
            module->process_slice(offset, timestamp);
        process_dssi_event(Events[e]);
        offset = timestamp;
    }
    if (offset != SampleCount)
        module->process_slice(offset, SampleCount);
}

#endif

char *ladspa_instance::configure(const char *key, const char *value)
{
#if USE_DSSI_GUI
    if (!strcmp(key, "OSC:FEEDBACK_URI"))
    {
        const line_graph_iface *lgi = dynamic_cast<const line_graph_iface *>(metadata);
        //if (!lgi)
        //    return NULL;
        if (*value)
        {
            if (feedback_sender) {
                delete feedback_sender;
                feedback_sender = NULL;
            }
            feedback_sender = new dssi_feedback_sender(value, lgi);
            feedback_sender->add_graphs(metadata->get_param_props(0), metadata->get_param_count());
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
    if (!strcmp(key, "OSC:SEND_STATUS"))
    {
        if (!feedback_sender)
            return NULL;
        struct status_gatherer: public send_updates_iface
        {
            osc_inline_typed_strstream str;            
            void send_status(const char *key, const char *value)
            {
                str << key << value;
            }
        } sg;
        int serial = atoi(value);
        serial = module->send_status_updates(&sg, serial);
        sg.str << (uint32_t)serial;
        feedback_sender->client->send("/status_data", sg.str);
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
    return module->configure(key, value);
}

template<class Module>
ladspa_plugin_metadata_set ladspa_wrapper<Module>::output;

#if USE_DSSI

/// Utility function: handle MIDI event (only handles a subset in this version)
void ladspa_instance::process_dssi_event(snd_seq_event_t &event)
{
    switch(event.type) {
        case SND_SEQ_EVENT_NOTEON:
            module->note_on(event.data.note.channel, event.data.note.note, event.data.note.velocity);
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            module->note_off(event.data.note.channel, event.data.note.note, event.data.note.velocity);
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            module->program_change(event.data.control.channel, event.data.control.value);
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            module->control_change(event.data.control.channel, event.data.control.param, event.data.control.value);
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            module->pitch_bend(event.data.control.channel, event.data.control.value);
            break;
        case SND_SEQ_EVENT_CHANPRESS:
            module->channel_pressure(event.data.control.channel, event.data.control.value);
            break;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LADSPA callbacks

/// LADSPA activate function (note that at this moment the ports are not set)
static void cb_activate(LADSPA_Handle Instance)
{
    ((ladspa_instance *)(Instance))->activate_flag = true;
}

/// LADSPA run function - does set sample rate / activate logic when it's run first time after activation
static void cb_run(LADSPA_Handle Instance, unsigned long SampleCount) {
    ((ladspa_instance *)(Instance))->run(SampleCount);
}

/// LADSPA port connection function
static void cb_connect(LADSPA_Handle Instance, unsigned long port, LADSPA_Data *DataLocation)
{
    ladspa_instance *const mod = (ladspa_instance *)Instance;
    
    int first_out = mod->ladspa->input_count;
    int first_param = first_out + mod->ladspa->output_count;
    int ladspa_port_count = first_param + mod->ladspa->param_count;
    
    if ((int)port < first_out)
        mod->ins[port] = DataLocation;
    else if ((int)port < first_param)
        mod->outs[port - first_out] = DataLocation;
    else if ((int)port < ladspa_port_count) {
        int i = port - first_param;
        mod->params[i] = DataLocation;
        *mod->params[i] = mod->metadata->get_param_props(i)->def_value;
    }
}


/// LADSPA deactivate function
static void cb_deactivate(LADSPA_Handle Instance) {
    ((ladspa_instance *)(Instance))->module->deactivate();
}

/// LADSPA cleanup (delete instance) function
static void cb_cleanup(LADSPA_Handle Instance) {
    delete ((ladspa_instance *)(Instance));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DSSI callbacks

#if USE_DSSI
/// DSSI "run synth" function, same as run() except it allows for event delivery
static void cb_run_synth(LADSPA_Handle Instance, unsigned long SampleCount, 
        snd_seq_event_t *Events, unsigned long EventCount) {
    ((ladspa_instance *)(Instance))->run_synth(SampleCount, Events, EventCount);
}

/// DSSI configure function (named properties)
static char *cb_configure(LADSPA_Handle Instance,
                   const char *Key,
                   const char *Value)
{
    return ((ladspa_instance *)(Instance))->configure(Key, Value);
}

/// DSSI get program descriptor function; for 0, it returns the default program (from parameter properties table), for others, it uses global or user preset
static const DSSI_Program_Descriptor *cb_get_program(LADSPA_Handle Instance, unsigned long index)
{
    ladspa_plugin_metadata_set *ladspa = ((ladspa_instance *)(Instance))->ladspa;
    if (index > ladspa->presets->size())
        return NULL;
    if (index)
        return &(*ladspa->preset_descs)[index - 1];
    return &ladspa->dssi_default_program;
}

/// DSSI select program function; for 0, it sets the defaults, for others, it sets global or user preset
static void cb_select_program(LADSPA_Handle Instance, unsigned long Bank, unsigned long Program)
{
    ladspa_instance *mod = (ladspa_instance *)Instance;
    ladspa_plugin_metadata_set *ladspa = mod->ladspa;
    unsigned int no = (Bank << 7) + Program - 1;
    // printf("no = %d presets = %p:%d\n", no, presets, presets->size());
    if (no == -1U) {
        int rpc = ladspa->param_count;
        for (int i =0 ; i < rpc; i++)
            *mod->params[i] = mod->metadata->get_param_props(i)->def_value;
        return;
    }
    if (no >= ladspa->presets->size())
        return;
    plugin_preset &p = (*ladspa->presets)[no];
    // printf("activating preset %s\n", p.name.c_str());
    p.activate(mod);
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

ladspa_plugin_metadata_set::ladspa_plugin_metadata_set()
{
    metadata = NULL;
    memset(&descriptor, 0, sizeof(descriptor));

#if USE_DSSI
    presets = NULL;
    preset_descs = NULL;
    memset(&descriptor_for_dssi, 0, sizeof(descriptor_for_dssi));
    memset(&dssi_descriptor, 0, sizeof(dssi_descriptor));
#endif
}

void ladspa_plugin_metadata_set::prepare(const plugin_metadata_iface *md, LADSPA_Handle (*cb_instantiate)(const struct _LADSPA_Descriptor * Descriptor, unsigned long sample_rate))
{
    metadata = md;
    
    input_count = md->get_input_count();
    output_count = md->get_output_count();
    param_count = md->get_param_count(); // XXXKF ladspa_instance<Module>::real_param_count();
    
    const ladspa_plugin_info &plugin_info = md->get_plugin_info();
    descriptor.UniqueID = plugin_info.unique_id;
    descriptor.Label = plugin_info.label;
    descriptor.Name = strdup((std::string(plugin_info.name) + " LADSPA").c_str());
    descriptor.Maker = plugin_info.maker;
    descriptor.Copyright = plugin_info.copyright;
    descriptor.Properties = md->is_rt_capable() ? LADSPA_PROPERTY_HARD_RT_CAPABLE : 0;
    descriptor.PortCount = input_count + output_count + param_count;
    descriptor.PortNames = new char *[descriptor.PortCount];
    descriptor.PortDescriptors = new LADSPA_PortDescriptor[descriptor.PortCount];
    descriptor.PortRangeHints = new LADSPA_PortRangeHint[descriptor.PortCount];
    int i;
    for (i = 0; i < input_count + output_count; i++)
    {
        LADSPA_PortRangeHint &prh = ((LADSPA_PortRangeHint *)descriptor.PortRangeHints)[i];
        ((int *)descriptor.PortDescriptors)[i] = i < input_count ? LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO
                                              : LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        prh.HintDescriptor = 0;
        ((const char **)descriptor.PortNames)[i] = md->get_port_names()[i];
    }
    for (; i < input_count + output_count + param_count; i++)
    {
        LADSPA_PortRangeHint &prh = ((LADSPA_PortRangeHint *)descriptor.PortRangeHints)[i];
        const parameter_properties &pp = *md->get_param_props(i - input_count - output_count);
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
    prepare_dssi();
}

void ladspa_plugin_metadata_set::prepare_dssi()
{
#if USE_DSSI
    const ladspa_plugin_info &plugin_info = metadata->get_plugin_info();
    memcpy(&descriptor_for_dssi, &descriptor, sizeof(descriptor));
    descriptor_for_dssi.Name = strdup((std::string(plugin_info.name) + " DSSI").c_str());
    memset(&dssi_descriptor, 0, sizeof(dssi_descriptor));
    dssi_descriptor.DSSI_API_Version = 1;
    dssi_descriptor.LADSPA_Plugin = &descriptor_for_dssi;
    dssi_descriptor.configure = cb_configure;
    dssi_descriptor.get_program = cb_get_program;
    dssi_descriptor.select_program = cb_select_program;
    if (metadata->get_midi())
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
#endif
}

ladspa_plugin_metadata_set::~ladspa_plugin_metadata_set()
{
    delete []descriptor.PortNames;
    delete []descriptor.PortDescriptors;
    delete []descriptor.PortRangeHints;
#if USE_DSSI
    if (presets)
        presets->clear();
    if (preset_descs)
        preset_descs->clear();
    delete presets;
    delete preset_descs;
#endif
}

#endif

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
