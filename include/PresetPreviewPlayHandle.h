/*
 * PresetPreviewPlayHandle.h - a PlayHandle specialization for playback of a short
 *                             preview of a preset or a file processed by a plugin
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _PRESET_PREVIEW_PLAY_HANDLE_H
#define _PRESET_PREVIEW_PLAY_HANDLE_H

#include "NotePlayHandle.h"


class InstrumentTrack;
class PreviewTrackContainer;

class EXPORT PresetPreviewPlayHandle : public PlayHandle
{
public:
	PresetPreviewPlayHandle( const QString& presetFile, bool loadByPlugin = false );
	virtual ~PresetPreviewPlayHandle();

	virtual void play( sampleFrame* buffer );
	virtual bool isFinished() const;

	virtual bool isFromTrack( const track * _track ) const;

	static void init();
	static void cleanup();
	static ConstNotePlayHandleList nphsOfInstrumentTrack( const InstrumentTrack* instrumentTrack );

	static bool isPreviewing();


private:
	static PreviewTrackContainer* s_previewTC;

	NotePlayHandle* m_previewNote;

} ;


#endif
