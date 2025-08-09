/*
 * InstrumentSoundShaping.cpp - implementation of class InstrumentSoundShaping
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QVarLengthArray>
#include <QDomElement>

#include "InstrumentSoundShaping.h"
#include "AudioEngine.h"
#include "BasicFilters.h"
#include "embed.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"

namespace lmms
{


const float CUT_FREQ_MULTIPLIER = 6000.0f;
const float RES_MULTIPLIER = 2.0f;
const float RES_PRECISION = 1000.0f;


InstrumentSoundShaping::InstrumentSoundShaping(
					InstrumentTrack * _instrument_track ) :
	Model( _instrument_track, tr( "Envelopes/LFOs" ) ),
	m_instrumentTrack( _instrument_track ),
	m_volumeParameters(1., this),
	m_cutoffParameters(0., this),
	m_resonanceParameters(0., this),
	m_filterEnabledModel( false, this ),
	m_filterModel( this, tr( "Filter type" ) ),
	m_filterCutModel( 14000.0, 1.0, 14000.0, 1.0, this, tr( "Cutoff frequency" ) ),
	m_filterResModel(0.5f, BasicFilters<>::minQ(), 10.f, 0.01f, this, tr("Q/Resonance"))
{
	m_volumeParameters.setDisplayName(tr("Volume"));
	m_cutoffParameters.setDisplayName(tr("Cutoff frequency"));
	m_resonanceParameters.setDisplayName(tr("Resonance"));

	m_filterModel.addItem( tr( "Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filterModel.addItem( tr( "Hi-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "Band-pass csg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filterModel.addItem( tr( "Band-pass czpg" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filterModel.addItem( tr( "Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filterModel.addItem( tr( "All-pass" ), std::make_unique<PixmapLoader>( "filter_ap" ) );
	m_filterModel.addItem( tr( "Moog" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filterModel.addItem( tr( "2x Low-pass" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filterModel.addItem( tr( "RC Low-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filterModel.addItem( tr( "RC Band-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filterModel.addItem( tr( "RC High-pass 12 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "RC Low-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filterModel.addItem( tr( "RC Band-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filterModel.addItem( tr( "RC High-pass 24 dB/oct" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "Vocal Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "2x Moog" ), std::make_unique<PixmapLoader>( "filter_2lp" ) );
	m_filterModel.addItem( tr( "SV Low-pass" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
	m_filterModel.addItem( tr( "SV Band-pass" ), std::make_unique<PixmapLoader>( "filter_bp" ) );
	m_filterModel.addItem( tr( "SV High-pass" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "SV Notch" ), std::make_unique<PixmapLoader>( "filter_notch" ) );
	m_filterModel.addItem( tr( "Fast Formant" ), std::make_unique<PixmapLoader>( "filter_hp" ) );
	m_filterModel.addItem( tr( "Tripole" ), std::make_unique<PixmapLoader>( "filter_lp" ) );
}






float InstrumentSoundShaping::volumeLevel( NotePlayHandle* n, const f_cnt_t frame )
{
	f_cnt_t envReleaseBegin = frame - n->releaseFramesDone() + n->framesBeforeRelease();

	if( n->isReleased() == false )
	{
		envReleaseBegin += Engine::audioEngine()->framesPerPeriod();
	}

	float level;
	getVolumeParameters().fillLevel(&level, frame, envReleaseBegin, 1);

	return level;
}




void InstrumentSoundShaping::processAudioBuffer( SampleFrame* buffer,
							const fpp_t frames,
							NotePlayHandle* n )
{
	const f_cnt_t envTotalFrames = n->totalFramesPlayed();
	f_cnt_t envReleaseBegin = envTotalFrames - n->releaseFramesDone() + n->framesBeforeRelease();

	if( !n->isReleased() || ( n->instrumentTrack()->isSustainPedalPressed() &&
		!n->isReleaseStarted() ) )
	{
		envReleaseBegin += frames;
	}

	// because of optimizations, there's special code for several cases:
	// 	- cut- and res-lfo/envelope active
	// 	- cut-lfo/envelope active
	// 	- res-lfo/envelope active
	//	- no lfo/envelope active but filter is used

	// only use filter, if it is really needed

	auto& cutoffParameters = getCutoffParameters();
	auto& resonanceParameters = getResonanceParameters();

	if( m_filterEnabledModel.value() )
	{
		QVarLengthArray<float> cutBuffer(frames);
		QVarLengthArray<float> resBuffer(frames);

		int old_filter_cut = 0;
		int old_filter_res = 0;

		if( n->m_filter == nullptr )
		{
			n->m_filter = std::make_unique<BasicFilters<>>( Engine::audioEngine()->outputSampleRate() );
		}
		n->m_filter->setFilterType( static_cast<BasicFilters<>::FilterType>(m_filterModel.value()) );

		if (cutoffParameters.isUsed())
		{
			cutoffParameters.fillLevel(cutBuffer.data(), envTotalFrames, envReleaseBegin, frames);
		}

		if (resonanceParameters.isUsed())
		{
			resonanceParameters.fillLevel(resBuffer.data(), envTotalFrames, envReleaseBegin, frames);
		}

		const float fcv = m_filterCutModel.value();
		const float frv = m_filterResModel.value();

		if (cutoffParameters.isUsed() && resonanceParameters.isUsed())
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				const float new_cut_val = EnvelopeAndLfoParameters::expKnobVal( cutBuffer[frame] ) *
								CUT_FREQ_MULTIPLIER + fcv;

				const float new_res_val = frv + RES_MULTIPLIER * resBuffer[frame];

				if( static_cast<int>( new_cut_val ) != old_filter_cut ||
					static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					n->m_filter->calcFilterCoeffs( new_cut_val, new_res_val );
					old_filter_cut = static_cast<int>( new_cut_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				buffer[frame][0] = n->m_filter->update( buffer[frame][0], 0 );
				buffer[frame][1] = n->m_filter->update( buffer[frame][1], 1 );
			}
		}
		else if (cutoffParameters.isUsed())
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				float new_cut_val = EnvelopeAndLfoParameters::expKnobVal( cutBuffer[frame] ) *
								CUT_FREQ_MULTIPLIER + fcv;

				if( static_cast<int>( new_cut_val ) != old_filter_cut )
				{
					n->m_filter->calcFilterCoeffs( new_cut_val, frv );
					old_filter_cut = static_cast<int>( new_cut_val );
				}

				buffer[frame][0] = n->m_filter->update( buffer[frame][0], 0 );
				buffer[frame][1] = n->m_filter->update( buffer[frame][1], 1 );
			}
		}
		else if(resonanceParameters.isUsed() )
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				float new_res_val = frv + RES_MULTIPLIER * resBuffer[frame];

				if( static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					n->m_filter->calcFilterCoeffs( fcv, new_res_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				buffer[frame][0] = n->m_filter->update( buffer[frame][0], 0 );
				buffer[frame][1] = n->m_filter->update( buffer[frame][1], 1 );
			}
		}
		else
		{
			n->m_filter->calcFilterCoeffs( fcv, frv );

			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				buffer[frame][0] = n->m_filter->update( buffer[frame][0], 0 );
				buffer[frame][1] = n->m_filter->update( buffer[frame][1], 1 );
			}
		}
	}

	auto& volumeParameters = getVolumeParameters();

	if (volumeParameters.isUsed())
	{
		QVarLengthArray<float> volBuffer(frames);
		volumeParameters.fillLevel(volBuffer.data(), envTotalFrames, envReleaseBegin, frames);

		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			float vol_level = volBuffer[frame];
			vol_level = vol_level * vol_level;
			buffer[frame][0] = vol_level * buffer[frame][0];
			buffer[frame][1] = vol_level * buffer[frame][1];
		}
	}

/*	else if( m_envLfoParameters[static_cast<std::size_t>(Target::Volume)]->isUsed() == false && m_envLfoParameters[PANNING]->isUsed() )
	{
		// only use panning-envelope...
		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			float vol_level = pan_buf[frame];
			vol_level = vol_level*vol_level;
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				buffer[frame][chnl] = vol_level * buffer[frame][chnl];
			}
		}
	}*/
}




