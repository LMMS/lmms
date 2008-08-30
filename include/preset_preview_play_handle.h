/*
 * preset_preview_play_handle.h - play-handle for playing a short preview-sound
 *                                of a preset
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _PRESET_PREVIEW_PLAY_HANDLE_H
#define _PRESET_PREVIEW_PLAY_HANDLE_H

#include "note_play_handle.h"


class instrumentTrack;
class previewTrackContainer;


class presetPreviewPlayHandle : public playHandle
{
public:
	presetPreviewPlayHandle( const QString & _preset_file,
						bool _special_preset = false );
	virtual ~presetPreviewPlayHandle();

	virtual void play( bool _try_parallelizing,
						sampleFrame * _working_buffer );
	virtual bool done( void ) const;

	virtual bool isFromTrack( const track * _track ) const;

	static void init( void );
	static void cleanup( void );
	static constNotePlayHandleVector nphsOfInstrumentTrack(
						const instrumentTrack * _ct );


private:
	static previewTrackContainer * s_previewTC;

	notePlayHandle * m_previewNote;

} ;


#endif
