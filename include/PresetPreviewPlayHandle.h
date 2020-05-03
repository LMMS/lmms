/*
 * PresetPreviewPlayHandle.h - a PlayHandle specialization for playback of a short
 *                             preview of a preset or a file processed by a plugin
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef PRESET_PREVIEW_PLAY_HANDLE_H
#define PRESET_PREVIEW_PLAY_HANDLE_H

#include "NotePlayHandle.h"


class InstrumentTrack;
class PreviewTrackContainer;

class LMMS_EXPORT PresetPreviewPlayHandle : public PlayHandle
{
public:
	PresetPreviewPlayHandle( const QString& presetFile, bool loadByPlugin = false, DataFile *dataFile = 0 );
	virtual ~PresetPreviewPlayHandle();

	inline bool affinityMatters() const override
	{
		return true;
	}

	void play( sampleFrame* buffer ) override;
	bool isFinished() const override;

	bool isFromTrack( const Track * _track ) const override;

	static void init();
	static void cleanup();
	static ConstNotePlayHandleList nphsOfInstrumentTrack( const InstrumentTrack* instrumentTrack );

	static bool isPreviewing();


private:
	static PreviewTrackContainer* s_previewTC;

	NotePlayHandle* m_previewNote;

} ;


#endif
