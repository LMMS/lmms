/*
 * peak_controller_effect.cpp - PeakController effect plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "Controller.h"
#include "song.h"
#include "PeakController.h"
#include "peak_controller_effect.h"

#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT peakcontrollereffect_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Peak Controller",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Plugin for controlling knobs with sound peaks" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}

// We have to keep a list of all the PeakController effects so that we can save
// an peakEffect-ID to the project.  This ID is referenced in the PeakController
// settings and is used to set the PeakControllerEffect pointer upon load

//QVector<PeakControllerEffect *> PeakControllerEffect::s_effects;

PeakControllerEffect::PeakControllerEffect(
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &peakcontrollereffect_plugin_descriptor, _parent, _key ),
	m_effectId( rand() ),
	m_peakControls( this ),
	m_lastSample( 0 ),
	m_lastRMS( -1 ),
	m_lastRMSavail(false),
	m_autoController( NULL )
{
	m_autoController = new PeakController( engine::getSong(), this );
	engine::getSong()->addController( m_autoController );
	PeakController::s_effects.append( this );
}




PeakControllerEffect::~PeakControllerEffect()
{
	int idx = PeakController::s_effects.indexOf( this );
	if( idx >= 0 )
	{
		PeakController::s_effects.remove( idx );
		engine::getSong()->removeController( m_autoController );
	}
}

namespace helpers
{

	//! returns 1.0f if val > 0.0f, -1.0 else
	inline float sign(float val) { return -1.0f + 2.0f * (val > 0.0f); }

	//! if val >= 0.0f, returns sqrtf(val), else: -sqrtf(-val)
	inline float sqrt_neg(float val) {
		return sqrtf(fabs(val)) * helpers::sign(val);
	}

}

bool PeakControllerEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	PeakControllerEffectControls & c = m_peakControls;

	// This appears to be used for determining whether or not to continue processing
	// audio with this effect	
	if( !isEnabled() || !isRunning() )
	{
		return false;
	}

	// RMS:
	double sum = 0;

	if(c.m_absModel.value())
	{
		for( int i = 0; i < _frames; ++i )
		{
			// absolute value is achieved because the squares are > 0
			sum += _buf[i][0]*_buf[i][0] + _buf[i][1]*_buf[i][1];
		}
	}
	else
	{
		for( int i = 0; i < _frames; ++i )
		{
			// the value is absolute because of squaring,
			// so we need to correct it
			sum +=	_buf[i][0]*_buf[i][0]*helpers::sign(_buf[i][0])
			 + _buf[i][1]*_buf[i][1]*helpers::sign(_buf[i][1]);
		}
	}

	// TODO: flipping this might cause clipping
	// this will mute the output after the values were measured
	if( c.m_muteModel.value() )
	{
		for( int i = 0; i < _frames; ++i )
		{
			_buf[i][0] = _buf[i][1] = 0.0f;
		}
	}

	float curRMS = helpers::sqrt_neg( sum / _frames );
	const float origRMS = curRMS;

	if( !m_lastRMSavail )
	{
		m_lastRMSavail = true;
		m_lastRMS = curRMS;
	}
	const float v = ( curRMS >= m_lastRMS ) ?
				c.m_attackModel.value() :
					c.m_decayModel.value();
	const float a = helpers::sqrt_neg( helpers::sqrt_neg( v ) );
	curRMS = (1-a)*curRMS + a*m_lastRMS;

	const float amount = c.m_amountModel.value() * c.m_amountMultModel.value();
	m_lastSample = c.m_baseModel.value() + amount*curRMS;
	m_lastRMS = curRMS;

	// on greater buffer sizes our LP is updated less frequently, therfore
	// replay a certain number of passes so the LP state is as if it was
	// updated N times with buffer-size 1/N
	const int timeOversamp = (4*_frames) / DEFAULT_BUFFER_SIZE-1;
	for( int i = 0; i < timeOversamp; ++i )
	{
		m_lastRMS = (1-a)*origRMS + a*m_lastRMS;
	}

	//checkGate( out_sum / _frames );

	return isRunning();
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return new PeakControllerEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( _data ) );
}

}

