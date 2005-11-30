/*
 * export.h - header which is needed for song-export
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _EXPORT_H
#define _EXPORT_H

#include "types.h"

class audioDevice;


typedef audioDevice * ( * getDeviceInst)( Uint32 _sample_rate,
						Uint32 _channels,
						bool & _success_ful,
						const QString & _file,
						bool _use_vbr,
						Uint16 _nom_bitrate,
						Uint16 _min_bitrate,
						Uint16 _max_bitrate );


enum fileTypes
{
	WAVE_FILE,
	OGG_FILE,
	NULL_FILE = 0xFF
} ;


struct fileEncodeDevice
{
	fileTypes m_fileType;
	const char * m_description;
	const char * m_extension;
	getDeviceInst m_getDevInst;
} ;

extern fileEncodeDevice fileEncodeDevices[];


#endif
