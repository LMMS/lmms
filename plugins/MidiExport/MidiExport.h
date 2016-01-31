/*
 * MidiExport.h - support for Exporting MIDI-files
 *
 * Author: Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _MIDI_EXPORT_H
#define _MIDI_EXPORT_H

#include <QString>

#include "ExportFilter.h"
#include "MidiFile.hpp"



class MidiExport: public ExportFilter
{
// 	Q_OBJECT
public:
	MidiExport( Engine * engine );
	~MidiExport();

	virtual PluginView * instantiateView( QWidget * )
	{
		return( NULL );
	}

	virtual bool tryExport( const TrackContainer::TrackList &tracks, int tempo, const QString &filename );
	
private:
	

	void error( void );


} ;


#endif
