/*
 * Controller.cpp - implementation of class controller which handles
 *                  remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QDomElement>
#include <QVector>


#include "AudioEngine.h"
#include "ControllerConnection.h"
#include "ControllerDialog.h"
#include "LfoController.h"
#include "MidiController.h"
#include "PeakController.h"

namespace lmms
{


long Controller::s_periods = 0;
QVector<Controller *> Controller::s_controllers;



Controller::Controller( ControllerTypes _type, Model * _parent,
					const QString & _display_name ) :
	Model( _parent, _display_name ),
	JournallingObject(),
	m_valueBuffer( Engine::audioEngine()->framesPerPeriod() ),
	m_bufferLastUpdated( -1 ),
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
			for (Controller * controller : s_controllers)
			{
				if ( controller->name() == new_name )
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
	updateValueBuffer();
}



Controller::~Controller()
{
	int idx = s_controllers.indexOf( this );
	if( idx >= 0 )
	{
		s_controllers.remove( idx );
	}

	m_valueBuffer.clear();
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



float Controller::value( int offset )
{
	if( m_bufferLastUpdated != s_periods )
	{
		updateValueBuffer();
	}
	return m_valueBuffer.values()[ offset ];
}
	

ValueBuffer * Controller::valueBuffer()
{
	if( m_bufferLastUpdated != s_periods )
	{
		updateValueBuffer();
	}
	return &m_valueBuffer;
}


void Controller::updateValueBuffer()
{
	m_valueBuffer.fill(0.5f);
	m_bufferLastUpdated = s_periods;
}


// Get position in frames
unsigned int Controller::runningFrames()
{
	return s_periods * Engine::audioEngine()->framesPerPeriod();
}



// Get position in seconds
float Controller::runningTime()
{
	return runningFrames() / Engine::audioEngine()->processingSampleRate();
}



void Controller::triggerFrameCounter()
{
	for (Controller * controller : s_controllers)
	{
		// This signal is for updating values for both stubborn knobs and for
		// painting.  If we ever get all the widgets to use or at least check
		// currentValue() then we can throttle the signal and only use it for
		// GUI.
		emit controller->valueChanged();
	}

	s_periods ++;
	//emit s_signaler.triggerValueChanged();
}



void Controller::resetFrameCounter()
{
	for (Controller * controller : s_controllers)
	{
		controller->m_bufferLastUpdated = 0;
	}
	s_periods = 0;
}



Controller * Controller::create( ControllerTypes _ct, Model * _parent )
{
	static Controller * dummy = nullptr;
	Controller * c = nullptr;

	switch( _ct )
	{
		case Controller::DummyController:
			if (!dummy)
				dummy = new Controller( DummyController, nullptr,
								QString() );
			c = dummy;
			break;

		case Controller::LfoController:
			c = new class LfoController( _parent );
			break;

		case Controller::PeakController:
			//Already instantiated in EffectChain::loadSettings()
			Q_ASSERT( false );
			break;

		case Controller::MidiController:
			c = new class MidiController( _parent );
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

	if( c != nullptr )
	{
		c->restoreState( _this );
	}

	return( c );
}



bool Controller::hasModel( const Model * m ) const
{
	for (QObject * c : children())
	{
		auto am = qobject_cast<AutomatableModel*>(c);
		if( am != nullptr )
		{
			if( am == m )
			{
				return true;
			}

			ControllerConnection * cc = am->controllerConnection();
			if( cc != nullptr && cc->getController()->hasModel( m ) )
			{
				return true;
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



gui::ControllerDialog * Controller::createDialog( QWidget * _parent )
{
	auto d = new gui::ControllerDialog(this, _parent);

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


} // namespace lmms




