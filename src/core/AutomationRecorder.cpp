/*
 * AutomationRecorder.cpp - declaration of class AutomationRecorder
 *						which handles the dataChanged event of every
 * 						AutomatableModel and records it if automation
 *						recording is on.
 *
 * Copyright (c) 2009-2010 Andrew Kelley <superjoe30/at/gmail.com>
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

#include "AutomationRecorder.h"
#include "Controller.h"
#include "song.h"


AutomationRecorder::AutomationRecorder() :
	m_recording( false ),
	m_clips( AutoClipMap() )
{
}

AutomationRecorder::~AutomationRecorder()
{
}

void AutomationRecorder::modelDataEvent( AutomatableModel * _model )
{
	if( _model->armed() &&
		engine::getSong()->isRecording() &&
		engine::getSong()->isPlaying()   &&
		m_recording )
	{
		// determine the current tick
		song * s = engine::getSong();
		midiTime & song_pos = s->getPlayPos( song::Mode_PlaySong );

		/*
		// if the tick is within an existing automation TCO
		// for the automatable model that this controller controls
		track * inside_track = NULL;
		for( int i=0; i < s->trackList.size(); ++i){
			track * t = s->trackList.at(i);
			if( t->type == track::AutomationTrack &&
				song_pos >= t->startPosition() &&
				song_pos <= t->endPosition() )
			{
				inside_track = t;
				break;
			}
		}

		if( inside_track != NULL )
		{
			// edit the value of the automation TCO at this tick
			
		}
		else
		{
			// create a new automation TCO for this automatable model
			// and set the value
			
		}
		*/

		// check if we've seen this model change yet
		float val = _model->value<float>();
		if( m_clips.contains( _model ) &&
			m_clips[_model].seen )
		{
			ClipData data = m_clips[_model];
			// we've seen this controller, add automation to the TCO we added
			// first make the TCO bigger
			data.pat->changeLength( song_pos - data.pat->startPosition() );
			// now draw a line from the last one to this one
			// TODO: make it smooth (draw line instead of insert value)
			data.pat->putValue(
				song_pos - data.pat->startPosition(), val, false );

		}
		else
		{
			// new entry in controller map
			ClipData data;

			// create a new automation track in the song
			engine::getMixer()->lock();
			data.auto_track = (AutomationTrack *)
				track::create( track::AutomationTrack, engine::getSong() );
			engine::getMixer()->unlock();
			
			// put a tco in the automation track which contains the automation
			// turn the tco into an automation pattern
			data.pat = dynamic_cast<AutomationPattern *>(
				data.auto_track->createTCO( song_pos ) );
			data.pat->movePosition( song_pos );

			// connect the model to the automation pattern
			data.pat->addObject( _model );

			// add first value TODO: make sure this is absolute
			data.pat->putValue(
				song_pos - data.pat->startPosition(), val, false );
			
			// insert into map
			data.seen = true;
			m_clips.insert(_model, data);
		}
	}
}

void AutomationRecorder::initRecord()
{
	// starting a new recording, clear map
	m_clips.clear();
}

#include "moc_AutomationRecorder.cxx"

