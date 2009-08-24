/*
 * automation_recorder.h - declaration of class AutomationRecorder 
 *						which accepts a controllerEvent call from midi
 * 						controllers and creates automation TCOs if automation
 *						recording is on.
 *
 * Copyright (c) 2009-2009 Andrew Kelley <superjoe30/at/gmail.com>
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


#ifndef _AUTOMATION_RECORDER_H
#define _AUTOMATION_RECORDER_H

#include <QObject>

#include "automation_track.h"
#include "automation_pattern.h"


class AutomationRecorder : public QObject
{
	Q_OBJECT
public:
	typedef struct 
	{
		// during this recording, have we seen this controller change?
		bool seen; 
		// the track that contains the tco
		automationTrack* auto_track;
		// the tco that we're putting this automation in
		automationPattern* pat;
	} ClipData;
	typedef QMap<const AutomatableModel *, ClipData> AutoClipMap;

	AutomationRecorder();
	~AutomationRecorder();
	
	// automatable models call this when their data changes
	void modelDataEvent( AutomatableModel * _model );
	
	// must be called at some point between a recording ending and a new
	// one beginning
	void initRecord( void );

	inline bool recording( void ) const { return m_recording; }
	inline void setRecording( bool _recording ){ m_recording = _recording; }

private:
	
	bool m_recording; // while the song is playing, should we record automation?
	AutoClipMap m_clips; // remember state during recording

	friend class engine;
} ;


#endif

