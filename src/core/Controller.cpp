/*
 * Controller.cpp - implementation of class controller which handles
 *                  remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "Mixer.h"
#include "Controller.h"
#include "ControllerConnection.h"
#include "ControllerDialog.h"
#include "LfoController.h"
#include "MidiController.h"
#include "PeakController.h"


unsigned int Controller::s_frames = 0;
QVector<Controller *> Controller::s_controllers;



Controller::Controller( ControllerTypes _type, Model * _parent,
					const QString & _display_name ) :
	Model( _parent, _display_name ),
	JournallingObject(),
	m_connectionCount( 0 ),
	m_type( _type )
{
	if( _type != DummyController && _type != MidiController )
	{
		s_controllers.append( this );
		// Determine which name to use
		for ( uint i=s_controllers.size(); ; i++ )
		{
			QString new_name = QString( tr( "Controller %1" ) )
					.arg( i );

			// Check if name is already in use
			bool name_used = false;
			QVector<Controller *>::const_iterator it;
			for ( it = s_controllers.constBegin();
				  it != s_controllers.constEnd(); ++it )
			{
				if ( (*it)->name() == new_name )
				{
					name_used = true;
					break;
				}
			}
			if ( ! name_used )
			{
				m_name = new_name;
				break;
			}
		}
	}
}



Controller::~Controller()
{
	int idx = s_controllers.indexOf( this );
	if( idx >= 0 )
	{
		s_controllers.remove( idx );
	}

	if( engine::getSong() )
	{
		engine::getSong()->removeController( this );
	}

	// Remove connections by destroyed signal
}



// Get current value, with an offset into the current buffer for sample exactness
float Controller::currentValue( int _offset )
{
	if( _offset == 0 || isSampleExact() )
	{
		m_currentValue = fittedValue( value( _offset ) );
	}
	
	return m_currentValue;
}



float Controller::value( int _offset )
{
	return 0.5f;
}
	


// Get position in frames
unsigned int Controller::runningFrames()
{
	return s_frames;
}



// Get position in seconds
float Controller::runningTime()
{
	return s_frames / engine::mixer()->processingSampleRate();
}



void Controller::triggerFrameCounter()
{
	for( int i = 0; i < s_controllers.size(); ++i ) 
	{
		// This signal is for updating values for both stubborn knobs and for
		// painting.  If we ever get all the widgets to use or at least check
		// currentValue() then we can throttle the signal and only use it for
		// GUI.
		emit s_controllers.at(i)->valueChanged();
	}

	s_frames += engine::mixer()->framesPerPeriod();
	//emit s_signaler.triggerValueChanged();
}



void Controller::resetFrameCounter()
{
	s_frames = 0;
}



Controller * Controller::create( ControllerTypes _ct, Model * _parent )
{
	static Controller * dummy = NULL;
	Controller * c = NULL;

	switch( _ct )
	{
		case Controller::DummyController: 
			if( dummy )
				c = dummy;
			else
			{
				c = new Controller( DummyController, NULL,
								QString() );
				dummy = c;
			}
			break;

		case Controller::LfoController:
			c = new ::LfoController( _parent );
			break;

		case Controller::PeakController:
			//Already instantiated in EffectChain::loadSettings()
			Q_ASSERT( false );
			break;

		case Controller::MidiController:
			c = new ::MidiController( _parent );
			break;

		default: 
			break;
	}

	return( c );
}



Controller * Controller::create( const QDomElement & _this, Model * _parent )
{
	Controller * c;
	if( _this.attribute( "type" ).toInt() == Controller::PeakController )
	{
		c = PeakController::getControllerBySetting( _this );
	}
	else
	{
		c = create(
			static_cast<ControllerTypes>( _this.attribute( "type" ).toInt() ),
										_parent );
	}

	if( c != NULL )
	{
		c->restoreState( _this );
	}

	return( c );
}



bool Controller::hasModel( const Model * m )
{
	QObjectList chldren = children();
	for( int i = 0; i < chldren.size(); ++i )
	{
		QObject * c = chldren.at(i);
		AutomatableModel * am = qobject_cast<AutomatableModel*>(c);
		if( am != NULL )
		{
			if( am == m )
			{
				return true;
			}

			ControllerConnection * cc = am->controllerConnection();
			if( cc != NULL )
			{
				if( cc->getController()->hasModel( m ) )
				{
					return true;
				}
			}
		}
	}
	
	return false;
}



void Controller::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "type", type() );
	_this.setAttribute( "name", name() );
}



void Controller::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "type" ).toInt() != type() )
	{
		qWarning( "controller-type does not match controller-type of "
							"settings-node!\n" );
	}

	setName( _this.attribute( "name" ) );
}


QString Controller::nodeName() const
{
	return( "Controller" );
}



ControllerDialog * Controller::createDialog( QWidget * _parent )
{
	ControllerDialog * d = new ControllerDialog( this, _parent );

	return d;
}




void Controller::addConnection( ControllerConnection * )
{
	m_connectionCount++;
}




void Controller::removeConnection( ControllerConnection * )
{
	m_connectionCount--;
	Q_ASSERT( m_connectionCount >= 0 );
}




int Controller::connectionCount() const{
	return m_connectionCount;
}




#include "moc_Controller.cxx"


