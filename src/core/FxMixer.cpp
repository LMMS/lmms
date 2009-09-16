/*
 * FxMixer.cpp - effect mixer for LMMS
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "Cpu.h"
#include "Effect.h"
#include "song.h"


FxChannel::FxChannel( Model * _parent ) :
	m_fxChain( NULL ),
	m_stillRunning( false ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( CPU::allocFrames( engine::getMixer()->framesPerPeriod() ) ),
	m_muteModel( false, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.01, _parent ),
	m_name(),
	m_lock()
{
	engine::getMixer()->clearAudioBuffer( m_buffer,
					engine::getMixer()->framesPerPeriod() );
}




FxChannel::~FxChannel()
{
	CPU::freeFrames( m_buffer );
}






FxMixer::FxMixer() :
	JournallingObject(),
	Model( NULL )
{
	// create master channel
	m_fxChannels[0] = new FxChannel(this);

	// create the rest of the channels
	for( int i = 1; i < NumFxChannels+1; ++i )
	{
		// create new channel
		m_fxChannels[i] = new FxChannel( this );

		// send the channel into master
		createChannelSend(i, 0);
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



void FxMixer::createChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel)
{
	// first make sure the send doesn't already exist
	if( ! channelSendsTo(fromChannel, toChannel) )
	{
		// add to from's sends
		m_fxChannels[fromChannel]->m_sends.push_back(toChannel);

		// add to to's receives
		m_fxChannels[toChannel]->m_receives.push_back(fromChannel);
	}
}



// delete the connection made by createChannelSend
void FxMixer::deleteChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel)
{
	// delete the send
	FxChannel * from = m_fxChannels[fromChannel];
	FxChannel * to   = m_fxChannels[toChannel];

	// find and delete the send entry
	for(int i=0; i<from->m_sends.size(); ++i) {
		if( from->m_sends[i] == toChannel )
		{
			// delete this index
			from->m_sends.remove(i);
			break;
		}
	}

	// find and delete the receive entry
	for(int i=0; i<to->m_receives.size(); ++i)
	{
		if( to->m_receives[i] == fromChannel )
		{
			// delete this index
			to->m_receives.remove(i);
			break;
		}
	}
}



// does fromChannel send its output to the input of toChannel?
bool FxMixer::channelSendsTo(fx_ch_t fromChannel, fx_ch_t toChannel)
{
	FxChannel * from = m_fxChannels[fromChannel];
	for(int i=0; i<from->m_sends.size(); ++i){
		if( from->m_sends[i] == toChannel )
			return true;
	}
	return false;
}



void FxMixer::mixToChannel( const sampleFrame * _buf, fx_ch_t _ch )
{
	if( m_fxChannels[_ch]->m_muteModel.value() == false )
	{
		m_fxChannels[_ch]->m_lock.lock();
		CPU::bufMix( m_fxChannels[_ch]->m_buffer, _buf,
						engine::getMixer()->framesPerPeriod() );
		m_fxChannels[_ch]->m_lock.unlock();
	}
}




void FxMixer::processChannel( fx_ch_t _ch, sampleFrame * _buf )
{
	const fpp_t fpp = engine::getMixer()->framesPerPeriod();
	FxChannel * thisCh = m_fxChannels[_ch];
	if( ! thisCh->m_muteModel.value() )
	{
		// do mixer sends. loop through the channels that send to this one
		for( int i = 0; i < thisCh->m_receives.size(); ++i)
		{
			fx_ch_t senderIndex = thisCh->m_receives[i];
			FxChannel * sender = m_fxChannels[senderIndex];

			// compute the sending channel
			processChannel( senderIndex );

			// mix it with this one
			sampleFrame * ch_buf = sender->m_buffer;
			const float v = sender->m_volumeModel.value();
			for( f_cnt_t f = 0; f < fpp; ++f )
			{
				_buf[f][0] += ch_buf[f][0] * v;
				_buf[f][1] += ch_buf[f][1] * v;
			}
			engine::getMixer()->clearAudioBuffer( ch_buf,
					engine::getMixer()->framesPerPeriod() );
		}


		if( _buf == NULL )
		{
			_buf = thisCh->m_buffer;
		}
		const float v = thisCh->m_volumeModel.value();

		thisCh->m_fxChain.startRunning();
		thisCh->m_stillRunning = thisCh->
			m_fxChain.processAudioBuffer( _buf, fpp);
		thisCh->m_peakLeft =
			engine::getMixer()->peakValueLeft( _buf, fpp ) * v;
		thisCh->m_peakRight =
			engine::getMixer()->peakValueRight( _buf, fpp ) * v;
	}
	else
	{
		thisCh->m_peakLeft = thisCh->m_peakRight = 0.0f;
	}
}




void FxMixer::prepareMasterMix()
{
	engine::getMixer()->clearAudioBuffer( m_fxChannels[0]->m_buffer,
					engine::getMixer()->framesPerPeriod() );
}




void FxMixer::masterMix( sampleFrame * _buf )
{
	const int fpp = engine::getMixer()->framesPerPeriod();
	memcpy( _buf, m_fxChannels[0]->m_buffer, sizeof( sampleFrame ) * fpp );

	processChannel( 0, _buf );

	/*if( m_fxChannels[0]->m_muteModel.value() )
	{
		engine::getMixer()->clearAudioBuffer( _buf,
					engine::getMixer()->framesPerPeriod() );
		return;
	}*/

	const float v = m_fxChannels[0]->m_volumeModel.value();
	for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod(); ++f )
	{
		_buf[f][0] *= v;
		_buf[f][1] *= v;
	}

	m_fxChannels[0]->m_peakLeft *= engine::getMixer()->masterGain();
	m_fxChannels[0]->m_peakRight *= engine::getMixer()->masterGain();
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


