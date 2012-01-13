/* Calf DSP Library
 * Implementation of various helpers for the plugin interface.
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
#include <limits.h>
#include <calf/giface.h>
#include <calf/utils.h>

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

float parameter_properties::from_01(double value01) const
{
    double value = dsp::clip(value01, 0., 1.);
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        value = min + (max - min) * value01;
        break;
    case PF_SCALE_QUAD:
        value = min + (max - min) * value01 * value01;
        break;
    case PF_SCALE_LOG:
        value = min * pow(double(max / min), value01);
        break;
    case PF_SCALE_GAIN:
        if (value01 < 0.00001)
            value = min;
        else {
            float rmin = std::max(1.0f / 1024.0f, min);
            value = rmin * pow(double(max / rmin), value01);
        }
        break;
    case PF_SCALE_LOG_INF:
        assert(step);
        if (value01 > (step - 1.0) / step)
            value = FAKE_INFINITY;
        else
            value = min * pow(double(max / min), value01 * step / (step - 1.0));
        break;
    }
    switch(flags & PF_TYPEMASK)
    {
    case PF_INT:
    case PF_BOOL:
    case PF_ENUM:
    case PF_ENUM_MULTI:
        if (value > 0)
            value = (int)(value + 0.5);
        else
            value = (int)(value - 0.5);
        break;
    }
    return value;
}

double parameter_properties::to_01(float value) const
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return double(value - min) / (max - min);
    case PF_SCALE_QUAD:
        return sqrt(double(value - min) / (max - min));
    case PF_SCALE_LOG:
        value /= min;
        return log((double)value) / log((double)max / min);
    case PF_SCALE_LOG_INF:
        if (IS_FAKE_INFINITY(value))
            return max;
        value /= min;
        assert(step);
        return (step - 1.0) * log((double)value) / (step * log((double)max / min));
    case PF_SCALE_GAIN:
        if (value < 1.0 / 1024.0) // new bottom limit - 60 dB
            return 0;
        double rmin = std::max(1.0f / 1024.0f, min);
        value /= rmin;
        return log((double)value) / log(max / rmin);
    }
}

float parameter_properties::get_increment() const
{
    float increment = 0.01;
    if (step > 1)
        increment = 1.0 / (step - 1);
    else 
    if (step > 0 && step < 1)
        increment = step;
    else
    if ((flags & PF_TYPEMASK) != PF_FLOAT)
        increment = 1.0 / (max - min);
    return increment;
}

int parameter_properties::get_char_count() const
{
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC)
        return 6;
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        char buf[256];
        size_t len = 0;
        sprintf(buf, "%0.0f dB", 6.0 * log(min) / log(2));
        len = strlen(buf);
        sprintf(buf, "%0.0f dB", 6.0 * log(max) / log(2));
        len = std::max(len, strlen(buf)) + 2;
        return (int)len;
    }
    return std::max(to_string(min).length(), std::max(to_string(max).length(), to_string(min + (max-min) * 0.987654).length()));
}

std::string parameter_properties::to_string(float value) const
{
    char buf[32];
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC) {
        sprintf(buf, "%0.f%%", 100.0 * value);
        return string(buf);
    }
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        if (value < 1.0 / 1024.0) // new bottom limit - 60 dB
            return "-inf dB"; // XXXKF change to utf-8 infinity
        sprintf(buf, "%0.1f dB", 6.0 * log(value) / log(2));
        return string(buf);
    }
    switch(flags & PF_TYPEMASK)
    {
    case PF_INT:
    case PF_BOOL:
    case PF_ENUM:
    case PF_ENUM_MULTI:
        value = (int)value;
        break;
    }

    if ((flags & PF_SCALEMASK) == PF_SCALE_LOG_INF && IS_FAKE_INFINITY(value))
        sprintf(buf, "+inf"); // XXXKF change to utf-8 infinity
    else
        sprintf(buf, "%g", value);
    
    switch(flags & PF_UNITMASK) {
    case PF_UNIT_DB: return string(buf) + " dB";
    case PF_UNIT_HZ: return string(buf) + " Hz";
    case PF_UNIT_SEC: return string(buf) + " s";
    case PF_UNIT_MSEC: return string(buf) + " ms";
    case PF_UNIT_CENTS: return string(buf) + " ct";
    case PF_UNIT_SEMITONES: return string(buf) + "#";
    case PF_UNIT_BPM: return string(buf) + " bpm";
    case PF_UNIT_RPM: return string(buf) + " rpm";
    case PF_UNIT_DEG: return string(buf) + " deg";
    case PF_UNIT_NOTE: 
        {
            static const char *notes = "C C#D D#E F F#G G#A A#B ";
            int note = (int)value;
            if (note < 0 || note > 127)
                return "---";
            return string(notes + 2 * (note % 12), 2) + i2s(note / 12 - 2);
        }
    }

    return string(buf);
}

void calf_plugins::plugin_ctl_iface::clear_preset() {
    int param_count = get_metadata_iface()->get_param_count();
    for (int i = 0; i < param_count; i++)
    {
        const parameter_properties &pp = *get_metadata_iface()->get_param_props(i);
        set_param_value(i, pp.def_value);
    }
    const char *const *vars = get_metadata_iface()->get_configure_vars();
    if (vars)
    {
        for (int i = 0; vars[i]; i++)
            configure(vars[i], NULL);
    }
}

const char *calf_plugins::load_gui_xml(const std::string &plugin_id)
{
#if 0
    try {
        return strdup(calf_utils::load_file((std::string(PKGLIBDIR) + "/gui-" + plugin_id + ".xml").c_str()).c_str());
    }
    catch(file_exception e)
#endif
    {
        return NULL;
    }
}

bool calf_plugins::get_freq_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context, bool use_frequencies, float res, float ofs)
{
    if (subindex < 0 )
	return false;
    if (use_frequencies)
    {
        if (subindex < 28)
        {
            vertical = true;
            if (subindex == 9) legend = "100 Hz";
            if (subindex == 18) legend = "1 kHz";
            if (subindex == 27) legend = "10 kHz";
            float freq = 100;
            if (subindex < 9)
                freq = 10 * (subindex + 1);
            else if (subindex < 18)
                freq = 100 * (subindex - 9 + 1);
            else if (subindex < 27)
                freq = 1000 * (subindex - 18 + 1);
            else
                freq = 10000 * (subindex - 27 + 1);
            pos = log(freq / 20.0) / log(1000);
            if (!legend.empty())
                context->set_source_rgba(0, 0, 0, 0.2);
            else
                context->set_source_rgba(0, 0, 0, 0.1);
            return true;
        }
        subindex -= 28;
    }
    if (subindex >= 32)
        return false;
    float gain = 16.0 / (1 << subindex);
    pos = dB_grid(gain, res, ofs);
    if (pos < -1)
        return false;
    if (subindex != 4)
        context->set_source_rgba(0, 0, 0, subindex & 1 ? 0.1 : 0.2);
    if (!(subindex & 1))
    {
        std::stringstream ss;
        ss << (24 - 6 * subindex) << " dB";
        legend = ss.str();
    }
    vertical = false;
    return true;
}

void calf_plugins::set_channel_color(cairo_iface *context, int channel)
{
    if (channel & 1)
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
    else
        context->set_source_rgba(0.35, 0.4, 0.2, 0.5);
    context->set_line_width(1.5);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool frequency_response_line_graph::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

int frequency_response_line_graph::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////

calf_plugins::plugin_registry &calf_plugins::plugin_registry::instance()
{
    static calf_plugins::plugin_registry registry;
    return registry;
}

const plugin_metadata_iface *calf_plugins::plugin_registry::get_by_uri(const char *plugin_uri)
{
    static const char prefix[] = "http://calf.sourceforge.net/plugins/";
    if (strncmp(plugin_uri, prefix, sizeof(prefix) - 1))
        return NULL;
    const char *label = plugin_uri + sizeof(prefix) - 1;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (!strcmp(plugins[i]->get_plugin_info().label, label))
            return plugins[i];
    }    
    return NULL;
}

const plugin_metadata_iface *calf_plugins::plugin_registry::get_by_id(const char *id, bool case_sensitive)
{
    typedef int (*comparator)(const char *, const char *);
    comparator comp = case_sensitive ? strcmp : strcasecmp;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (!comp(plugins[i]->get_id(), id))
            return plugins[i];
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////

bool calf_plugins::parse_table_key(const char *key, const char *prefix, bool &is_rows, int &row, int &column)
{
    is_rows = false;
    row = -1;
    column = -1;
    if (0 != strncmp(key, prefix, strlen(prefix)))
        return false;
    
    key += strlen(prefix);
    
    if (!strcmp(key, "rows"))
    {
        is_rows = true;
        return true;
    }
    
    const char *comma = strchr(key, ',');
    if (comma)
    {
        row = atoi(string(key, comma - key).c_str());
        column = atoi(comma + 1);
        return true;
    }
    
    printf("Unknown key %s under prefix %s", key, prefix);
    
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////

const char *mod_mapping_names[] = { "0..1", "-1..1", "-1..0", "x^2", "2x^2-1", "ASqr", "ASqrBip", "Para", NULL };

mod_matrix_metadata::mod_matrix_metadata(unsigned int _rows, const char **_src_names, const char **_dest_names)
: mod_src_names(_src_names)
, mod_dest_names(_dest_names)
, matrix_rows(_rows)
{
    table_column_info tci[6] = {
        { "Source", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Mapping", TCT_ENUM, 0, 0, 0, mod_mapping_names },
        { "Modulator", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Amount", TCT_FLOAT, 0, 1, 1, NULL},
        { "Destination", TCT_ENUM, 0, 0, 0, mod_dest_names  },
        { NULL }
    };
    assert(sizeof(table_columns) == sizeof(tci));
    memcpy(table_columns, tci, sizeof(table_columns));
}

const table_column_info *mod_matrix_metadata::get_table_columns() const
{
    return table_columns;
}

uint32_t mod_matrix_metadata::get_table_rows() const
{
    return matrix_rows;
}

///////////////////////////////////////////////////////////////////////////////////////

#if USE_EXEC_GUI

table_via_configure::table_via_configure()
{
    rows = 0;
}

table_via_configure::~table_via_configure()
{
}

#endif
