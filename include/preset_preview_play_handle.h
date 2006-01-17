/*
 * preset_preview_play_handle.h - play-handle for playing a short preview-sound
 *                                of a preset
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PRESET_PREVIEW_PLAY_HANDLE_H
#define _PRESET_PREVIEW_PLAY_HANDLE_H

#include "note_play_handle.h"

#include <qmutex.h>

class channelTrack;
class QMutex;


class presetPreviewPlayHandle : public playHandle
{
public:
	presetPreviewPlayHandle( const QString & _preset_file );
	virtual ~presetPreviewPlayHandle();

	virtual void play( void );
	virtual bool done( void ) const;

	static void cleanUp( void );
	static constNotePlayHandleVector nphsOfChannelTrack(
						const channelTrack * _ct );

private:
	static channelTrack * s_globalChannelTrack;
	static notePlayHandle * s_globalPreviewNote;
	static QMutex s_globalDataMutex;

	notePlayHandle * m_previewNote;

} ;


#endif
