/* Calf DSP Library
 * Preset management
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
#ifndef __CALF_PRESET_H
#define __CALF_PRESET_H

#include <vector>
#include <string.h>
#include "utils.h"

namespace calf_plugins {

class plugin_ctl_iface;
    
/// Contents of single preset
struct plugin_preset
{
    /// Bank the preset belongs to (not used yet)
    int bank;
    /// Program number of the preset (not used yet)
    int program;
    /// Name of the preset
    std::string name;
    /// Name of the plugin the preset is for
    std::string plugin;
    /// Names of parameters in values array (for each item in param_names there should be a counterpart in values)
    std::vector<std::string> param_names;
    /// Values of parameters
    std::vector<float> values;
    /// DSSI configure-style variables
    std::map<std::string, std::string> variables;

    plugin_preset() : bank(0), program(0) {}
    /// Export preset as XML
    std::string to_xml();   
    /// "Upload" preset content to the plugin
    void activate(plugin_ctl_iface *plugin);
    /// "Download" preset content from the plugin
    void get_from(plugin_ctl_iface *plugin);
        
    std::string get_safe_name();
};

/// Exception thrown by preset system
struct preset_exception
{
    std::string message, param, fulltext;
    int error;
    preset_exception(const std::string &_message, const std::string &_param, int _error)
    : message(_message), param(_param), error(_error)
    {
    }
    const char *what() {
        if (error)
            fulltext = message + " " + param + " (" + strerror(error) + ")";
        else
            fulltext = message + " " + param;
        return fulltext.c_str();
    }
    ~preset_exception()
    {
    }
};

/// A vector of presets
typedef std::vector<plugin_preset> preset_vector;

/// A single list of presets (usually there are two - @see get_builtin_presets(), get_user_presets() )
struct preset_list
{
    /// Plugin list item
    struct plugin_snapshot
    {
        /// Preset offset
        int preset_offset;
        /// Plugin type
        std::string type;
        /// Instance name
        std::string instance_name;
        /// Index of the first input port
        int input_index;
        /// Index of the first output port
        int output_index;
        /// Index of the first MIDI port
        int midi_index;
        
        /// Reset to initial values
        void reset();
    };
    
    /// Parser states
    enum parser_state
    {
        START, ///< Beginning of parsing process (before root element)
        LIST, ///< Inside root element
        PRESET, ///< Inside preset definition
        VALUE, ///< Inside (empty) param tag
        VAR, ///< Inside (non-empty) var tag
        PLUGIN, ///< Inside plugin element (calfjackhost snapshots only)
        RACK, ///< Inside rack element (calfjackhost snapshots only)
    } state;

    /// Contained presets (usually for all plugins)
    preset_vector presets;
    /// Temporary preset used during parsing process
    plugin_preset parser_preset;
    /// Temporary plugin desc used during parsing process
    plugin_snapshot parser_plugin;
    /// Preset number counters for DSSI (currently broken)
    std::map<std::string, int> last_preset_ids;
    /// The key used in current <var name="key"> tag (for state == VAR)
    std::string current_key;
    /// The file is loaded in rack mode (and rack/plugin elements are expected)
    bool rack_mode;
    /// List of plugin states for rack mode
    std::vector<plugin_snapshot> plugins;

    /// Return the name of the built-in or user-defined preset file
    static std::string get_preset_filename(bool builtin);
    /// Load default preset list (built-in or user-defined)
    bool load_defaults(bool builtin);
    /// Load preset list from an in-memory XML string
    void parse(const std::string &data, bool in_rack_mode);
    /// Load preset list from XML file
    void load(const char *filename, bool in_rack_mode);
    /// Save preset list as XML file
    void save(const char *filename);
    /// Append or replace a preset (replaces a preset with the same plugin and preset name)
    void add(const plugin_preset &sp);
    /// Get a sublist of presets for a given plugin (those with plugin_preset::plugin == plugin)
    void get_for_plugin(preset_vector &vec, const char *plugin);
    
protected:
    /// Internal function: start element handler for expat
    static void xml_start_element_handler(void *user_data, const char *name, const char *attrs[]);
    /// Internal function: end element handler for expat
    static void xml_end_element_handler(void *user_data, const char *name);
    /// Internal function: character data (tag text content) handler for expat
    static void xml_character_data_handler(void *user_data, const char *data, int len);
};

/// Return the current list of built-in (factory) presets (these are loaded from system-wide file)
extern preset_list &get_builtin_presets();

/// Return the current list of user-defined presets (these are loaded from ~/.calfpresets)
extern preset_list &get_user_presets();

};

#endif
