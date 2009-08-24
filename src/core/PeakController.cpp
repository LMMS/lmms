/*
 * PeakController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
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

#include <math.h>
#include <cstdio>
#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "PeakController.h"
#include "ControllerDialog.h"
#include "plugins/peak_controller_effect/peak_controller_effect.h"

int PeakController::s_lastEffectId = 0;
PeakControllerEffectVector PeakController::s_effects;


PeakController::PeakController( Model * _parent, 
		PeakControllerEffect * _peak_effect ) :
	Controller( Controller::PeakController, _parent, tr( "Peak Controller" ) ),
	m_peakEffect( _peak_effect )
{
	if( m_peakEffect )
	{
		connect( m_peakEffect, SIGNAL( destroyed( ) ),
			this, SLOT( handleDestroyedEffect( ) ) );
	}
}




PeakController::~PeakController()
{
	// disconnects
}



float PeakController::value( int _offset )
{
	if( m_peakEffect )
	{
		return m_peakEffect->lastSample();
	}
	return( 0 );
}



void PeakController::handleDestroyedEffect( )
{
	// possible race condition...
	printf("disconnecting effect\n");
	disconnect( m_peakEffect );
	m_peakEffect = NULL;
	//deleteLater();
	delete this;
}



void PeakController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( m_peakEffect )
	{
		Controller::saveSettings( _doc, _this );

		_this.setAttribute( "effectId", m_peakEffect->effectId() );
	}
}



void PeakController::loadSettings( const QDomElement & _this )
{
	int effectId = _this.attribute( "effectId" ).toInt();

	PeakControllerEffectVector::Iterator i;
	for( i = s_effects.begin(); i != s_effects.end(); ++i )
	{
		if( (*i)->effectId() == effectId )
		{
			m_peakEffect = *i;
			return;
		}
	}
}



QString PeakController::nodeName() const
{
	return( "Peakcontroller" );
}



ControllerDialog * PeakController::createDialog( QWidget * _parent )
{
	return new PeakControllerDialog( this, _parent );
}


#include "moc_PeakController.cxx"

