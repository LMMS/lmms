/*
 * TempoTrack.h - definition of tempo track
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

/*
#ifndef TEMPO_TRACK_H
#define TEMPO_TRACK_H

#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "song.h"

class TempoTrackView;
class TempoPatternView;
class TempoTrack : public AutomationTrack
{
	Q_OBJECT
	MM_OPERATORS
public:
	TempoTrack();
	virtual ~TempoTrack();
	
	virtual QString nodeName() const
	{
		return "tempotrack";
	}
	
	virtual inline float getMin() const
	{
		return MinTempo;
	}
	
	virtual inline float getMax() const
	{
		return MaxTempo;
	}	
	
	virtual trackView * createView( TrackContainerView* );
	virtual trackContentObject * createTCO( const MidiTime & _pos );

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );
	
	virtual ProcessHandle * getProcessHandle();
	
private:
	friend class TempoTrackView;
};

class TempoTrackView : public trackView
{
public:
	TempoTrackView( TempoTrack * tt, TrackContainerView * tcv );
	virtual ~TempoTrackView();

	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
};


class TempoPattern : public AutomationPattern
{
	MM_OPERATORS
	Q_OBJECT
public:
	TempoPattern( TempoTrack * tt );
	TempoPattern( const TempoPattern & tpCopy );
	virtual ~TempoPattern();
	
	virtual MidiTime length() const;
	
	
	// settings-management
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	
	virtual static inline const QString classNodeName()
	{
		return "tempopattern";
	}
	
	inline virtual QString nodeName() const
	{
		return classNodeName();
	}
	
	virtual trackContentObjectView * createView( trackView * _tv );
	
private:
	friend class TempoTrack;
	friend class TempoPatternView;
	
};

class TempoPatternView : public trackContentObjectView
{
	Q_OBJECT

// theming qproperties
	Q_PROPERTY( QColor fgColor READ fgColor WRITE setFgColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )

public:
	TempoPatternView( TempoPattern * pat, trackView * parent );
	virtual ~TempoPatternView();

public slots:
	virtual void update();

protected slots:
	void resetName();
	void changeName();
};

#endif
*/
