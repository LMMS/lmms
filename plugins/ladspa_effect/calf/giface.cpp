/* Calf DSP Library
 * Module wrapper methods.
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
 * Boston, MA  02110-1301  USA
 */
#include <assert.h>
#include <memory.h>
#include <calf/giface.h>
#include <stdio.h>
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
    case PF_STRING:
        return "N/A";
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
    int param_count = get_param_count();
    for (int i=0; i < param_count; i++)
    {
        parameter_properties &pp = *get_param_props(i);
        if ((pp.flags & PF_TYPEMASK) == PF_STRING)
        {
            configure(pp.short_name, pp.choices ? pp.choices[0] : "");
        }
        else
            set_param_value(i, pp.def_value);
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

bool calf_plugins::check_for_message_context_ports(parameter_properties *parameters, int count)
{
    for (int i = count - 1; i >= 0; i--)
    {
        if (parameters[i].flags & PF_PROP_MSGCONTEXT)
            return true;
    }
    return false;
}

bool calf_plugins::check_for_string_ports(parameter_properties *parameters, int count)
{
    for (int i = count - 1; i >= 0; i--)
    {
        if ((parameters[i].flags & PF_TYPEMASK) == PF_STRING)
            return true;
        if ((parameters[i].flags & PF_TYPEMASK) < PF_STRING)
            return false;
    }
    return false;
}

#if USE_DSSI
struct osc_cairo_control: public cairo_iface
{
    osctl::osc_inline_typed_strstream &os;
    
    osc_cairo_control(osctl::osc_inline_typed_strstream &_os) : os(_os) {}
    virtual void set_source_rgba(float r, float g, float b, float a = 1.f)
    {
        os << (uint32_t)LGI_SET_RGBA << r << g << b << a;
    }
    virtual void set_line_width(float width)
    {
        os << (uint32_t)LGI_SET_WIDTH << width;
    }
};

static void send_graph_via_osc(osctl::osc_client &client, const std::string &address, line_graph_iface *graph, std::vector<int> &params)
{
    osctl::osc_inline_typed_strstream os;
    osc_cairo_control cairoctl(os);
    for (size_t i = 0; i < params.size(); i++)
    {
        int index = params[i];
        os << (uint32_t)LGI_GRAPH;
        os << (uint32_t)index;
        for (int j = 0; ; j++)
        {
            float data[128];
            if (graph->get_graph(index, j, data, 128, &cairoctl))
            {
                os << (uint32_t)LGI_SUBGRAPH;
                os << (uint32_t)128;
                for (int p = 0; p < 128; p++)
                    os << data[p];
            }
            else
                break;
        }
        for (int j = 0; ; j++)
        {
            float x, y;
            int size = 3;
            if (graph->get_dot(index, j, x, y, size, &cairoctl))
                os << (uint32_t)LGI_DOT << x << y << (uint32_t)size;
            else
                break;
        }
        for (int j = 0; ; j++)
        {
            float pos = 0;
            bool vertical = false;
            string legend;
            if (graph->get_gridline(index, j, pos, vertical, legend, &cairoctl))
                os << (uint32_t)LGI_LEGEND << pos << (uint32_t)(vertical ? 1 : 0) << legend;
            else
                break;
        }
        os << (uint32_t)LGI_END_ITEM;
    }
    os << (uint32_t)LGI_END;
    client.send(address, os);
}

calf_plugins::dssi_feedback_sender::dssi_feedback_sender(const char *URI, line_graph_iface *_graph, calf_plugins::parameter_properties *props, int num_params)
{
    graph = _graph;
    client = new osctl::osc_client;
    client->bind("0.0.0.0", 0);
    client->set_url(URI);
    for (int i = 0; i < num_params; i++)
    {
        if (props[i].flags & PF_PROP_GRAPH)
            indices.push_back(i);
    }
}

void calf_plugins::dssi_feedback_sender::update()
{
    send_graph_via_osc(*client, "/lineGraph", graph, indices);
}

calf_plugins::dssi_feedback_sender::~dssi_feedback_sender()
{
    // this would not be received by GUI's main loop because it's already been terminated
    // client->send("/iQuit");
    delete client;
}
#endif
