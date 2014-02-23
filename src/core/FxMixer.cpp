/*
 * FxMixer.cpp - effect mixer for LMMS
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FxMixer.h"
#include "Effect.h"
#include "song.h"


FxChannel::FxChannel( Model * _parent ) :
	m_fxChain( NULL ),
	m_used( false ),
	m_stillRunning( false ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( new sampleFrame[engine::mixer()->framesPerPeriod()] ),
	m_muteModel( false, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.01, _parent ),
	m_name(),
	m_lock()
{
	engine::mixer()->clearAudioBuffer( m_buffer,
					engine::mixer()->framesPerPeriod() );
}




FxChannel::~FxChannel()
{
	delete[] m_buffer;
}






FxMixer::FxMixer() :
	JournallingObject(),
	Model( NULL )
{
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		m_fxChannels[i] = new FxChannel( this );
	}
	// reset name etc.
	clear();
}




FxMixer::~FxMixer()
{
	for( int i = 0; i < NumFxChannels+1; ++i )
	{
		delete m_fxChannels[i];
	}
}




void FxMixer::mixToChannel( const sampleFrame * _buf, fx_ch_t _ch )
{
	if( m_fxChannels[_ch]->m_muteModel.value() == false )
	{
		m_fxChannels[_ch]->m_lock.lock();
		sampleFrame * buf = m_fxChannels[_ch]->m_buffer;
		for( f_cnt_t f = 0; f < engine::mixer()->framesPerPeriod(); ++f )
		{
			buf[f][0] += _buf[f][0];
			buf[f][1] += _buf[f][1];
		}
		m_fxChannels[_ch]->m_used = true;
		m_fxChannels[_ch]->m_lock.unlock();
	}
}




void FxMixer::processChannel( fx_ch_t _ch, sampleFrame * _buf )
{
	if( m_fxChannels[_ch]->m_muteModel.value() == false &&
		( m_fxChannels[_ch]->m_used || m_fxChannels[_ch]->m_stillRunning || _ch == 0 ) )
	{
		if( _buf == NULL )
		{
			_buf = m_fxChannels[_ch]->m_buffer;
		}
		const fpp_t f = engine::mixer()->framesPerPeriod();

		// only start effects if sound was mixed to this FX channel before
		if( m_fxChannels[_ch]->m_used )
		{
			m_fxChannels[_ch]->m_fxChain.startRunning();
		}

		// process FX chain
		m_fxChannels[_ch]->m_stillRunning = m_fxChannels[_ch]->m_fxChain.processAudioBuffer( _buf, f, m_fxChannels[_ch]->m_used );

		float peakLeft = engine::mixer()->peakValueLeft( _buf, f ) * m_fxChannels[_ch]->m_volumeModel.value();
		float peakRight = engine::mixer()->peakValueRight( _buf, f ) * m_fxChannels[_ch]->m_volumeModel.value();

		if( peakLeft > m_fxChannels[_ch]->m_peakLeft )
		{
			m_fxChannels[_ch]->m_peakLeft = peakLeft;
		}
		if( peakRight > m_fxChannels[_ch]->m_peakRight )
		{
			m_fxChannels[_ch]->m_peakRight = peakRight;
		}

		m_fxChannels[_ch]->m_used = true;
	}
	else
	{
		m_fxChannels[_ch]->m_peakLeft = m_fxChannels[_ch]->m_peakRight = 0.0f; 
	}
}




void FxMixer::prepareMasterMix()
{
	engine::mixer()->clearAudioBuffer( m_fxChannels[0]->m_buffer,
					engine::mixer()->framesPerPeriod() );
}




void FxMixer::masterMix( sampleFrame * _buf )
{
	const int fpp = engine::mixer()->framesPerPeriod();
	memcpy( _buf, m_fxChannels[0]->m_buffer, sizeof( sampleFrame ) * fpp );

	for( int i = 1; i < NumFxChannels+1; ++i )
	{
		if( m_fxChannels[i]->m_used )
		{
			sampleFrame * ch_buf = m_fxChannels[i]->m_buffer;
			const float v = m_fxChannels[i]->m_volumeModel.value();
			for( f_cnt_t f = 0; f < fpp; ++f )
			{
				_buf[f][0] += ch_buf[f][0] * v;
				_buf[f][1] += ch_buf[f][1] * v;
			}
			engine::mixer()->clearAudioBuffer( ch_buf,
					engine::mixer()->framesPerPeriod() );
			m_fxChannels[i]->m_used = false;
		}
	}

	processChannel( 0, _buf );

	if( m_fxChannels[0]->m_muteModel.value() )
	{
		engine::mixer()->clearAudioBuffer( _buf,
					engine::mixer()->framesPerPeriod() );
		return;
	}

	const float v = m_fxChannels[0]->m_volumeModel.value();
	for( f_cnt_t f = 0; f < engine::mixer()->framesPerPeriod(); ++f )
	{
		_buf[f][0] *= v;
		_buf[f][1] *= v;
	}

	m_fxChannels[0]->m_peakLeft *= engine::mixer()->masterGain();
	m_fxChannels[0]->m_peakRight *= engine::mixer()->masterGain();
}




void FxMixer::clear()
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		m_fxChannels[i]->m_fxChain.clear();
		m_fxChannels[i]->m_volumeModel.setValue( 1.0f );
		m_fxChannels[i]->m_muteModel.setValue( false );
		m_fxChannels[i]->m_name = ( i == 0 ) ?
				tr( "Master" ) : tr( "FX %1" ).arg( i );
		m_fxChannels[i]->m_volumeModel.setDisplayName( 
				m_fxChannels[i]->m_name );

	}
}




void FxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i <= NumFxChannels; ++i )
	{
		QDomElement fxch = _doc.createElement( QString( "fxchannel" ) );
		_this.appendChild( fxch );
		m_fxChannels[i]->m_fxChain.saveState( _doc, fxch );
		m_fxChannels[i]->m_volumeModel.saveSettings( _doc, fxch,
								"volume" );
		m_fxChannels[i]->m_muteModel.saveSettings( _doc, fxch,
								"muted" );
		fxch.setAttribute( "num", i );
		fxch.setAttribute( "name", m_fxChannels[i]->m_name );
	}
}




void FxMixer::loadSettings( const QDomElement & _this )
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
		m_fxChannels[num]->m_muteModel.loadSettings( fxch, "muted" );
		m_fxChannels[num]->m_name = fxch.attribute( "name" );
		node = node.nextSibling();
	}

	emit dataChanged();
}

