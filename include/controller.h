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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "engine.h"
#include "mixer.h"
#include "mv_base.h"
#include "templates.h"

class controllerDialog;
class controller; 

typedef QVector<controller *> controllerVector;

class controller : public model
{
	Q_OBJECT
public:
	controller( model * _parent );

	virtual ~controller();

	virtual float currentValue( int _offset );

	inline bool isSampleExact( void ) const
	{
		return m_sampleExact || engine::getMixer()->highQuality();
	}

	void setSampleExact( bool _exact )
	{
		m_sampleExact = _exact;
	}

	virtual QString publicName() const
	{
		return "Dummy Controller";
	}

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

protected:

	// The internal per-controller get-value function
	virtual float value( int _offset );

	float m_currentValue;
	bool  m_sampleExact;


	static controllerVector s_controllers;

	static unsigned int s_frames;

	/*
slots:
	void trigger();

	*/

signals:
	// The value changed while the mixer isn't running (i.e: MIDI CC)
	void valueChanged( void );

	// Allow all attached models to unlink
	void destroying( void );

	friend class controllerDialog;
};

#endif

