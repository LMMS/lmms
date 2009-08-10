/* Calf DSP Library
 * Modulation matrix boilerplate code.
 *
 * Copyright (C) 2009 Krzysztof Foltman
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
#ifndef __CALF_MODMATRIX_H
#define __CALF_MODMATRIX_H
 
#include "giface.h"

namespace dsp {

/// Mapping modes
enum mapping_mode {
    map_positive, ///< 0..100%
    map_bipolar, ///< -100%..100%
    map_negative, ///< -100%..0%
    map_squared, ///< x^2
    map_squared_bipolar, ///< x^2 scaled to -100%..100%
    map_antisquared, ///< 1-(1-x)^2 scaled to 0..100%
    map_antisquared_bipolar, ///< 1-(1-x)^2 scaled to -100..100%
    map_parabola, ///< inverted parabola (peaks at 0.5, then decreases to 0)
    map_type_count
};

/// Single entry in modulation matrix
struct modulation_entry
{
    /// Mapped source
    int src1;
    /// Source mapping mode
    mapping_mode mapping;
    /// Unmapped modulating source
    int src2;
    /// Modulation amount
    float amount;
    /// Modulation destination
    int dest;
    
    modulation_entry() {
        reset();
    }
    
    /// Reset the row to default
    void reset() {
        src1 = 0;
        src2 = 0;
        mapping = map_positive;
        amount = 0.f;
        dest = 0;
    }
};

};

namespace calf_plugins {

class mod_matrix: public table_edit_iface
{
protected:
    /// Polynomials for different scaling modes (1, x, x^2)
    static const float scaling_coeffs[dsp::map_type_count][3];
    /// Column descriptions for table widget
    table_column_info table_columns[6];
    
    dsp::modulation_entry *matrix;
    unsigned int matrix_rows;
    const char **mod_src_names, **mod_dest_names;

    mod_matrix(dsp::modulation_entry *_matrix, unsigned int _rows, const char **_src_names, const char **_dest_names);
public:
    virtual const table_column_info *get_table_columns(int param);
    virtual uint32_t get_table_rows(int param);
    virtual std::string get_cell(int param, int row, int column);
    virtual void set_cell(int param, int row, int column, const std::string &src, std::string &error);

    /// Process modulation matrix, calculate outputs from inputs
    inline void calculate_modmatrix(float *moddest, int moddest_count, float *modsrc)
    {
        for (int i = 0; i < moddest_count; i++)
            moddest[i] = 0;
        for (unsigned int i = 0; i < matrix_rows; i++)
        {
            dsp::modulation_entry &slot = matrix[i];
            if (slot.dest) {
                float value = modsrc[slot.src1];
                const float *c = scaling_coeffs[slot.mapping];
                value = c[0] + c[1] * value + c[2] * value * value;
                moddest[slot.dest] += value * modsrc[slot.src2] * slot.amount;
            }
        }
    }
};
    
};

#endif
