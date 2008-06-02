/*
 * automation_pattern.h - declaration of class automationPattern, which contains
 *                        all information about an automation pattern
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _AUTOMATION_PATTERN_H
#define _AUTOMATION_PATTERN_H

#include <QtCore/QObject>

#include "journalling_object.h"


class automatableModel;
class midiTime;
class track;



class EXPORT automationPattern : public QObject, public journallingObject
{
	Q_OBJECT
public:
	typedef QMap<int, float> timeMap;

	automationPattern( track * _track, automatableModel * _object );
	automationPattern( const automationPattern & _pat_to_copy );
	automationPattern( const automationPattern & _pat_to_copy,
						automatableModel * _object );
	virtual ~automationPattern();


	virtual midiTime length( void ) const;

	midiTime putValue( const midiTime & _time, const float _value,
					const bool _quant_pos = TRUE );

	void removeValue( const midiTime & _time );

	inline timeMap & getTimeMap( void )
	{
		return( m_time_map );
	}

	float valueAt( const midiTime & _time );

	const QString name( void );

	// settings-management
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	static inline const QString classNodeName( void )
	{
		return( "automation-pattern" );
	}

	inline virtual QString nodeName( void ) const
	{
		return( classNodeName() );
	}

	inline const track * getTrack( void ) const
	{
		return( m_track );
	}

	inline const automatableModel * object( void ) const
	{
		return( m_object );
	}

	inline automatableModel * object( void )
	{
		return( m_object );
	}

	void processMidiTime( const midiTime & _time );

	inline bool updateFirst( void ) const
	{
		return( m_update_first );
	}

	inline void setUpdateFirst( bool _update )
	{
		m_update_first = _update;
	}

	void forgetTrack( void )
	{
		m_track = NULL;
	}


public slots:
	void openInAutomationEditor( void );
	void clear( void );


private:
	track * m_track;
	automatableModel * m_object;
	timeMap m_time_map;
	bool m_update_first;
	bool m_dynamic;

} ;




#endif
