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
	Model( NULL ),
	m_fxChannels()
{
	// create master channel
	createChannel();
}



FxMixer::~FxMixer()
{
	for( int i = 0; i < m_fxChannels.size(); ++i )
	{
		for( int j = 0; j < m_fxChannels[i]->m_sendAmount.size(); ++j)
		{
			delete m_fxChannels[i]->m_sendAmount[j];
		}
		delete m_fxChannels[i];
	}
}



int FxMixer::createChannel()
{
	// create new channel
	m_fxChannels.push_back(new FxChannel( this ));

	// reset channel state
	int index = m_fxChannels.size() - 1;
	clearChannel(index);

	return index;
}



void FxMixer::createChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel,
								float amount)
{
	// find the existing connection
	FxChannel * from = m_fxChannels[fromChannel];
	for(int i=0; i<from->m_sends.size(); ++i){
		if( from->m_sends[i] == toChannel )
		{
			// simply adjust the amount
			from->m_sendAmount[i]->setValue(amount);
			return;
		}
	}

	// connection does not exist. create a new one

	// add to from's sends
	from->m_sends.push_back(toChannel);
	from->m_sendAmount.push_back(new FloatModel(amount, 0, 1, 0.001, NULL,
												tr("Amount to send")));

	// add to to's receives
	m_fxChannels[toChannel]->m_receives.push_back(fromChannel);

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
			delete from->m_sendAmount[i];
			from->m_sendAmount.remove(i);
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



// how much does fromChannel send its output to the input of toChannel?
FloatModel * FxMixer::channelSendModel(fx_ch_t fromChannel, fx_ch_t toChannel)
{
	FxChannel * from = m_fxChannels[fromChannel];
	for(int i=0; i<from->m_sends.size(); ++i){
		if( from->m_sends[i] == toChannel )
			return from->m_sendAmount[i];
	}
	return NULL;
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
	if( _buf == NULL )
	{
		_buf = thisCh->m_buffer;
	}

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
			float amt = channelSendModel(senderIndex, _ch)->value();
			sampleFrame * ch_buf = sender->m_buffer;
			const float v = sender->m_volumeModel.value();
			for( f_cnt_t f = 0; f < fpp; ++f )
			{
				_buf[f][0] += ch_buf[f][0] * v * amt;
				_buf[f][1] += ch_buf[f][1] * v * amt;
			}
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
	for( int i = 0; i < m_fxChannels.size(); ++i )
	{
		clearChannel(i);
	}
}


void FxMixer::clearChannel(fx_ch_t index)
{
	FxChannel * ch = m_fxChannels[index];
	ch->m_fxChain.clear();
	ch->m_volumeModel.setValue( 1.0f );
	ch->m_muteModel.setValue( false );
	ch->m_name = ( index == 0 ) ? tr( "Master" ) : tr( "FX %1" ).arg( index );
	ch->m_volumeModel.setDisplayName(ch->m_name );

	// send only to master
	if( index > 0)
	{
		// delete existing sends
		for( int i=0; i<ch->m_sends.size(); ++i)
		{
			deleteChannelSend(index, ch->m_sends[i]);
		}

		// add send to master
		createChannelSend(index, 0);
	}

	// delete receives
	for( int i=0; i<ch->m_receives.size(); ++i)
	{
		deleteChannelSend(ch->m_receives[i], index);
	}

}

void FxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i < m_fxChannels.size(); ++i )
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
	for( int i = 0; i <= 64; ++i ) // TODO make this work
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


