/*
 * PeakController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <math.h>
#include <cstdio>
#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QMessageBox>


#include "song.h"
#include "engine.h"
#include "Mixer.h"
#include "PeakController.h"
#include "EffectChain.h"
#include "ControllerDialog.h"
#include "plugins/peak_controller_effect/peak_controller_effect.h"
#include "PresetPreviewPlayHandle.h"

PeakControllerEffectVector PeakController::s_effects;
int PeakController::m_getCount;
int PeakController::m_loadCount;
bool PeakController::m_buggedFile;


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
	//EffectChain::loadSettings() appends effect to EffectChain::m_effects
	//When it's previewing, EffectChain::loadSettings(<Controller Fx XML>) is not called
	//Therefore, we shouldn't call removeEffect() as it is not even appended.
	//NB: Most XML setting are loaded on preview, except controller fx.
	if( m_peakEffect != NULL && m_peakEffect->effectChain() != NULL && PresetPreviewPlayHandle::isPreviewing() == false )
	{
		m_peakEffect->effectChain()->removeEffect( m_peakEffect );
	}
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
	//printf("disconnecting effect\n");
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

		_this.setAttribute( "effectId", m_peakEffect->m_effectId );
	}
}



void PeakController::loadSettings( const QDomElement & _this )
{
	Controller::loadSettings( _this );

	int effectId = _this.attribute( "effectId" ).toInt();
	if( m_buggedFile == true )
	{
		effectId = m_loadCount++;
	}

	PeakControllerEffectVector::Iterator i;
	for( i = s_effects.begin(); i != s_effects.end(); ++i )
	{
		if( (*i)->m_effectId == effectId )
		{
			m_peakEffect = *i;
			return;
		}
	}
}




//Backward compatibility function for bug in <= 0.4.15
void PeakController::initGetControllerBySetting()
{
	m_loadCount = 0;
	m_getCount = 0;
	m_buggedFile = false;
}




PeakController * PeakController::getControllerBySetting(const QDomElement & _this )
{
	int effectId = _this.attribute( "effectId" ).toInt();

	PeakControllerEffectVector::Iterator i;

	//Backward compatibility for bug in <= 0.4.15 . For >= 1.0.0 ,
	//foundCount should always be 1 because m_effectId is initialized with rand()
	int foundCount = 0;
	if( m_buggedFile == false )
	{
		for( i = s_effects.begin(); i != s_effects.end(); ++i )
		{
			if( (*i)->m_effectId == effectId )
			{
				foundCount++;
			}
		}
		if( foundCount >= 2 )
		{
			m_buggedFile = true;
			int newEffectId = 0;
			for( i = s_effects.begin(); i != s_effects.end(); ++i )
			{
				(*i)->m_effectId = newEffectId++;
			}
			QMessageBox msgBox;
			msgBox.setIcon( QMessageBox::Information );
			msgBox.setWindowTitle( tr("Peak Controller Bug") );
			msgBox.setText( tr("Due to a bug in older version of LMMS, the peak "
							   "controllers may not be connect properly. "
							   "Please ensure that peak controllers are connected "
							   "properly and re-save this file. "
							   "Sorry for any inconvenience caused.") );
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
		}
	}

	if( m_buggedFile == true )
	{
		effectId = m_getCount;
	}
	m_getCount++; //NB: m_getCount should be increased even m_buggedFile is false

	for( i = s_effects.begin(); i != s_effects.end(); ++i )
	{
		if( (*i)->m_effectId == effectId )
		{
			return (*i)->controller();
		}
	}

	return NULL;
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

