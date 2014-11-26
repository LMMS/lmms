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

#include <cstdio>
#include <QDomElement>
#include <QObject>
#include <QVector>
#include <QMessageBox>


#include "Song.h"
#include "Engine.h"
#include "Mixer.h"
#include "PeakController.h"
#include "EffectChain.h"
#include "ControllerDialog.h"
#include "plugins/peak_controller_effect/peak_controller_effect.h"
#include "PresetPreviewPlayHandle.h"
#include "lmms_math.h"
#include "interpolation.h"


PeakControllerEffectVector PeakController::s_effects;


PeakController::PeakController( Model * _parent, 
		PeakControllerEffect * _peak_effect ) :
	Controller( Controller::PeakController, _parent, tr( "Peak Controller" ) ),
	m_peakEffect( _peak_effect ),
	m_currentSample( 0.0f )
{
	setSampleExact( true );
	if( m_peakEffect )
	{
		connect( m_peakEffect, SIGNAL( destroyed( ) ),
			this, SLOT( handleDestroyedEffect( ) ) );
	}
	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateCoeffs() ) );
	connect( m_peakEffect->attackModel(), SIGNAL( dataChanged() ), this, SLOT( updateCoeffs() ) );
	connect( m_peakEffect->decayModel(), SIGNAL( dataChanged() ), this, SLOT( updateCoeffs() ) );
	m_coeffNeedsUpdate = true;
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


void PeakController::updateValueBuffer()
{
	if( m_coeffNeedsUpdate )
	{
		const float ratio = 44100.0f / Engine::mixer()->processingSampleRate();
		m_attackCoeff = 1.0f - powf( 2.0f, -0.3f * ( 1.0f - m_peakEffect->attackModel()->value() ) * ratio );
		m_decayCoeff = 1.0f -  powf( 2.0f, -0.3f * ( 1.0f - m_peakEffect->decayModel()->value()  ) * ratio );
		m_coeffNeedsUpdate = false;
	}

	if( m_peakEffect )
	{
		float targetSample = m_peakEffect->lastSample();
		if( m_currentSample != targetSample )
		{
			const f_cnt_t frames = Engine::mixer()->framesPerPeriod();
			float * values = m_valueBuffer.values();
			
			for( f_cnt_t f = 0; f < frames; ++f )
			{
				const float diff = ( targetSample - m_currentSample );
				if( m_currentSample < targetSample ) // going up...
				{
					m_currentSample += diff * m_attackCoeff;
				}
				else if( m_currentSample > targetSample ) // going down
				{
					m_currentSample += diff * m_decayCoeff;
				}
				values[f] = m_currentSample;
			}
		}
		else
		{
			m_valueBuffer.fill( m_currentSample );
		}
	}
	else
	{
		m_valueBuffer.fill( 0 );
	}
	m_bufferLastUpdated = s_periods;
}


void PeakController::updateCoeffs()
{
	m_coeffNeedsUpdate = true;
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



QString PeakController::nodeName() const
{
	return( "Peakcontroller" );
}



ControllerDialog * PeakController::createDialog( QWidget * _parent )
{
	return new PeakControllerDialog( this, _parent );
}




