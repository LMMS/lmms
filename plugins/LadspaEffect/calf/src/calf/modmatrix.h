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
#include <stdio.h>

namespace dsp {

/// Single entry in modulation matrix
struct modulation_entry
{
    /// Mapped source
    int src1;
    /// Source mapping mode
    calf_plugins::mod_matrix_metadata::mapping_mode mapping;
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
        mapping = calf_plugins::mod_matrix_metadata::map_positive;
        amount = 0.f;
        dest = 0;
    }
};

};

namespace calf_plugins {

class mod_matrix_impl
{
protected:
    dsp::modulation_entry *matrix;
    mod_matrix_metadata *metadata;
    unsigned int matrix_rows;
    /// Polynomials for different scaling modes (1, x, x^2)
    static const float scaling_coeffs[calf_plugins::mod_matrix_metadata::map_type_count][3];

public:
    mod_matrix_impl(dsp::modulation_entry *_matrix, calf_plugins::mod_matrix_metadata *_metadata);

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
    void send_configures(send_configure_iface *);
    char *configure(const char *key, const char *value);
    
    /// Return a list of configure variables used by the modulation matrix
    template<int rows>
    static const char **get_configure_vars()
    {
        static std::vector<std::string> names_vector;
        static const char *names[rows * 5 + 1];
        
        if (names[0] == NULL)
        {
            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < 5; j++)
                {
                    char buf[40];
                    sprintf(buf, "mod_matrix:%d,%d", i, j);
                    names_vector.push_back(buf);
                }
            }
            for (size_t i = 0; i < names_vector.size(); i++)
                names[i] = names_vector[i].c_str();
            names[names_vector.size()] = NULL;
        }
        
        return names;
    }
    
private:
    std::string get_cell(int row, int column) const;
    void set_cell(int row, int column, const std::string &src, std::string &error);
};

};

#endif
