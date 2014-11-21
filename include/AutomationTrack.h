/*
 * AutomationTrack.h - declaration of class AutomationTrack, which handles
 *                     automation of objects without a track
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef AUTOMATION_TRACK_H
#define AUTOMATION_TRACK_H

#include "track.h"

class AutomationProcessHandle;
typedef QVector<QPointer<AutomatableModel> > objectVector;

class AutomationTrack : public track
{
	Q_OBJECT
	MM_OPERATORS
public:
	AutomationTrack( TrackContainer* tc, bool _hidden = false );
	virtual ~AutomationTrack();

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 );
						
	virtual QString nodeName() const
	{
		return "automationtrack";
	}
	
	QString defaultName() const
	{
		return tr( "Automation track" ); 
	}

	virtual trackView * createView( TrackContainerView* );
	virtual trackContentObject * createTCO( const MidiTime & _pos );

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );
	
	virtual ProcessHandle * getProcessHandle();
	
	void addObject( AutomatableModel * _obj, bool _search_dup = true );
	void removeObject( AutomatableModel * obj );
	const AutomatableModel * firstObject() const;
	static void resolveAllIDs();
	
	objectVector * objects()
	{
		return &m_objects;
	}
	
	virtual inline float getMin() const
	{
		return firstObject()->minValue<float>();
	}

	virtual inline float getMax() const
	{
		return firstObject()->maxValue<float>();
	}

public slots:
	void objectDestroyed( jo_id_t );

private:
	friend class AutomationTrackView;
	
	AutomationProcessHandle * m_processHandle;
	objectVector m_objects;
	QVector<jo_id_t> m_idsToResolve;
} ;



class AutomationTrackView : public trackView
{
public:
	AutomationTrackView( AutomationTrack* at, TrackContainerView* tcv );
	virtual ~AutomationTrackView();

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );

} ;


// threadable processhandle for processing automation tracks
class AutomationProcessHandle : public ProcessHandle
{
	MM_OPERATORS
public:
	AutomationProcessHandle( AutomationTrack * at ) :
	ProcessHandle( ProcessHandle::AutomationProcessHandle ),
	m_track( at )
	{ }
	virtual ~AutomationProcessHandle() {}
	
	virtual void doProcessing();
	
private:
	AutomationTrack * m_track;
};


#endif