f_cnt_t InstrumentSoundShaping::envFrames( const bool _only_vol ) const
{
	f_cnt_t ret_val = getVolumeParameters().PAHD_Frames();

	if (!_only_vol)
	{
		auto& cutoffParameters = getCutoffParameters();
		if (cutoffParameters.isUsed())
		{
			ret_val = std::max(ret_val, cutoffParameters.PAHD_Frames());
		}

		auto& resonanceParameters = getResonanceParameters();
		if (resonanceParameters.isUsed())
		{
			ret_val = std::max(ret_val, resonanceParameters.PAHD_Frames());
		}
	}

	return ret_val;
}




f_cnt_t InstrumentSoundShaping::releaseFrames() const
{
	if( !m_instrumentTrack->instrument() )
	{
		return 0;
	}

	f_cnt_t ret_val = m_instrumentTrack->instrument()->desiredReleaseFrames();

	if (m_instrumentTrack->instrument()->isSingleStreamed())
	{
		return ret_val;
	}

	auto& volumeParameters = getVolumeParameters();

	if (volumeParameters.isUsed())
	{
		return volumeParameters.releaseFrames();
	}

	auto& cutoffParameters = getCutoffParameters();
	if (cutoffParameters.isUsed())
	{
		ret_val = std::max(ret_val, cutoffParameters.releaseFrames());
	}

	auto& resonanceParameters = getResonanceParameters();
	if (resonanceParameters.isUsed())
	{
		ret_val = std::max(ret_val, resonanceParameters.releaseFrames());
	}

	return ret_val;
}


