#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_sound_shaping.cpp - class instrumentSoundShaping
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>


#include "instrument_sound_shaping.h"
#include "basic_filters.h"
#include "embed.h"
#include "engine.h"
#include "envelope_and_lfo_parameters.h"
#include "instrument_track.h"
#include "note_play_handle.h"



const float CUT_FREQ_MULTIPLIER = 6000.0f;
const float RES_MULTIPLIER = 2.0f;
const float RES_PRECISION = 1000.0f;


// names for env- and lfo-targets - first is name being displayed to user
// and second one is used internally, e.g. for saving/restoring settings
const QString __targetNames[instrumentSoundShaping::NumTargets][3] =
{
	{ instrumentSoundShaping::tr( "VOLUME" ), "vol",
			instrumentSoundShaping::tr( "Volume" ) },
/*	instrumentSoundShaping::tr( "Pan" ),
	instrumentSoundShaping::tr( "Pitch" ),*/
	{ instrumentSoundShaping::tr( "CUTOFF" ), "cut",
			instrumentSoundShaping::tr( "Cutoff frequency" ) },
	{ instrumentSoundShaping::tr( "RESO" ), "res",
			instrumentSoundShaping::tr( "Resonance" ) }
} ;
 


instrumentSoundShaping::instrumentSoundShaping(
					instrumentTrack * _instrument_track ) :
	model( _instrument_track, tr( "Envelopes/LFOs" ) ),
	m_instrumentTrack( _instrument_track ),
	m_filterEnabledModel( false, this ),
	m_filterModel( this, tr( "Filter type" ) ),
	m_filterCutModel( 14000.0, 1.0, 14000.0, 1.0, this,
					tr( "Cutoff frequency" ) ),
	m_filterResModel( 0.5, basicFilters<>::minQ(), 10.0, 0.01, this,
					tr( "Q/Resonance" ) )
{
	for( int i = 0; i < NumTargets; ++i )
	{
		float value_for_zero_amount = 0.0;
		if( i == Volume )
		{
			value_for_zero_amount = 1.0;
		}
		m_envLFOParameters[i] = new envelopeAndLFOParameters(
							value_for_zero_amount, 
							this );
		m_envLFOParameters[i]->setDisplayName(
			tr( __targetNames[i][2].toUtf8().constData() ) );
	}

	m_filterModel.addItem( tr( "LowPass" ),
					new pixmapLoader( "filter_lp" ) );
	m_filterModel.addItem( tr( "HiPass" ),
					new pixmapLoader( "filter_hp" ) );
	m_filterModel.addItem( tr( "BandPass csg" ),
					new pixmapLoader( "filter_bp" ) );
	m_filterModel.addItem( tr( "BandPass czpg" ),
					new pixmapLoader( "filter_bp" ) );
	m_filterModel.addItem( tr( "Notch" ),
					new pixmapLoader( "filter_notch" ) );
	m_filterModel.addItem( tr( "Allpass" ),
					new pixmapLoader( "filter_ap" ) );
	m_filterModel.addItem( tr( "Moog" ),
					new pixmapLoader( "filter_lp" ) );
	m_filterModel.addItem( tr( "2x LowPass" ),
					new pixmapLoader( "filter_2lp" ) );
	m_filterModel.addItem( tr( "RC LowPass" ),
					new pixmapLoader( "filter_lp" ) );
	m_filterModel.addItem( tr( "RC BandPass" ),
					new pixmapLoader( "filter_bp" ) );
	m_filterModel.addItem( tr( "RC HighPass" ),
					new pixmapLoader( "filter_hp" ) );
}




instrumentSoundShaping::~instrumentSoundShaping()
{
}




float instrumentSoundShaping::volumeLevel( notePlayHandle * _n,
							const f_cnt_t _frame )
{
	f_cnt_t release_begin = _frame - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == false )
	{
		release_begin += engine::getMixer()->framesPerPeriod();
	}

	float volume_level;
	m_envLFOParameters[Volume]->fillLevel( &volume_level, _frame,
							release_begin, 1 );

	return( volume_level );
}




