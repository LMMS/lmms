/*
 * automatable_object.h - declaration of class automatableObject
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUTOMATABLE_OBJECT_H
#define _AUTOMATABLE_OBJECT_H

#include <math.h>

#include "journalling_object.h"
#include "level_object.h"

#include <QtCore/QPointer>



class automationPattern;
class track;


template<typename T, typename EDIT_STEP_TYPE = T>
class automatableObject : public journallingObject, public levelObject
{
public:
	typedef automatableObject<T, EDIT_STEP_TYPE> autoObj;

	automatableObject( track * _track = NULL, const T _val = 0,
					const T _min = 0, const T _max = 0,
					const T _step = defaultRelStep() );

	virtual ~automatableObject();

	static inline T minRelStep( void )
	{
		return( 1 );
	}

	static inline T defaultRelStep( void )
	{
		return( 1 );
	}

	static inline T minEps( void )
	{
		return( 1 );
	}


	inline virtual T value( void ) const
	{
		return( m_value );
	}

	inline virtual T minValue( void ) const
	{
		return( m_minValue );
	}

	inline virtual T maxValue( void ) const
	{
		return( m_maxValue );
	}

	inline virtual T step( void ) const
	{
		return( m_step );
	}

	inline int curLevel( void ) const
	{
		return( m_curLevel );
	}

	inline T fittedValue( T _value ) const;

	T value( int _level ) const
	{
		return( fittedValue( _level * m_step ) );
	}

	virtual void setInitValue( const T _value );

	inline virtual void setValue( const T _value );

	inline virtual void incValue( int _steps )
	{
		setValue( m_value + _steps * m_step );
	}

	virtual void setRange( const T _min, const T _max,
					const T _step = defaultRelStep() );

	inline virtual void setStep( const T _step );

	static void linkObjects( autoObj * _object1, autoObj * _object2 );

	static void unlinkObjects( autoObj * _object1, autoObj * _object2 );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
					QDomElement & _this,
					const QString & _name = "value" );

	virtual void FASTCALL loadSettings( const QDomElement & _this,
					const QString & _name = "value" );

	virtual QString nodeName( void ) const
	{
		return( "automatableobject" );
	}

	inline automationPattern * getAutomationPattern( void );

	inline bool nullTrack( void )
	{
		return( m_track == NULL );
	}

	void initAutomationPattern( void )
	{
		m_automation_pattern = new automationPattern( NULL, this );
	}


protected:
	virtual void redoStep( journalEntry & _je );

	virtual void undoStep( journalEntry & _je );

	void prepareJournalEntryFromOldVal( void );

	void addJournalEntryFromOldToCurVal( void );

	inline void setFirstValue( void );


private:
	T m_value;
	T m_minValue;
	T m_maxValue;
	T m_step;
	int m_curLevel;
	QPointer<automationPattern> m_automation_pattern;
	track * m_track;

	// most objects will need this temporarily
	T m_oldValue;
	bool m_journalEntryReady;

	typedef QVector<autoObj *> autoObjVector;
	autoObjVector m_linkedObjects;

	inline void linkObject( autoObj * _object );

	inline void unlinkObject( autoObj * _object );

	static inline T attributeValue( QString _value );

	inline void syncAutomationPattern( void );

	inline void setLevel( int _level );

	inline int level( T _value ) const
	{
		return( (int)roundf( _value / (float)m_step ) );
	}

	QString levelToLabel( int _level ) const
	{
		return( QString::number( value( _level ) ) );
	}

	int labelToLevel( QString _label )
	{
		return( level( attributeValue( _label ) ) );
	}

} ;




#endif

