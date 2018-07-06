/*
 * ReverbSC.cpp - A native reverb based on an algorithm by Sean Costello
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

#include <math.h>
#include "ReverbSC.h"

#include "embed.h"
#include "plugin_export.h"

#define DB2LIN(X) pow(10, X / 20.0f);

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT reverbsc_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ReverbSC",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Reverb algorithm by Sean Costello" ),
	"Paul Batchelor",
	0x0123,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}

ReverbSCEffect::ReverbSCEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &reverbsc_plugin_descriptor, parent, key ),
	m_reverbSCControls( this )
{
	sp_create(&sp);
	sp->sr = Engine::mixer()->processingSampleRate();

	sp_revsc_create(&revsc);
	sp_revsc_init(sp, revsc);

	sp_dcblock_create(&dcblk[0]);
	sp_dcblock_create(&dcblk[1]);
	
	sp_dcblock_init(sp, dcblk[0], Engine::mixer()->currentQualitySettings().sampleRateMultiplier() );
	sp_dcblock_init(sp, dcblk[1], Engine::mixer()->currentQualitySettings().sampleRateMultiplier() );
}

ReverbSCEffect::~ReverbSCEffect()
{
	sp_revsc_destroy(&revsc);
	sp_dcblock_destroy(&dcblk[0]);
	sp_dcblock_destroy(&dcblk[1]);
	sp_destroy(&sp);
}

bool ReverbSCEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

	SPFLOAT tmpL, tmpR;
	SPFLOAT dcblkL, dcblkR;
	
	ValueBuffer * inGainBuf = m_reverbSCControls.m_inputGainModel.valueBuffer();
	ValueBuffer * sizeBuf = m_reverbSCControls.m_sizeModel.valueBuffer();
	ValueBuffer * colorBuf = m_reverbSCControls.m_colorModel.valueBuffer();
	ValueBuffer * outGainBuf = m_reverbSCControls.m_outputGainModel.valueBuffer();

	for( fpp_t f = 0; f < frames; ++f )
	{
		sample_t s[2] = { buf[f][0], buf[f][1] };

		const SPFLOAT inGain = (SPFLOAT)DB2LIN((inGainBuf ? 
			inGainBuf->values()[f] 
			: m_reverbSCControls.m_inputGainModel.value()));
		const SPFLOAT outGain = (SPFLOAT)DB2LIN((outGainBuf ? 
			outGainBuf->values()[f] 
			: m_reverbSCControls.m_outputGainModel.value()));

		s[0] *= inGain;
		s[1] *= inGain;
		revsc->feedback = (SPFLOAT)(sizeBuf ? 
			sizeBuf->values()[f] 
			: m_reverbSCControls.m_sizeModel.value());

		revsc->lpfreq = (SPFLOAT)(colorBuf ? 
			colorBuf->values()[f] 
			: m_reverbSCControls.m_colorModel.value());


		sp_revsc_compute(sp, revsc, &s[0], &s[1], &tmpL, &tmpR);
		sp_dcblock_compute(sp, dcblk[0], &tmpL, &dcblkL);
		sp_dcblock_compute(sp, dcblk[1], &tmpR, &dcblkR);
		buf[f][0] = d * buf[f][0] + w * dcblkL * outGain;
		buf[f][1] = d * buf[f][1] + w * dcblkR * outGain;

		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}


	checkGate( outSum / frames );

	return isRunning();
}
	
void ReverbSCEffect::changeSampleRate()
{
	// Change sr variable in Soundpipe. does not need to be destroyed
	sp->sr = Engine::mixer()->processingSampleRate();

	mutex.lock();
	sp_revsc_destroy(&revsc);
	sp_dcblock_destroy(&dcblk[0]);
	sp_dcblock_destroy(&dcblk[1]);

	sp_revsc_create(&revsc);
	sp_revsc_init(sp, revsc);

	sp_dcblock_create(&dcblk[0]);
	sp_dcblock_create(&dcblk[1]);
	
	sp_dcblock_init(sp, dcblk[0], Engine::mixer()->currentQualitySettings().sampleRateMultiplier() );
	sp_dcblock_init(sp, dcblk[1], Engine::mixer()->currentQualitySettings().sampleRateMultiplier() );
	mutex.unlock();
}

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new ReverbSCEffect( 
		parent, 
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data) 
	);
}

}