void instrumentSoundShaping::processAudioBuffer( sampleFrame * _ab,
							const fpp_t _frames,
							notePlayHandle * _n )
{
	const f_cnt_t total_frames = _n->totalFramesPlayed();
	f_cnt_t release_begin = total_frames - _n->releaseFramesDone() +
						_n->framesBeforeRelease();

	if( _n->released() == false )
	{
		release_begin += engine::getMixer()->framesPerPeriod();
	}

	// because of optimizations, there's special code for several cases:
	// 	- cut- and res-lfo/envelope active
	// 	- cut-lfo/envelope active
	// 	- res-lfo/envelope active
	//	- no lfo/envelope active but filter is used

	// only use filter, if it is really needed

	if( m_filterEnabledModel.value() )
	{
		int old_filter_cut = 0;
		int old_filter_res = 0;

		if( _n->m_filter == NULL )
		{
			_n->m_filter = new basicFilters<>(
				engine::getMixer()->processingSampleRate() );
		}
		_n->m_filter->setFilterType( m_filterModel.value() );

#ifdef __GNUC__
		float cut_buf[_frames];
		float res_buf[_frames];
#else
		float * cut_buf = NULL;
		float * res_buf = NULL;
#endif

		if( m_envLFOParameters[Cut]->used() )
		{
#ifndef __GNUC__
			cut_buf = new float[_frames];
#endif
			m_envLFOParameters[Cut]->fillLevel( cut_buf, total_frames,
						release_begin, _frames );
		}
		if( m_envLFOParameters[Resonance]->used() )
		{
#ifndef __GNUC__
			res_buf = new float[_frames];
#endif
			m_envLFOParameters[Resonance]->fillLevel( res_buf,
						total_frames, release_begin,
								_frames );
		}

		const float fcv = m_filterCutModel.value();
		const float frv = m_filterResModel.value();

		if( m_envLFOParameters[Cut]->used() &&
			m_envLFOParameters[Resonance]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				const float new_cut_val = envelopeAndLFOParameters::expKnobVal( cut_buf[frame] ) *
								CUT_FREQ_MULTIPLIER + fcv;

				const float new_res_val = frv + RES_MULTIPLIER * res_buf[frame];

				if( static_cast<int>( new_cut_val ) != old_filter_cut ||
					static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( new_cut_val, new_res_val );
					old_filter_cut = static_cast<int>( new_cut_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				_ab[frame][0] = _n->m_filter->update( _ab[frame][0], 0 );
				_ab[frame][1] = _n->m_filter->update( _ab[frame][1], 1 );
			}
		}
		else if( m_envLFOParameters[Cut]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				float new_cut_val = envelopeAndLFOParameters::expKnobVal( cut_buf[frame] ) *
								CUT_FREQ_MULTIPLIER + fcv;

				if( static_cast<int>( new_cut_val ) != old_filter_cut )
				{
					_n->m_filter->calcFilterCoeffs( new_cut_val, frv );
					old_filter_cut = static_cast<int>( new_cut_val );
				}

				_ab[frame][0] = _n->m_filter->update( _ab[frame][0], 0 );
				_ab[frame][1] = _n->m_filter->update( _ab[frame][1], 1 );
			}
		}
		else if( m_envLFOParameters[Resonance]->used() )
		{
			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				float new_res_val = frv + RES_MULTIPLIER * res_buf[frame];

				if( static_cast<int>( new_res_val*RES_PRECISION ) != old_filter_res )
				{
					_n->m_filter->calcFilterCoeffs( fcv, new_res_val );
					old_filter_res = static_cast<int>( new_res_val*RES_PRECISION );
				}

				_ab[frame][0] = _n->m_filter->update( _ab[frame][0], 0 );
				_ab[frame][1] = _n->m_filter->update( _ab[frame][1], 1 );
			}
		}
		else
		{
			_n->m_filter->calcFilterCoeffs( fcv, frv );

			for( fpp_t frame = 0; frame < _frames; ++frame )
			{
				_ab[frame][0] = _n->m_filter->update( _ab[frame][0], 0 );
				_ab[frame][1] = _n->m_filter->update( _ab[frame][1], 1 );
			}
		}

#ifndef __GNUC__
		delete[] cut_buf;
		delete[] res_buf;
#endif
	}

	if( m_envLFOParameters[Volume]->used() )
	{
#ifdef __GNUC__
		float vol_buf[_frames];
#else
		float * vol_buf = new float[_frames];
#endif
		m_envLFOParameters[Volume]->fillLevel( vol_buf, total_frames,
						release_begin, _frames );

		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			float vol_level = vol_buf[frame];
			vol_level = vol_level * vol_level;
			_ab[frame][0] = vol_level * _ab[frame][0];
			_ab[frame][1] = vol_level * _ab[frame][1];
		}
