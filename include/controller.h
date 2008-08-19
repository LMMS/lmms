/*
 * controller.h - declaration of class controller, which provides a
 *                standard for all controllers and controller plugins
 *
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
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


#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "engine.h"
#include "mixer.h"
#include "mv_base.h"
#include "journalling_object.h"

class controllerDialog;
class controller; 

typedef QVector<controller *> controllerVector;


class controller : public model, public journallingObject
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

	controller( ControllerTypes _type, model * _parent,
						const QString & _display_name );

	virtual ~controller();

	virtual float currentValue( int _offset );

	inline bool isSampleExact( void ) const
	{
		return m_sampleExact ||
			engine::getMixer()->currentQualitySettings().
							sampleExactControllers;
	}

	void setSampleExact( bool _exact )
	{
		m_sampleExact = _exact;
	}

	ControllerTypes type( void ) const
	{
		return( m_type );
	}


	virtual const QString & name( void ) const
	{
		return( m_name );
	}


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const;

	static controller * create( ControllerTypes _tt, model * _parent );
	static controller * create( const QDomElement & _this,
							model * _parent );

	inline static float fittedValue( float _val )
	{
		return tLimit<float>( _val, 0.0f, 1.0f );
	}

	static unsigned int runningFrames();
	static float runningTime();

	static void triggerFrameCounter( void );
	static void resetFrameCounter( void );


public slots:
	virtual controllerDialog * createDialog( QWidget * _parent );

	virtual void setName( const QString & _new_name )
	{
		m_name = _new_name;
	}

	bool hasModel( const model * m );


protected:
	// The internal per-controller get-value function
	virtual float value( int _offset );

	float m_currentValue;
	bool  m_sampleExact;

	QString m_name;
	ControllerTypes m_type;

	static controllerVector s_controllers;

	static unsigned int s_frames;


signals:
	// The value changed while the mixer isn't running (i.e: MIDI CC)
	void valueChanged( void );

	// Allow all attached models to unlink
	void destroying( void );

	friend class controllerDialog;

} ;

#endif

