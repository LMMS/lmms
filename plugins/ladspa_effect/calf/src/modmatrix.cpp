/* Calf DSP Library
 * Modulation matrix boilerplate code.
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
#include <calf/modmatrix.h>
#include <calf/utils.h>
#include <memory.h>
#include <sstream>

using namespace std;
using namespace dsp;
using namespace calf_plugins;

const char *mod_mapping_names[] = { "0..1", "-1..1", "-1..0", "x^2", "2x^2-1", "ASqr", "ASqrBip", "Para", NULL };

const float mod_matrix::scaling_coeffs[dsp::map_type_count][3] = {
    { 0, 1, 0 },
    { -1, 2, 0 },
    { -1, 1, 0 },
    { 0, 0, 1 },
    { -1, 0, 1 },
    { 0, 2, -1 },
    { -1, 4, -2 },
    { 0, 4, -4 },
};

mod_matrix::mod_matrix(modulation_entry *_matrix, unsigned int _rows, const char **_src_names, const char **_dest_names)
: matrix(_matrix)
, matrix_rows(_rows)
, mod_src_names(_src_names)
, mod_dest_names(_dest_names)
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
    for (unsigned int i = 0; i < matrix_rows; i++)
        matrix[i].reset();
}

const table_column_info *mod_matrix::get_table_columns(int param) const
{
    return table_columns;
}

uint32_t mod_matrix::get_table_rows(int param) const
{
    return matrix_rows;
}

std::string mod_matrix::get_cell(int param, int row, int column) const
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    switch(column) {
        case 0: // source 1
            return mod_src_names[slot.src1];
        case 1: // mapping mode
            return mod_mapping_names[slot.mapping];
        case 2: // source 2
            return mod_src_names[slot.src2];
        case 3: // amount
            return calf_utils::f2s(slot.amount);
        case 4: // destination
            return mod_dest_names[slot.dest];
        default: 
            assert(0);
            return "";
    }
}
    
void mod_matrix::set_cell(int param, int row, int column, const std::string &src, std::string &error) const
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    const char **arr = mod_src_names;
    if (column == 1) 
        arr = mod_mapping_names;
    if (column == 4) 
        arr = mod_dest_names;
    switch(column) {
        case 0:
        case 1:
        case 2:
        case 4:
        {
            for (int i = 0; arr[i]; i++)
            {
                if (src == arr[i])
                {
                    if (column == 0)
                        slot.src1 = i;
                    else if (column == 1)
                        slot.mapping = (mapping_mode)i;
                    else if (column == 2)
                        slot.src2 = i;
                    else if (column == 4)
                        slot.dest = i;
                    error.clear();
                    return;
                }
            }
            error = "Invalid name: " + src;
            return;
        }
        case 3:
        {
            stringstream ss(src);
            ss >> slot.amount;
            error.clear();
            return;
        }
    }
}

