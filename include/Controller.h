/*
 * Controller.h - declaration of class controller, which provides a
 *                standard for all controllers and controller plugins
 *
 * Copyright (c) 2008-2009 Paul Giblock <pgllama/at/gmail.com>
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


#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "engine.h"
#include "Mixer.h"
#include "Model.h"
#include "JournallingObject.h"

class ControllerDialog;
class Controller;
class ControllerConnection;

typedef QVector<Controller *> ControllerVector;


class Controller : public Model, public JournallingObject
{
	Q_OBJECT
public:
	enum ControllerTypes
	{
		DummyController,
		LfoController,
		MidiController,
		PeakController,
		/*
		XYController,
		EquationController
		*/
		NumControllerTypes
	} ;

	Controller( ControllerTypes _type, Model * _parent,
						const QString & _display_name );

	virtual ~Controller();

	virtual float currentValue( int _offset );

	inline bool isSampleExact() const
	{
		return m_sampleExact;
	}

	void setSampleExact( bool _exact )
	{
		m_sampleExact = _exact;
	}

	inline ControllerTypes type() const
	{
		return( m_type );
	}

	// return whether this controller updates models frequently - used for
	// determining when to update GUI
	inline bool frequentUpdates() const
	{
		switch( m_type )
		{
			case LfoController: return( true );
			case PeakController: return( true );
			default:
				break;
		}
		return( false );
	}

	virtual const QString & name() const
	{
		return( m_name );
	}


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName() const;

	static Controller * create( ControllerTypes _tt, Model * _parent );
	static Controller * create( const QDomElement & _this,
							Model * _parent );

	inline static float fittedValue( float _val )
	{
		return tLimit<float>( _val, 0.0f, 1.0f );
	}

	static unsigned int runningFrames();
	static float runningTime();

	static void triggerFrameCounter();
	static void resetFrameCounter();

	//Accepts a ControllerConnection * as it may be used in the future.
	void addConnection( ControllerConnection * );
	void removeConnection( ControllerConnection * );
	int connectionCount() const;


public slots:
	virtual ControllerDialog * createDialog( QWidget * _parent );

	virtual void setName( const QString & _new_name )
	{
		m_name = _new_name;
	}

	bool hasModel( const Model * m );


protected:
	// The internal per-controller get-value function
	virtual float value( int _offset );

	float m_currentValue;
	bool  m_sampleExact;
	int m_connectionCount;

	QString m_name;
	ControllerTypes m_type;

	static ControllerVector s_controllers;

	static unsigned int s_frames;


signals:
	// The value changed while the mixer isn't running (i.e: MIDI CC)
	void valueChanged();

	friend class ControllerDialog;

} ;

#endif

