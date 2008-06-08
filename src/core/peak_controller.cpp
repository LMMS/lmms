#ifndef SINGLE_SOURCE_COMPILE

/*
 * lfo_controller.cpp - implementation of class controller which handles
 *                      remote-control of automatableModels
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
#include <Qt/QtXml>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "peak_controller.h"
#include "controller_dialog.h"
#include "plugins/peak_controller_effect/peak_controller_effect.h"


peakController::peakController( model * _parent, 
		peakControllerEffect * _peak_effect ) :
	controller( PeakController, _parent ),
	m_peakEffect( _peak_effect )
{
}




peakController::~peakController()
{
	// disconnects
}



float peakController::value( int _offset )
{
	if( m_peakEffect )
	{
		return m_peakEffect->lastSample();
	}
}




void peakController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// Probably not the best idea..
//	controller::saveSettings( _doc, _this );
}



void peakController::loadSettings( const QDomElement & _this )
{
//	controller::loadSettings( _this );
}



QString peakController::nodeName( void ) const
{
	return( "peakcontroller" );
}



controllerDialog * peakController::createDialog( QWidget * _parent )
{
	controllerDialog * d = new peakControllerDialog( this, _parent );
	return d;
}


#include "peak_controller.moc"


#endif

