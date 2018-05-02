/*
 * AudioJack.h - support for JACK-transport
 *
 * Copyright (c) 2017 
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

#ifndef I_TRANSPORT_H
#define I_TRANSPORT_H

#include "lmms_basics.h"

/* Interface for transport */
class ITransport
{
 public:
	virtual f_cnt_t transportPosition() =0;
	virtual void transportStart() =0;
	virtual void transportStop() =0;
	virtual void transportLocate(f_cnt_t _frame) =0;
};

#endif