static void saveEnvelopeAndLFOParameters(EnvelopeAndLfoParameters& p, const QString & tagName, QDomDocument & _doc, QDomElement & _this)
{
	p.saveState(_doc, _this).setTagName(tagName);
}

void InstrumentSoundShaping::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_filterModel.saveSettings( _doc, _this, "ftype" );
	m_filterCutModel.saveSettings( _doc, _this, "fcut" );
	m_filterResModel.saveSettings( _doc, _this, "fres" );
	m_filterEnabledModel.saveSettings( _doc, _this, "fwet" );

	saveEnvelopeAndLFOParameters(getVolumeParameters(), getVolumeNodeName(), _doc, _this);
	saveEnvelopeAndLFOParameters(getCutoffParameters(), getCutoffNodeName(), _doc, _this);
	saveEnvelopeAndLFOParameters(getResonanceParameters(), getResonanceNodeName(), _doc, _this);
}




void InstrumentSoundShaping::loadSettings( const QDomElement & _this )
{
	m_filterModel.loadSettings( _this, "ftype" );
	m_filterCutModel.loadSettings( _this, "fcut" );
	m_filterResModel.loadSettings( _this, "fres" );
	m_filterEnabledModel.loadSettings( _this, "fwet" );

	QDomNode node = _this.firstChild();
	while (!node.isNull())
	{
		if (node.isElement())
		{
			const auto nodeName = node.nodeName();
			if (nodeName == getVolumeNodeName())
			{
				getVolumeParameters().restoreState(node.toElement());
			}
			else if (nodeName == getCutoffNodeName())
			{
				getCutoffParameters().restoreState(node.toElement());
			}
			else if (nodeName == getResonanceNodeName())
			{
				getResonanceParameters().restoreState(node.toElement());
			}
		}

		node = node.nextSibling();
	}
}

QString InstrumentSoundShaping::getVolumeNodeName() const
{
	return getVolumeParameters().nodeName() + "vol";
}

QString InstrumentSoundShaping::getCutoffNodeName() const
{
	return getCutoffParameters().nodeName() + "cut";
}

QString InstrumentSoundShaping::getResonanceNodeName() const
{
	return getResonanceParameters().nodeName() + "res";
}

} // namespace lmms
