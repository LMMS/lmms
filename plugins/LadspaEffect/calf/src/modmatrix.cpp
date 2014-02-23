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
using namespace calf_utils;

mod_matrix_impl::mod_matrix_impl(dsp::modulation_entry *_matrix, mod_matrix_metadata *_metadata)
: matrix(_matrix)
, metadata(_metadata)
{
    matrix_rows = metadata->get_table_rows();
    for (unsigned int i = 0; i < matrix_rows; i++)
        matrix[i].reset();
}

const float mod_matrix_impl::scaling_coeffs[mod_matrix_metadata::map_type_count][3] = {
    { 0, 1, 0 },
    { -1, 2, 0 },
    { -1, 1, 0 },
    { 0, 0, 1 },
    { -1, 0, 1 },
    { 0, 2, -1 },
    { -1, 4, -2 },
    { 0, 4, -4 },
};

std::string mod_matrix_impl::get_cell(int row, int column) const
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    const char **arr = metadata->get_table_columns()[column].values;
    switch(column) {
        case 0: // source 1
            return arr[slot.src1];
        case 1: // mapping mode
            return arr[slot.mapping];
        case 2: // source 2
            return arr[slot.src2];
        case 3: // amount
            return calf_utils::f2s(slot.amount);
        case 4: // destination
            return arr[slot.dest];
        default: 
            assert(0);
            return "";
    }
}
    
void mod_matrix_impl::set_cell(int row, int column, const std::string &src, std::string &error)
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    const char **arr = metadata->get_table_columns()[column].values;
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
                        slot.mapping = (mod_matrix_metadata::mapping_mode)i;
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

void mod_matrix_impl::send_configures(send_configure_iface *sci)
{
    for (int i = 0; i < (int)matrix_rows; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            string key = "mod_matrix:" + i2s(i) + "," + i2s(j);
            sci->send_configure(key.c_str(), get_cell(i, j).c_str());
        }
    }
}

char *mod_matrix_impl::configure(const char *key, const char *value)
{
    bool is_rows;
    int row, column;
    if (!parse_table_key(key, "mod_matrix:", is_rows, row, column))
        return NULL;
    if (is_rows)
        return strdup("Unexpected key");
    
    if (row != -1 && column != -1)
    {
        string error;
        string value_text;
        if (value == NULL)
        {
            const table_column_info &ci = metadata->get_table_columns()[column];
            if (ci.type == TCT_ENUM)
                value_text = ci.values[(int)ci.def_value];
            else
            if (ci.type == TCT_FLOAT)
                value_text = f2s(ci.def_value);
            value = value_text.c_str();
        }
        set_cell(row, column, value, error);
        if (!error.empty())
            return strdup(error.c_str());
    }
    return NULL;
}
