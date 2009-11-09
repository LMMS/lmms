/* Calf DSP Library
 * Common plugin interface definitions (shared between LADSPA/LV2/DSSI/standalone).
 *
 * Copyright (C) 2007 Krzysztof Foltman
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
#ifndef __CALF_GIFACE_H
#define __CALF_GIFACE_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <exception>
#include <string>
#include "primitives.h"
#include "preset.h"

namespace osctl {
    struct osc_client;
}

namespace calf_plugins {

enum {
    MAX_SAMPLE_RUN = 256
};
    
/// Values ORed together for flags field in parameter_properties
enum parameter_flags
{
  PF_TYPEMASK = 0x000F, ///< bit mask for type
  PF_FLOAT = 0x0000, ///< any float value
  PF_INT = 0x0001,   ///< integer value (still represented as float)
  PF_BOOL = 0x0002,  ///< bool value (usually >=0.5f is treated as TRUE, which is inconsistent with LV2 etc. which treats anything >0 as TRUE)
  PF_ENUM = 0x0003,  ///< enum value (min, min+1, ..., max, only guaranteed to work when min = 0)
  PF_ENUM_MULTI = 0x0004, ///< SET / multiple-choice
  PF_STRING = 0x0005, ///< see: http://lv2plug.in/docs/index.php?title=String_port
  
  PF_SCALEMASK = 0xF0, ///< bit mask for scale
  PF_SCALE_DEFAULT = 0x00, ///< no scale given
  PF_SCALE_LINEAR = 0x10, ///< linear scale
  PF_SCALE_LOG = 0x20, ///< log scale
  PF_SCALE_GAIN = 0x30, ///< gain = -96dB..0 or -inf dB
  PF_SCALE_PERC = 0x40, ///< percent
  PF_SCALE_QUAD = 0x50, ///< quadratic scale (decent for some gain/amplitude values)
  PF_SCALE_LOG_INF = 0x60, ///< log scale + +inf (FAKE_INFINITY)

  PF_CTLMASK =     0x0F00, ///< bit mask for control type
  PF_CTL_DEFAULT = 0x0000, ///< try to figure out automatically
  PF_CTL_KNOB =    0x0100, ///< knob
  PF_CTL_FADER =   0x0200, ///< fader (slider)
  PF_CTL_TOGGLE =  0x0300, ///< toggle button
  PF_CTL_COMBO =   0x0400, ///< combo box
  PF_CTL_RADIO =   0x0500, ///< radio button
  PF_CTL_BUTTON =  0x0600, ///< push button
  PF_CTL_METER  =  0x0700, ///< volume meter
  PF_CTL_LED    =  0x0800, ///< light emitting diode
  PF_CTL_LABEL  =  0x0900, ///< label
  
  PF_CTLOPTIONS     = 0x00F000, ///< bit mask for control (widget) options
  PF_CTLO_HORIZ     = 0x001000, ///< horizontal version of the control (unused)
  PF_CTLO_VERT      = 0x002000, ///< vertical version of the control (unused)
  PF_CTLO_LABEL     = 0x004000, ///< add a text display to the control (meters only)
  PF_CTLO_REVERSE   = 0x008000, ///< use VU_MONOCHROME_REVERSE mode (meters only)

  PF_PROP_NOBOUNDS =  0x010000, ///< no epp:hasStrictBounds
  PF_PROP_EXPENSIVE = 0x020000, ///< epp:expensive, may trigger expensive calculation
  PF_PROP_OUTPUT_GAIN=0x050000, ///< epp:outputGain + skip epp:hasStrictBounds
  PF_PROP_OUTPUT    = 0x080000, ///< output port
  PF_PROP_OPTIONAL  = 0x100000, ///< connection optional
  PF_PROP_GRAPH     = 0x200000, ///< add graph
  PF_PROP_MSGCONTEXT= 0x400000, ///< message context
  
  PF_UNITMASK     = 0xFF000000,  ///< bit mask for units   \todo reduce to use only 5 bits
  PF_UNIT_DB      = 0x01000000,  ///< decibels
  PF_UNIT_COEF    = 0x02000000,  ///< multiply-by factor
  PF_UNIT_HZ      = 0x03000000,  ///< Hertz
  PF_UNIT_SEC     = 0x04000000,  ///< second
  PF_UNIT_MSEC    = 0x05000000,  ///< millisecond
  PF_UNIT_CENTS   = 0x06000000,  ///< cents (1/100 of a semitone, 1/1200 of an octave)
  PF_UNIT_SEMITONES = 0x07000000,///< semitones
  PF_UNIT_BPM     = 0x08000000,  ///< beats per minute
  PF_UNIT_DEG     = 0x09000000,  ///< degrees
  PF_UNIT_NOTE    = 0x0A000000,  ///< MIDI note number
  PF_UNIT_RPM     = 0x0B000000,  ///< revolutions per minute
};

/// A fake infinity value (because real infinity may break some hosts)
#define FAKE_INFINITY (65536.0 * 65536.0)
/// Check for infinity (with appropriate-ish tolerance)
#define IS_FAKE_INFINITY(value) (fabs(value-FAKE_INFINITY) < 1.0)

/// Information record about plugin's menu command
struct plugin_command_info
{
    const char *label;           ///< short command name / label
    const char *name;            ///< human-readable command name
    const char *description;     ///< description (for status line etc.)
};

/// Range, default value, flags and names for a parameter
struct parameter_properties
{
    /// default value
    float def_value;
    /// minimum value
    float min;
    /// maximum value
    float max;
    /// number of steps (for an integer value from 0 to 100 this will be 101; for 0/90/180/270/360 this will be 5), or 0 for continuous
    float step;
    /// logical OR of parameter_flags
    uint32_t flags;
    /// for PF_ENUM: array of text values (from min to max step 1), otherwise NULL
    const char **choices;
    /// parameter label (for use in LV2 label field etc.)
    const char *short_name;
    /// parameter human-readable name
    const char *name;
    /// convert from [0, 1] range to [min, max] (applying scaling)
    float from_01(double value01) const;
    /// convert from [min, max] to [0, 1] range (applying reverse scaling)
    double to_01(float value) const;
    /// stringify (in sensible way)
    std::string to_string(float value) const;
    /// get required width (for reserving GUI space)
    int get_char_count() const;
    /// get increment step based on step value (if specified) and other factors
    float get_increment() const;
};

struct cairo_iface
{
    virtual void set_source_rgba(float r, float g, float b, float a = 1.f) = 0;
    virtual void set_line_width(float width) = 0;
    virtual ~cairo_iface() {}
};

struct progress_report_iface
{
    virtual void report_progress(float percentage, const std::string &message) = 0;
    virtual ~progress_report_iface() {}
};

/// 'provides live line graph values' interface
struct line_graph_iface
{
    /// Obtain subindex'th graph of parameter 'index'
    /// @param index parameter/graph number (usually tied to particular plugin control port)
    /// @param subindex graph number (there may be multiple overlaid graphs for one parameter, eg. for monosynth 2x12dB filters)
    /// @param data buffer for normalized output values
    /// @param points number of points to fill
    /// @param context cairo context to adjust (for multicolour graphs etc.)
    /// @retval true graph data was returned; subindex+1 graph may or may not be available
    /// @retval false graph data was not returned; subindex+1 graph does not exist either
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) { return false; }

    /// Obtain subindex'th dot of parameter 'index'
    /// @param index parameter/dot number (usually tied to particular plugin control port)
    /// @param subindex dot number (there may be multiple dots graphs for one parameter)
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) { return false; }
    
    /// Obtain subindex'th dot of parameter 'index'
    /// @param index parameter/dot number (usually tied to particular plugin control port)
    /// @param subindex dot number (there may be multiple dots graphs for one parameter)
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) { return false; }
    
    /// Obtain subindex'th static graph of parameter index (static graphs are only dependent on parameter value, not plugin state)
    /// @param index parameter/graph number (usually tied to particular plugin control port)
    /// @param subindex graph number (there may be multiple overlaid graphs for one parameter, eg. for monosynth 2x12dB filters)
    /// @param value parameter value to pick the graph for
    /// @param data buffer for normalized output values
    /// @param points number of points to fill
    /// @param context cairo context to adjust (for multicolour graphs etc.)
    /// @retval true graph data was returned; subindex+1 graph may or may not be available
    /// @retval false graph data was not returned; subindex+1 graph does not exist either
    virtual bool get_static_graph(int index, int subindex, float value, float *data, int points, cairo_iface *context) { return false; }
    
    /// Return which graphs need to be redrawn and which can be cached for later reuse
    /// @param index Parameter/graph number (usually tied to particular plugin control port)
    /// @param generation 0 (at start) or the last value returned by the function (corresponds to a set of input values)
    /// @param subindex_graph First graph that has to be redrawn (because it depends on values that might have changed)
    /// @param subindex_dot First dot that has to be redrawn
    /// @param subindex_gridline First gridline/legend that has to be redrawn
    /// @retval Current generation (to pass when calling the function next time); if different than passed generation value, call the function again to retrieve which graph offsets should be put into cache
    virtual int get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) { subindex_graph = subindex_dot = subindex_gridline = 0; return 0; }
    
    /// Standard destructor to make compiler happy
    virtual ~line_graph_iface() {}
};

enum table_column_type
{
    TCT_UNKNOWN, ///< guard invalid type
    TCT_FLOAT, ///< float value (encoded as C locale string)
    TCT_ENUM, ///< enum value (see: 'values' array in table_column_info) - encoded as string base 10 representation of integer
    TCT_STRING, ///< string value (encoded as C-escaped string)
    TCT_OBJECT, ///< external object, encoded as string
    TCT_LABEL, ///< string value (encoded as C-escaped string)
};

/// parameters of 
struct table_column_info
{
    const char *name; ///< column label
    table_column_type type; ///< column data type
    float min; ///< minimum value (for float)
    float max; ///< maximum value (for float and enum)
    float def_value; ///< default value (for float and enum)
    const char **values; ///< NULL unless a TCT_ENUM, where it represents a NULL-terminated list of choices
};

/// 'has string parameters containing tabular data' interface
struct table_edit_iface
{
    /// retrieve the table layout for specific parameter
    virtual const table_column_info *get_table_columns(int param) = 0;

    /// return the current number of rows
    virtual uint32_t get_table_rows(int param) = 0;
    
    /// retrieve data item from the plugin
    virtual std::string get_cell(int param, int row, int column) { return calf_utils::i2s(row)+":"+calf_utils::i2s(column); }

    /// set data item to the plugin
    virtual void set_cell(int param, int row, int column, const std::string &src, std::string &error) { error.clear(); }
    
    /// return a line graph interface for a specific parameter/column (unused for now)
    virtual line_graph_iface *get_graph_iface(int param, int column) { return NULL; }
    
    /// return an editor name for a specific grid cell (unused for now - I don't even know how editors be implemented)
    virtual const char *get_cell_editor(int param, int column) { return NULL; }
    
    virtual ~table_edit_iface() {}
};

/// 'may receive configure variables' interface
struct send_configure_iface
{
    /// Called to set configure variable
    /// @param key variable name
    /// @param value variable content
    virtual void send_configure(const char *key, const char *value) = 0;
    
    virtual ~send_configure_iface() {}
};

/// 'may receive new status values' interface
struct send_updates_iface
{
    /// Called to set configure variable
    /// @param key variable name
    /// @param value variable content
    virtual void send_status(const char *key, const char *value) = 0;
    
    virtual ~send_updates_iface() {}
};

struct plugin_command_info;

/// General information about the plugin - @todo XXXKF lacks the "new" id-label-name triple
struct ladspa_plugin_info
{
    /// LADSPA ID
    uint32_t unique_id;
    /// plugin short name (camel case)
    const char *label;
    /// plugin human-readable name
    const char *name;
    /// maker (author)
    const char *maker;
    /// copyright notice
    const char *copyright;
    /// plugin type for LRDF/LV2
    const char *plugin_type;
};

/// An interface returning metadata about a plugin
struct plugin_metadata_iface
{
    /// @return plugin long name
    virtual const char *get_name() = 0;
    /// @return plugin LV2 label
    virtual const char *get_id() = 0;
    /// @return plugin human-readable label
    virtual const char *get_label() = 0;
    /// @return total number of parameters
    virtual int get_param_count() = 0;
    /// Return custom XML
    virtual const char *get_gui_xml() = 0;
    /// @return number of audio inputs
    virtual int get_input_count()=0;
    /// @return number of audio outputs
    virtual int get_output_count()=0;
    /// @return true if plugin can work in hard-realtime conditions
    virtual bool is_rt_capable()=0;
    /// @return true if plugin has MIDI input
    virtual bool get_midi()=0;
    /// @return true if plugin has MIDI input
    virtual bool requires_midi()=0;
    /// @return port offset of first control (parameter) port (= number of audio inputs + number of audio outputs in all existing plugins as for 1 Aug 2008)
    virtual int get_param_port_offset() = 0;
    /// @return line_graph_iface if any
    virtual line_graph_iface *get_line_graph_iface() = 0;
    /// @return table_edit_iface if any
    virtual table_edit_iface *get_table_edit_iface() = 0;
    /// @return NULL-terminated list of menu commands
    virtual plugin_command_info *get_commands() { return NULL; }
    /// @return description structure for given parameter
    virtual parameter_properties *get_param_props(int param_no) = 0;
    /// @return retrieve names of audio ports (@note control ports are named in parameter_properties, not here)
    virtual const char **get_port_names() = 0;
    /// @return description structure for the plugin
    virtual const ladspa_plugin_info &get_plugin_info() = 0;
    /// is a given parameter a control voltage?
    virtual bool is_cv(int param_no) = 0;
    /// is the given parameter non-interpolated?
    virtual bool is_noisy(int param_no) = 0;
    /// does the plugin require message context? (or DSSI configure) may be slow
    virtual bool requires_message_context() = 0;
    /// does the plugin require string port extension? (or DSSI configure) may be slow
    virtual bool requires_string_ports() = 0;
    /// add all message context parameter numbers to the ports vector
    virtual void get_message_context_parameters(std::vector<int> &ports) = 0;

    /// Do-nothing destructor to silence compiler warning
    virtual ~plugin_metadata_iface() {}
};

/// Interface for host-GUI-plugin interaction (should be really split in two, but ... meh)
struct plugin_ctl_iface: public virtual plugin_metadata_iface
{
    /// @return value of given parameter
    virtual float get_param_value(int param_no) = 0;
    /// Set value of given parameter
    virtual void set_param_value(int param_no, float value) = 0;
    /// Load preset with given number
    virtual bool activate_preset(int bank, int program) = 0;
    /// @return volume level for port'th port (if supported by the implementation, currently only jack_host<Module> implements that by measuring signal level on plugin ports)
    virtual float get_level(unsigned int port)=0;
    /// Execute menu command with given number
    virtual void execute(int cmd_no)=0;
    /// Set a configure variable on a plugin
    virtual char *configure(const char *key, const char *value) { return NULL; }
    /// Send all configure variables set within a plugin to given destination (which may be limited to only those that plugin understands)
    virtual void send_configures(send_configure_iface *)=0;
    /// Restore all state (parameters and configure vars) to default values - implemented in giface.cpp
    virtual void clear_preset();
    /// Call a named function in a plugin - this will most likely be redesigned soon - and never used
    /// @retval false call has failed, result contains an error message
    virtual bool blobcall(const char *command, const std::string &request, std::string &result) { result = "Call not supported"; return false; }
    /// Update status variables changed since last_serial
    /// @return new last_serial
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { return last_serial; }
    /// Do-nothing destructor to silence compiler warning
    virtual ~plugin_ctl_iface() {}
};

struct plugin_list_info_iface;

/// Get a list of all "large" (effect/synthesizer) plugins
extern void get_all_plugins(std::vector<plugin_metadata_iface *> &plugins);
/// Get a list of all "small" (module) plugins
extern void get_all_small_plugins(plugin_list_info_iface *plii);
/// Load and strdup a text file with GUI definition
extern const char *load_gui_xml(const std::string &plugin_id);

/// Empty implementations for plugin functions. Note, that functions aren't virtual, because they're called via the particular
/// subclass (flanger_audio_module etc) via template wrappers (ladspa_wrapper<> etc), not via base class pointer/reference
template<class Metadata>
class audio_module: public Metadata
{
public:
    typedef Metadata metadata_type;

    progress_report_iface *progress_report;

    audio_module() {
        progress_report = NULL;
    }

    /// Handle MIDI Note On
    inline void note_on(int note, int velocity) {}
    /// Handle MIDI Note Off
    inline void note_off(int note, int velocity) {}
    /// Handle MIDI Program Change
    inline void program_change(int program) {}
    /// Handle MIDI Control Change
    inline void control_change(int controller, int value) {}
    /// Handle MIDI Pitch Bend
    /// @param value pitch bend value (-8192 to 8191, defined as in MIDI ie. 8191 = 200 ct by default)
    inline void pitch_bend(int value) {}
    /// Handle MIDI Channel Pressure
    /// @param value channel pressure (0 to 127)
    inline void channel_pressure(int value) {}
    /// Called when params are changed (before processing)
    inline void params_changed() {}
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    inline void activate() {}
    /// LADSPA-esque deactivate function
    inline void deactivate() {}
    /// Set sample rate for the plugin
    inline void set_sample_rate(uint32_t sr) { }
    /// Execute menu command with given number
    inline void execute(int cmd_no) {}
    /// DSSI configure call
    virtual char *configure(const char *key, const char *value) { return NULL; }
    /// Send all understood configure vars (none by default)
    inline void send_configures(send_configure_iface *sci) {}
    /// Send all supported status vars (none by default)
    inline int send_status_updates(send_updates_iface *sui, int last_serial) { return last_serial; }
    /// Reset parameter values for epp:trigger type parameters (ones activated by oneshot push button instead of check box)
    inline void params_reset() {}
    /// Called after instantiating (after all the feature pointers are set - including interfaces like progress_report_iface)
    inline void post_instantiate() {}
    /// Handle 'message context' port message
    /// @arg output_ports pointer to bit array of output port "changed" flags, note that 0 = first audio input, not first parameter (use input_count + output_count)
    inline uint32_t message_run(const void *valid_ports, void *output_ports) { 
        fprintf(stderr, "ERROR: message run not implemented\n");
        return 0;
    }
};

extern bool check_for_message_context_ports(parameter_properties *parameters, int count);
extern bool check_for_string_ports(parameter_properties *parameters, int count);

#if USE_DSSI

enum line_graph_item
{
    LGI_END = 0,
    LGI_GRAPH,
    LGI_SUBGRAPH,
    LGI_LEGEND,
    LGI_DOT,
    LGI_END_ITEM,
    LGI_SET_RGBA,
    LGI_SET_WIDTH,
};

/// A class to send status updates via OSC
struct dssi_feedback_sender
{
    /// OSC client object used to send updates
    osctl::osc_client *client;
    /// Background thread handle
    pthread_t bg_thread;
    /// Quit flag (used to terminate the thread)
    bool quit;
    /// Indices of graphs to send
    std::vector<int> indices;
    /// Source for the graph data (interface to marshal)
    calf_plugins::line_graph_iface *graph;
    
    dssi_feedback_sender(const char *URI, line_graph_iface *_graph, calf_plugins::parameter_properties *props, int num_params);
    void update();
    ~dssi_feedback_sender();
};
#endif

/// Metadata base class template, to provide default versions of interface functions
template<class Metadata>
class plugin_metadata: public virtual plugin_metadata_iface
{    
public:
    static const char *port_names[];
    static parameter_properties param_props[];
    static ladspa_plugin_info plugin_info;

    // These below are stock implementations based on enums and static members in Metadata classes
    // they may be overridden to provide more interesting functionality

    const char *get_name() { return Metadata::impl_get_name(); } 
    const char *get_id() { return Metadata::impl_get_id(); } 
    const char *get_label() { return Metadata::impl_get_label(); } 
    int get_input_count() { return Metadata::in_count; }
    int get_output_count() { return Metadata::out_count; }
    int get_param_count() { return Metadata::param_count; }
    bool get_midi() { return Metadata::support_midi; }
    bool requires_midi() { return Metadata::require_midi; }
    bool is_rt_capable() { return Metadata::rt_capable; }
    line_graph_iface *get_line_graph_iface() { return dynamic_cast<line_graph_iface *>(this); }    
    table_edit_iface *get_table_edit_iface() { return dynamic_cast<table_edit_iface *>(this); }    
    int get_param_port_offset()  { return Metadata::in_count + Metadata::out_count; }
    const char *get_gui_xml() { static const char *data_ptr = calf_plugins::load_gui_xml(get_id()); return data_ptr; }
    plugin_command_info *get_commands() { return NULL; }
    parameter_properties *get_param_props(int param_no) { return &param_props[param_no]; }
    const char **get_port_names() { return port_names; }
    bool is_cv(int param_no) { return true; }
    bool is_noisy(int param_no) { return false; }
    const ladspa_plugin_info &get_plugin_info() { return plugin_info; }
    bool requires_message_context() { return check_for_message_context_ports(param_props, Metadata::param_count); }
    bool requires_string_ports() { return check_for_string_ports(param_props, Metadata::param_count); }
    void get_message_context_parameters(std::vector<int> &ports) {
        for (int i = 0; i < get_param_count(); ++i) {
            if (get_param_props(i)->flags & PF_PROP_MSGCONTEXT)
                ports.push_back(i);
        }
    }
};

/// A class for delegating metadata implementation to a "remote" metadata class.
/// Used for GUI wrappers that cannot have a dependency on actual classes,
/// and which instead take an "external" metadata object pointer, obtained
/// through get_all_plugins.
class plugin_metadata_proxy: public virtual plugin_metadata_iface
{
public:
    plugin_metadata_iface *impl;
public:
    plugin_metadata_proxy(plugin_metadata_iface *_impl) { impl = _impl; }
    const char *get_name() { return impl->get_name(); } 
    const char *get_id() { return impl->get_id(); } 
    const char *get_label() { return impl->get_label(); } 
    int get_input_count() { return impl->get_input_count(); }
    int get_output_count() { return impl->get_output_count(); }
    int get_param_count() { return impl->get_param_count(); }
    bool get_midi() { return impl->get_midi(); }
    bool requires_midi() { return impl->requires_midi(); }
    bool is_rt_capable() { return impl->is_rt_capable(); }
    line_graph_iface *get_line_graph_iface() { return impl->get_line_graph_iface(); }    
    table_edit_iface *get_table_edit_iface() { return impl->get_table_edit_iface(); }    
    int get_param_port_offset()  { return impl->get_param_port_offset(); }
    const char *get_gui_xml() { return impl->get_gui_xml(); }
    plugin_command_info *get_commands() { return impl->get_commands(); }
    parameter_properties *get_param_props(int param_no) { return impl->get_param_props(param_no); }
    const char **get_port_names() { return impl->get_port_names(); }
    bool is_cv(int param_no) { return impl->is_cv(param_no); }
    bool is_noisy(int param_no) { return impl->is_noisy(param_no); }
    const ladspa_plugin_info &get_plugin_info() { return impl->get_plugin_info(); }
    bool requires_message_context() { return impl->requires_message_context(); }
    bool requires_string_ports() { return impl->requires_string_ports(); }
    void get_message_context_parameters(std::vector<int> &ports) { impl->get_message_context_parameters(ports); }
};

#define CALF_PORT_NAMES(name) template<> const char *::plugin_metadata<name##_metadata>::port_names[]
#define CALF_PORT_PROPS(name) template<> parameter_properties plugin_metadata<name##_metadata>::param_props[]
#define CALF_PLUGIN_INFO(name) template<> calf_plugins::ladspa_plugin_info plugin_metadata<name##_metadata>::plugin_info
#define PLUGIN_NAME_ID_LABEL(name, id, label) \
    static const char *impl_get_name() { return name; } \
    static const char *impl_get_id() { return id; } \
    static const char *impl_get_label() { return label; } \
    

extern const char *calf_copyright_info;

};

#endif
