/*
    wavtool-yawu
    Copyright (C) 2015 StarBrilliant <m13253@hotmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.  If not,
    see <http://www.gnu.org/licenses/>.
*/

#ifndef YAWU_OPTION_MANAGER_HPP
#define YAWU_OPTION_MANAGER_HPP

#include <cassert>
#include <vector>
#include <libwintf8/u8str.h>

namespace YAWU {

/**
 * Store global status
 */
class OptionManager {
    friend class CmdlineParser;
public:
    OptionManager();

    WTF8::u8string get_output_file_name() const { return output_file_name; }
    void set_output_file_name(WTF8::u8string new_file_name) {output_file_name = new_file_name;}
    WTF8::u8string get_input_file_name() const { return input_file_name; }
    void set_input_file_name(WTF8::u8string new_input_file_name){input_file_name = new_input_file_name;}
    double get_stp() const { return stp; }
    void set_stp(double new_stp) {stp = new_stp; }
    double get_note_length() const { return note_length; }
    void set_note_length(double new_note_length) {note_length = new_note_length;}
    double get_overlap() const { return overlap; }
    void set_overlap(double new_overlap){overlap = new_overlap;}
    double get_env_p(size_t idx) const {
        assert(idx > 0 && idx < 6);
        return p[idx];
    }
    void set_env_p(double new_env_p[6])
    {
        delete[] p;
        p = new_env_p;
    }
    double get_env_v(size_t idx) const {
        assert(idx > 0 && idx < 6);
        return v[idx];
    }
    void set_env_v(double new_env_v[6])
    {
        delete v;
        v = new_env_v;
    }
    bool is_p5_enabled() const { return p5_enabled; }
    void set_p5_enabled(bool new_state){p5_enabled = new_state;}
protected:
    WTF8::u8string output_file_name;
    WTF8::u8string input_file_name;

    // Parameters are converted into units of seconds
    double stp = 0;
    double note_length = 0;
    double overlap = 0;

    double *p = new double[6] { 0, 0, 0, 0, 0, 0 };
    double *v = new double[6] { 1, 1, 1, 1, 1, 1 };
    bool p5_enabled = false;
};

}

#endif
