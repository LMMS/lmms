#ifndef SINGLE_SOURCE_COMPILE

/*
 * fx_mixer.cpp - effect-mixer for LMMS
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "fx_mixer.h"
#include "effect.h"
#include "song.h"


fxChannel::fxChannel( model * _parent ) :
	m_fxChain( NULL ),
	m_used( FALSE ),
	m_stillRunning( FALSE ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( new sampleFrame[engine::getMixer()->framesPerPeriod()] ),
	m_muteModel( FALSE, _parent ),
	m_soloModel( FALSE, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.01, _parent ),
	m_name(),
	m_lock()
{
	engine::getMixer()->clearAudioBuffer( m_buffer,
					engine::getMixer()->framesPerPeriod() );
	m_volumeModel.setTrack( engine::getSong()->getAutomationTrack() );
}




fxChannel::~fxChannel()
{
	delete[] m_buffer;
}






fxMixer::fxMixer() :
	journallingObject(),
	model( NULL ),
	m_out( new surroundSampleFrame[
				engine::getMixer()->framesPerPeriod()] )
{
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		m_fxChannels[i] = new fxChannel( this );
	}
	// reset name etc.
	clear();
}




fxMixer::~fxMixer()
{
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		delete m_fxChannels[i];
	}
}




void fxMixer::mixToChannel( const sampleFrame * _buf, fx_ch_t _ch )
{
	m_fxChannels[_ch]->m_lock.lock();
	sampleFrame * buf = m_fxChannels[_ch]->m_buffer;
	for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod(); ++f )
	{
		buf[f][0] += _buf[f][0];
		buf[f][1] += _buf[f][1];
	}
	m_fxChannels[_ch]->m_used = TRUE;
	m_fxChannels[_ch]->m_lock.unlock();
}




void fxMixer::processChannel( fx_ch_t _ch )
{
	if( m_fxChannels[_ch]->m_used || m_fxChannels[_ch]->m_stillRunning ||
								_ch == 0 )
	{
		const fpp_t f = engine::getMixer()->framesPerPeriod();
		m_fxChannels[_ch]->m_fxChain.startRunning();
		m_fxChannels[_ch]->m_stillRunning =
			m_fxChannels[_ch]->m_fxChain.processAudioBuffer(
					m_fxChannels[_ch]->m_buffer, f );
		m_fxChannels[_ch]->m_peakLeft =
			engine::getMixer()->peakValueLeft(
					m_fxChannels[_ch]->m_buffer, f ) *
				m_fxChannels[_ch]->m_volumeModel.value();
		m_fxChannels[_ch]->m_peakRight =
			engine::getMixer()->peakValueRight(
					m_fxChannels[_ch]->m_buffer, f ) *
				m_fxChannels[_ch]->m_volumeModel.value();
		m_fxChannels[_ch]->m_used = TRUE;
	}
	else
	{
		m_fxChannels[_ch]->m_peakLeft =
					m_fxChannels[_ch]->m_peakRight = 0.0f; 
	}
}




void fxMixer::prepareMasterMix( void )
{
	engine::getMixer()->clearAudioBuffer( m_fxChannels[0]->m_buffer,
					engine::getMixer()->framesPerPeriod() );
}




const surroundSampleFrame * fxMixer::masterMix( void )
{
	sampleFrame * buf = m_fxChannels[0]->m_buffer;
	for( int i = 1; i < NumFxChannels+1; ++i )
	{
		if( m_fxChannels[i]->m_used )
		{
			sampleFrame * ch_buf = m_fxChannels[i]->m_buffer;
			const float v = m_fxChannels[i]->m_volumeModel.value();
			for( f_cnt_t f = 0; f <
				engine::getMixer()->framesPerPeriod(); ++f )
			{
				buf[f][0] += ch_buf[f][0] * v;
				buf[f][1] += ch_buf[f][1] * v;
			}
			engine::getMixer()->clearAudioBuffer( ch_buf,
					engine::getMixer()->framesPerPeriod() );
			m_fxChannels[i]->m_used = FALSE;
		}
	}

	processChannel( 0 );

	const float v = m_fxChannels[0]->m_volumeModel.value();
	for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod(); ++f )
	{
		for( ch_cnt_t ch = 0; ch < SURROUND_CHANNELS; ++ch )
		{
			m_out[f][ch] = buf[f][ch%DEFAULT_CHANNELS] * v;
		}
	}

	m_fxChannels[0]->m_peakLeft *= engine::getMixer()->masterGain();
	m_fxChannels[0]->m_peakRight *= engine::getMixer()->masterGain();

	return( m_out );
}




void fxMixer::clear()
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		m_fxChannels[i]->m_fxChain.clear();
		m_fxChannels[i]->m_volumeModel.setValue( 1.0f );
		m_fxChannels[i]->m_name = ( i == 0 ) ?
				tr( "Master" ) : tr( "FX %1" ).arg( i );
		m_fxChannels[i]->m_volumeModel.setDisplayName( 
				m_fxChannels[i]->m_name );

	}
}




void fxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		QDomElement fxch = _doc.createElement( QString( "fxchannel" ) );
		_this.appendChild( fxch );
		m_fxChannels[i]->m_fxChain.saveState( _doc, fxch );
		m_fxChannels[i]->m_volumeModel.saveSettings( _doc, fxch,
								"volume" );
		fxch.setAttribute( "num", i );
		fxch.setAttribute( "name", m_fxChannels[i]->m_name );
	}
}




void fxMixer::loadSettings( const QDomElement & _this )
{
	clear();
	QDomNode node = _this.firstChild();
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		QDomElement fxch = node.toElement();
		int num = fxch.attribute( "num" ).toInt();
		m_fxChannels[num]->m_fxChain.restoreState(
			fxch.firstChildElement(
				m_fxChannels[num]->m_fxChain.nodeName() ) );
		m_fxChannels[num]->m_volumeModel.loadSettings( fxch, "volume" );
		m_fxChannels[num]->m_name = fxch.attribute( "name" );
		node = node.nextSibling();
	}

	emit dataChanged();
}


#endif
