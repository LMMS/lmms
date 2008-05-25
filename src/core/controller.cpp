#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller.cpp - implementation of class controller which handles
 *                  remote-control of automatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <Qt/QtXml>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "controller.h"
#include "controller_dialog.h"
#include "lfo_controller.h"


unsigned int controller::s_frames = 0;
QVector<controller *> controller::s_controllers;



controller::controller( ControllerTypes _type, model * _parent ) :
	model( _parent ),
	m_type( _type )
{
	s_controllers.append( this );
}



controller::~controller()
{
	s_controllers.remove( s_controllers.indexOf( this ) );

	if( engine::getSong() )
	{
		engine::getSong()->removeController( this );
	}
}



// Get current value, with an offset into the current buffer for sample exactness
float controller::currentValue( int _offset )
{
	if( _offset == 0 || isSampleExact() )
	{
		m_currentValue = fittedValue( value( _offset ) );
	}
	
	return m_currentValue;
}



float controller::value( int _offset )
{
	return 0.5f;
}
	


// Get position in frames
unsigned int controller::runningFrames()
{
	return s_frames;
}



// Get position in seconds
float controller::runningTime()
{
	return s_frames / engine::getMixer()->processingSampleRate();
}



void controller::triggerFrameCounter( void )
{
	for( int i = 0; i < s_controllers.size(); ++i ) 
	{
		// This signal is for updating values for both stubborn knobs and for
		// painting.  If we ever get all the widgets to use or at least check
		// currentValue() then we can throttle the signal and only use it for
		// GUI.
		emit s_controllers.at(i)->valueChanged();
	}

	s_frames += engine::getMixer()->framesPerPeriod();
	//emit s_signaler.triggerValueChanged();
}



void controller::resetFrameCounter( void )
{
	s_frames = 0;
}



controller * controller::create( ControllerTypes _ct, model * _parent )
{
	controller * c = NULL;

	switch( _ct )
	{
		case LfoController: c = new lfoController( _parent ); break;
		default: break;
	}

	return( c );
}



controller * controller::create( const QDomElement & _this, model * _parent )
{
	controller * c = create(
		static_cast<ControllerTypes>( _this.attribute( "type" ).toInt() ),
									_parent );
	if( c != NULL )
	{
		c->restoreState( _this );
	}

	return( c );
}



void controller::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "type", type() );
	_this.setAttribute( "name", name() );
}



void controller::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "type" ).toInt() != type() )
	{
		qWarning( "controller-type does not match controller-type of "
							"settings-node!\n" );
	}

	setName( _this.attribute( "muted" ) );
}


QString controller::nodeName( void ) const
{
	return( "controller" );
}



controllerDialog * controller::createDialog( QWidget * _parent )
{
	controllerDialog * d = new controllerDialog( this, _parent );

	return d;
}


#include "controller.moc"


#endif