#ifndef __GNUC__
		delete[] vol_buf;
#endif
	}

/*	else if( m_envLFOParameters[Volume]->used() == false && m_envLFOParameters[PANNING]->used() )
	{
		// only use panning-envelope...
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			float vol_level = pan_buf[frame];
			vol_level = vol_level*vol_level;
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				_ab[frame][chnl] = vol_level * _ab[frame][chnl];
			}
		}
	}*/
}




f_cnt_t instrumentSoundShaping::envFrames( const bool _only_vol ) const
{
	f_cnt_t ret_val = m_envLFOParameters[Volume]->PAHD_Frames();

	if( _only_vol == false )
	{
		for( int i = Volume+1; i < NumTargets; ++i )
		{
			if( m_envLFOParameters[i]->used() &&
				m_envLFOParameters[i]->PAHD_Frames() > ret_val )
			{
				ret_val = m_envLFOParameters[i]->PAHD_Frames();
			}
		}
	}
	return ret_val;
}




f_cnt_t instrumentSoundShaping::releaseFrames( void ) const
{
	f_cnt_t ret_val = m_envLFOParameters[Volume]->used() ?
				m_envLFOParameters[Volume]->releaseFrames() : 0;
	if( m_instrumentTrack->getInstrument()->desiredReleaseFrames() >
								ret_val )
	{
		ret_val = m_instrumentTrack->getInstrument()->
							desiredReleaseFrames();
	}

	if( m_envLFOParameters[Volume]->used() == false )
	{
		for( int i = Volume+1; i < NumTargets; ++i )
		{
			if( m_envLFOParameters[i]->used() &&
				m_envLFOParameters[i]->releaseFrames() >
								ret_val )
			{
				ret_val = m_envLFOParameters[i]->releaseFrames();
			}
		}
	}
	return ret_val;
}




void instrumentSoundShaping::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_filterModel.saveSettings( _doc, _this, "ftype" );
	m_filterCutModel.saveSettings( _doc, _this, "fcut" );
	m_filterResModel.saveSettings( _doc, _this, "fres" );
	m_filterEnabledModel.saveSettings( _doc, _this, "fwet" );

	for( int i = 0; i < NumTargets; ++i )
	{
		m_envLFOParameters[i]->saveState( _doc, _this ).setTagName(
			m_envLFOParameters[i]->nodeName() +
				QString( __targetNames[i][1] ).toLower() );
	}
}




void instrumentSoundShaping::loadSettings( const QDomElement & _this )
{
	m_filterModel.loadSettings( _this, "ftype" );
	m_filterCutModel.loadSettings( _this, "fcut" );
	m_filterResModel.loadSettings( _this, "fres" );
	m_filterEnabledModel.loadSettings( _this, "fwet" );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			for( int i = 0; i < NumTargets; ++i )
			{
				if( node.nodeName() ==
					m_envLFOParameters[i]->nodeName() +
					QString( __targetNames[i][1] ).
								toLower() )
				{
					m_envLFOParameters[i]->restoreState(
							node.toElement() );
				}
			}
		}
		node = node.nextSibling();
	}
}




#include "moc_instrument_sound_shaping.cxx"

#endif
