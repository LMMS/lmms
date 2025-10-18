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

#include "ReverbSC.h"

#include "embed.h"
#include "lmms_math.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT reverbsc_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"ReverbSC",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Reverb algorithm by Sean Costello" ),
	"Paul Batchelor",
	0x0123,
	Plugin::Type::Effect,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
	nullptr,
} ;

}

ReverbSCEffect::ReverbSCEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &reverbsc_plugin_descriptor, parent, key ),
	m_reverbSCControls( this )
{
	sp_create(&sp);
	sp->sr = Engine::audioEngine()->outputSampleRate();

	sp_revsc_create(&revsc);
	sp_revsc_init(sp, revsc);

	sp_dcblock_create(&dcblk[0]);
	sp_dcblock_create(&dcblk[1]);

	sp_dcblock_init(sp, dcblk[0], 1);
	sp_dcblock_init(sp, dcblk[1], 1);
}

ReverbSCEffect::~ReverbSCEffect()
{
	sp_revsc_destroy(&revsc);
	sp_dcblock_destroy(&dcblk[0]);
	sp_dcblock_destroy(&dcblk[1]);
	sp_destroy(&sp);
}

Effect::ProcessStatus ReverbSCEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
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
		auto s = std::array{buf[f][0], buf[f][1]};

		const auto inGain = static_cast<SPFLOAT>(fastPow10f(
			(inGainBuf ? inGainBuf->values()[f] : m_reverbSCControls.m_inputGainModel.value()) / 20.f));
		const auto outGain = static_cast<SPFLOAT>(fastPow10f(
			(outGainBuf ? outGainBuf->values()[f] : m_reverbSCControls.m_outputGainModel.value()) / 20.f));

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
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

void ReverbSCEffect::changeSampleRate()
{
	// Change sr variable in Soundpipe. does not need to be destroyed
	sp->sr = Engine::audioEngine()->outputSampleRate();

	mutex.lock();
	sp_revsc_destroy(&revsc);
	sp_dcblock_destroy(&dcblk[0]);
	sp_dcblock_destroy(&dcblk[1]);

	sp_revsc_create(&revsc);
	sp_revsc_init(sp, revsc);

	sp_dcblock_create(&dcblk[0]);
	sp_dcblock_create(&dcblk[1]);

	sp_dcblock_init(sp, dcblk[0], 1);
	sp_dcblock_init(sp, dcblk[1], 1);
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


} // namespace lmms
