/*
 * lmms_constants.h - defines system constants
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_CONSTANTS_H
#define LMMS_CONSTANTS_H

const long double LD_PI = 3.14159265358979323846264338327950288419716939937510;
const long double LD_2PI = LD_PI * 2.0;
const long double LD_PI_2 = LD_PI * 0.5;
const long double LD_PI_R = 1.0 / LD_PI;
const long double LD_PI_SQR = LD_PI * LD_PI;
const long double LD_E = 2.71828182845904523536028747135266249775724709369995;
const long double LD_E_R = 1.0 / LD_E;

const double D_PI = (double) LD_PI;
const double D_2PI = (double) LD_2PI;
const double D_PI_2 = (double) LD_PI_2;
const double D_PI_R = (double) LD_PI_R;
const double D_PI_SQR = (double) LD_PI_SQR;
const double D_E = (double) LD_E;
const double D_E_R = (double) LD_E_R;

const float F_PI = (float) LD_PI;
const float F_2PI = (float) LD_2PI;
const float F_PI_2 = (float) LD_PI_2;
const float F_PI_R = (float) LD_PI_R;
const float F_PI_SQR = (float) LD_PI_SQR;
const float F_E = (float) LD_E;
const float F_E_R = (float) LD_E_R;

#endif
