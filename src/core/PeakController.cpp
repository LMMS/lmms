/*
 * PeakController.cpp - implementation of class controller which handles
 *                      remote-control of AutomatableModels
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include "PeakController.h"


#include <QDomElement>
#include <QMessageBox>

#include "AudioEngine.h"
#include "EffectChain.h"
#include "plugins/PeakControllerEffect/PeakControllerEffect.h"

namespace lmms
{


PeakControllerEffectVector PeakController::s_effects;
int PeakController::m_getCount;
int PeakController::m_loadCount;
bool PeakController::m_buggedFile;


PeakController::PeakController( Model * _parent,
		PeakControllerEffect * _peak_effect ) :
	Controller( ControllerType::Peak, _parent, tr( "Peak Controller" ) ),
	m_peakEffect( _peak_effect ),
	m_currentSample( 0.0f )
{
	setSampleExact( true );
	if( m_peakEffect )
	{
		connect( m_peakEffect, SIGNAL(destroyed()),
			this, SLOT(handleDestroyedEffect()));
	}
	connect( Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(updateCoeffs()));
	connect( m_peakEffect->attackModel(), SIGNAL(dataChanged()),
			this, SLOT(updateCoeffs()), Qt::DirectConnection );
	connect( m_peakEffect->decayModel(), SIGNAL(dataChanged()),
			this, SLOT(updateCoeffs()), Qt::DirectConnection );
	m_coeffNeedsUpdate = true;
}




PeakController::~PeakController()
{
	if( m_peakEffect != nullptr && m_peakEffect->effectChain() != nullptr )
	{
		m_peakEffect->effectChain()->removeEffect( m_peakEffect );
	}
}


void PeakController::updateValueBuffer()
{
	if( m_coeffNeedsUpdate )
	{
		const float ratio = 44100.0f / Engine::audioEngine()->outputSampleRate();
		m_attackCoeff = 1.0f - powf( 2.0f, -0.3f * ( 1.0f - m_peakEffect->attackModel()->value() ) * ratio );
		m_decayCoeff = 1.0f -  powf( 2.0f, -0.3f * ( 1.0f - m_peakEffect->decayModel()->value()  ) * ratio );
		m_coeffNeedsUpdate = false;
	}

	if( m_peakEffect )
	{
		float targetSample = m_peakEffect->lastSample();
		if( m_currentSample != targetSample )
		{
			const f_cnt_t frames = Engine::audioEngine()->framesPerPeriod();
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


void PeakController::handleDestroyedEffect()
{
	// possible race condition...
	//printf("disconnecting effect\n");
	disconnect( m_peakEffect );
	m_peakEffect = nullptr;
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

	for (const auto& effect : s_effects)
	{
		if (effect->m_effectId == effectId)
		{
			m_peakEffect = effect;
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

	//Backward compatibility for bug in <= 0.4.15 . For >= 1.0.0 ,
	//foundCount should always be 1 because m_effectId is initialized with rand()
	int foundCount = 0;
	if( m_buggedFile == false )
	{
		for (const auto& effect : s_effects)
		{
			if (effect->m_effectId == effectId)
			{
				foundCount++;
			}
		}
		if( foundCount >= 2 )
		{
			m_buggedFile = true;
			int newEffectId = 0;
			for (const auto& effect : s_effects)
			{
				effect->m_effectId = newEffectId++;
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

	for (const auto& effect : s_effects)
	{
		if (effect->m_effectId == effectId)
		{
			return effect->controller();
		}
	}

	return nullptr;
}



QString PeakController::nodeName() const
{
	return( "Peakcontroller" );
}



gui::ControllerDialog * PeakController::createDialog( QWidget * _parent )
{
	return new gui::PeakControllerDialog( this, _parent );
}


} // namespace lmms
