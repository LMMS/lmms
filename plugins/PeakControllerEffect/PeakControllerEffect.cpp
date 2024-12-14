/*
 * PeakControllerEffect.cpp - PeakController effect plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "Song.h"
#include "PresetPreviewPlayHandle.h"
#include "PeakController.h"
#include "PeakControllerEffect.h"
#include "lmms_math.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT peakcontrollereffect_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Peak Controller",
	QT_TRANSLATE_NOOP( "PluginBrowser",
			"Plugin for controlling knobs with sound peaks" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}

// We have to keep a list of all the PeakController effects so that we can save
// an peakEffect-ID to the project.  This ID is referenced in the PeakController
// settings and is used to set the PeakControllerEffect pointer upon load

//QVector<PeakControllerEffect *> PeakControllerEffect::s_effects;

PeakControllerEffect::PeakControllerEffect(
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	AudioPluginInterface(&peakcontrollereffect_plugin_descriptor, _parent, _key),
	m_effectId( rand() ),
	m_peakControls( this ),
	m_lastSample( 0 ),
	m_autoController( nullptr )
{
	m_autoController = new PeakController( Engine::getSong(), this );
	if( !Engine::getSong()->isLoadingProject() && !PresetPreviewPlayHandle::isPreviewing() )
	{
		Engine::getSong()->addController( m_autoController );
	}
	PeakController::s_effects.push_back(this);
}




PeakControllerEffect::~PeakControllerEffect()
{
	auto it = std::find(PeakController::s_effects.begin(), PeakController::s_effects.end(), this);
	if (it != PeakController::s_effects.end())
	{
		PeakController::s_effects.erase(it);
		Engine::getSong()->removeController(m_autoController);
	}
}


ProcessStatus PeakControllerEffect::processImpl(CoreAudioDataMut inOut)
{
	PeakControllerEffectControls & c = m_peakControls;

	// RMS:
	double sum = 0;

	if( c.m_absModel.value() )
	{
		for (SampleFrame& frame : inOut)
		{
			// absolute value is achieved because the squares are > 0
			sum += frame.sumOfSquaredAmplitudes();
		}
	}
	else
	{
		for (SampleFrame& frame : inOut)
		{
			// the value is absolute because of squaring,
			// so we need to correct it
			sum += frame[0] * frame[0] * sign(frame[0])
				+ frame[1] * frame[1] * sign(frame[1]);
		}
	}

	// TODO: flipping this might cause clipping
	// this will mute the output after the values were measured
	if( c.m_muteModel.value() )
	{
		for (SampleFrame& frame : inOut)
		{
			frame[0] = frame[1] = 0.0f;
		}
	}

	float curRMS = sqrt_neg(sum / inOut.size());
	const float tres = c.m_tresholdModel.value();
	const float amount = c.m_amountModel.value() * c.m_amountMultModel.value();
	const float attack = 1.0f - c.m_attackModel.value();
	const float decay = 1.0f - c.m_decayModel.value();

	curRMS = qAbs( curRMS ) < tres ? 0.0f : curRMS;
	float target = c.m_baseModel.value() + amount * curRMS;
	// Use decay when the volume is decreasing, attack otherwise.
	// Since direction can change as often as every sampleBuffer, it's difficult
	// to witness attack/decay working in isolation unless using large buffer sizes.
	const float t = target < m_lastSample ? decay : attack;
	// Set m_lastSample to the interpolation between itself and target.
	// When t is 1.0, m_lastSample snaps to target. When t is 0.0, m_lastSample shouldn't change.
	m_lastSample = std::clamp(m_lastSample + t * (target - m_lastSample), 0.0f, 1.0f);

	return ProcessStatus::Continue;
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return new PeakControllerEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( _data ) );
}

}


} // namespace lmms
