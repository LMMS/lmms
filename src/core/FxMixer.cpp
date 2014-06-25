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
#include "MixerWorkerThread.h"
#include "MixHelpers.h"
#include "Effect.h"
#include "song.h"

#include "InstrumentTrack.h"
#include "bb_track_container.h"


FxChannel::FxChannel( int idx, Model * _parent ) :
	m_fxChain( NULL ),
	m_hasInput( false ),
	m_stillRunning( false ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( new sampleFrame[engine::mixer()->framesPerPeriod()] ),
	m_muteModel( false, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.01, _parent ),
	m_name(),
	m_lock(),
	m_channelIndex( idx ),
	m_queued( false )
{
	engine::mixer()->clearAudioBuffer( m_buffer,
					engine::mixer()->framesPerPeriod() );
}




FxChannel::~FxChannel()
{
	delete[] m_buffer;
}




void FxChannel::doProcessing( sampleFrame * _buf )
{
	FxMixer * fxm = engine::fxMixer();
	const fpp_t fpp = engine::mixer()->framesPerPeriod();

	// <tobydox> ignore the passed _buf
	// <tobydox> always use m_buffer
	// <tobydox> this is just an auxilliary buffer if doProcessing()
	//			 needs one for processing while running
	// <tobydox> particularly important for playHandles, so Instruments
	//			 can operate on this buffer the whole time
	// <tobydox> this improves cache hit rate
	_buf = m_buffer;

	// SMF: OK, due to the fact, that the data from the audio-tracks has been
	//			written into our buffer already, all which needs to be done at this
	//			stage is to process inter-channel sends. I really don't like the idea
	//			of using threads for this -- it just doesn't make any sense and wastes
	//			cpu-cylces... so I just go through every child of this channel and
	//			call the acc. doProcessing() directly.

	if( m_muteModel.value() == false )
	{
		// OK, we are not muted, so we go recursively through all the channels
		// which send to us (our children)...
		foreach( fx_ch_t senderIndex, m_receives )
		{
			FxChannel * sender = fxm->effectChannel( senderIndex );

			// wait for the sender job - either it's just been queued yet,
			// then ThreadableJob::process() will process it now within this
			// thread - otherwise it has been is is being processed by another
			// thread and we just have to wait for it to finish
			while( sender->state() != ThreadableJob::Done )
			{
				sender->process();
			}

			if( sender->m_hasInput || sender->m_stillRunning )
			{
				// get the send level...
				const float amt =
					fxm->channelSendModel( senderIndex, m_channelIndex )->value();

				// mix it's output with this one's output
				sampleFrame * ch_buf = sender->m_buffer;
				const float v = sender->m_volumeModel.value() * amt;
				for( f_cnt_t f = 0; f < fpp; ++f )
				{
					_buf[f][0] += ch_buf[f][0] * v;
					_buf[f][1] += ch_buf[f][1] * v;
				}
			}
			
			// if sender channel hasInput, then we hasInput too
			if( sender->m_hasInput ) m_hasInput = true;
		}
	}

	const float v = m_volumeModel.value();

	if( m_hasInput ) 
	{
		// only start fxchain when we have input...
		m_fxChain.startRunning(); 
	}
	if( m_hasInput || m_stillRunning )
	{
		m_stillRunning = m_fxChain.processAudioBuffer( _buf, fpp, m_hasInput );
	}
	m_peakLeft = qMax( m_peakLeft, engine::mixer()->peakValueLeft( _buf, fpp ) * v );
	m_peakRight = qMax( m_peakRight, engine::mixer()->peakValueRight( _buf, fpp ) * v );
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
	const int index = m_fxChannels.size();
	// create new channel
	m_fxChannels.push_back( new FxChannel( index, this ) );

	// reset channel state
	clearChannel( index );

	return index;
}


void FxMixer::deleteChannel(int index)
{
	m_fxChannels[index]->m_lock.lock();

	// go through every instrument and adjust for the channel index change
	TrackContainer::TrackList tracks;
	tracks += engine::getSong()->tracks();
	tracks += engine::getBBTrackContainer()->tracks();

	foreach( track* t, tracks )
	{
		if( t->type() == track::InstrumentTrack )
		{
			InstrumentTrack* inst = dynamic_cast<InstrumentTrack *>( t );
			int val = inst->effectChannelModel()->value(0);
			if( val == index )
			{
				// we are deleting this track's fx send
				// send to master
				inst->effectChannelModel()->setValue(0);
			}
			else if( val > index )
			{
				// subtract 1 to make up for the missing channel
				inst->effectChannelModel()->setValue(val-1);
			}
		}
	}
	
	// delete all of this channel's sends and receives
	while( ! m_fxChannels[index]->m_sends.isEmpty() )
	{
		deleteChannelSend( index, m_fxChannels[index]->m_sends.first() );
	}
	while( ! m_fxChannels[index]->m_receives.isEmpty() )
	{
		deleteChannelSend( m_fxChannels[index]->m_receives.first(), index );
	}

	for(int i=0; i<m_fxChannels.size(); ++i)
	{
		// for every send/receive, adjust for the channel index change
		for(int j=0; j<m_fxChannels[i]->m_sends.size(); ++j)
		{
			if( m_fxChannels[i]->m_sends[j] > index )
			{
				// subtract 1 to make up for the missing channel
				--m_fxChannels[i]->m_sends[j];
			}
		}
		for(int j=0; j<m_fxChannels[i]->m_receives.size(); ++j)
		{
			if( m_fxChannels[i]->m_receives[j] > index )
			{
				// subtract 1 to make up for the missing channel
				--m_fxChannels[i]->m_receives[j];
			}
		}

	}

	// actually delete the channel
	delete m_fxChannels[index];
	m_fxChannels.remove(index);
	
	// check names of all channels above the removal
	for( int i = index; i < m_fxChannels.size(); ++i )
	{
		validateChannelName( i, i + 1 );
	}
}



void FxMixer::moveChannelLeft(int index)
{
	// can't move master or first channel
	if( index <= 1 || index >= m_fxChannels.size() )
	{
		return;
	}
	m_sendsMutex.lock();
	// channels to swap
	int a = index - 1, b = index;

	// go through every instrument and adjust for the channel index change
	QVector<track *> songTrackList = engine::getSong()->tracks();
	QVector<track *> bbTrackList = engine::getBBTrackContainer()->tracks();

	QVector<track *> trackLists[] = {songTrackList, bbTrackList};
	for(int tl=0; tl<2; ++tl)
	{
		QVector<track *> trackList = trackLists[tl];
		for(int i=0; i<trackList.size(); ++i)
		{
			if( trackList[i]->type() == track::InstrumentTrack )
			{
				InstrumentTrack * inst = (InstrumentTrack *) trackList[i];
				int val = inst->effectChannelModel()->value(0);
				if( val == a )
				{
					inst->effectChannelModel()->setValue(b);
				}
				else if( val == b )
				{
					inst->effectChannelModel()->setValue(a);
				}

			}
		}
	}

	for(int i=0; i<m_fxChannels.size(); ++i)
	{
		// for every send/receive, adjust for the channel index change
		for(int j=0; j<m_fxChannels[i]->m_sends.size(); ++j)
		{
			if( m_fxChannels[i]->m_sends[j] == a )
			{
				m_fxChannels[i]->m_sends[j] = b;
			}
			else if( m_fxChannels[i]->m_sends[j] == b )
			{
				m_fxChannels[i]->m_sends[j] = a;
			}
		}
		for(int j=0; j<m_fxChannels[i]->m_receives.size(); ++j)
		{
			if( m_fxChannels[i]->m_receives[j] == a )
			{
				m_fxChannels[i]->m_receives[j] = b;
			}
			else if( m_fxChannels[i]->m_receives[j] == b )
			{
				m_fxChannels[i]->m_receives[j] = a;
			}
		}
	}

	// actually do the swap
	FxChannel * tmpChannel = m_fxChannels[a];
	m_fxChannels[a] = m_fxChannels[b];
	m_fxChannels[b] = tmpChannel;
	
	// check names
	validateChannelName( a, b );
	validateChannelName( b, a );

	m_sendsMutex.unlock();
}



void FxMixer::moveChannelRight(int index)
{
	moveChannelLeft(index+1);
}


void FxMixer::validateChannelName( int index, int oldIndex )
{
	FxChannel * fxc = m_fxChannels[ index ];
	if( fxc->m_name == tr( "FX %1" ).arg( oldIndex ) )
	{
		fxc->m_name = tr( "FX %1" ).arg( index );
	}
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
	m_sendsMutex.lock();
	// add to from's sends
	from->m_sends.push_back(toChannel);
	from->m_sendAmount.push_back(new FloatModel(amount, 0, 1, 0.001, NULL,
												tr("Amount to send")));

	// add to to's receives
	m_fxChannels[toChannel]->m_receives.push_back(fromChannel);
	m_sendsMutex.unlock();
}



// delete the connection made by createChannelSend
void FxMixer::deleteChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel)
{
	// delete the send
	FxChannel * from = m_fxChannels[fromChannel];
	FxChannel * to	 = m_fxChannels[toChannel];
	m_sendsMutex.lock();
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
	m_sendsMutex.unlock();
}


bool FxMixer::isInfiniteLoop(fx_ch_t sendFrom, fx_ch_t sendTo) {
	// can't send master to anything
	if( sendFrom == 0 ) return true;

	// can't send channel to itself
	if( sendFrom == sendTo ) return true;

	// follow sendTo's outputs recursively looking for something that sends
	// to sendFrom
	for(int i=0; i<m_fxChannels[sendTo]->m_sends.size(); ++i)
	{
		if( isInfiniteLoop( sendFrom, m_fxChannels[sendTo]->m_sends[i] ) )
		{
			return true;
		}
	}

	return false;
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
	// SMF: it seems like here the track-channels are mixed in... but from where
	//			is this called and when and why...?!?
	//
	//			OK, found it (git grep is your friend...): This is the next part,
	//			where there is a mix between push and pull model inside the core, as
	//			the audio-tracks *push* their data into the fx-channels hopefully just
	//			before the Mixer-Channels are processed... Sorry to say this: but this
	//			took me senseless hours to find out and is silly, too...

	if( m_fxChannels[_ch]->m_muteModel.value() == false )
	{
		m_fxChannels[_ch]->m_lock.lock();
		MixHelpers::add( m_fxChannels[_ch]->m_buffer, _buf, engine::mixer()->framesPerPeriod() );
		m_fxChannels[_ch]->m_hasInput = true;
		m_fxChannels[_ch]->m_lock.unlock();
	}
}




void FxMixer::prepareMasterMix()
{
	engine::mixer()->clearAudioBuffer( m_fxChannels[0]->m_buffer,
					engine::mixer()->framesPerPeriod() );
}



void FxMixer::addChannelLeaf( int _ch, sampleFrame * _buf )
{
	FxChannel * thisCh = m_fxChannels[_ch];

	// if we're muted or this channel is seen already, discount it
	if( thisCh->m_queued )
	{
		return;
	}

	foreach( const int senderIndex, thisCh->m_receives )
	{
		addChannelLeaf( senderIndex, _buf );
	}

	// add this channel to job list
	thisCh->m_queued = true;
	MixerWorkerThread::addJob( thisCh );
}



void FxMixer::masterMix( sampleFrame * _buf )
{
	const int fpp = engine::mixer()->framesPerPeriod();

	// recursively loop through channel dependency chain
	// and add all channels to job list that have no dependencies
	// when the channel completes it will check its parent to see if it needs
	// to be processed.
	m_sendsMutex.lock();
	MixerWorkerThread::resetJobQueue( MixerWorkerThread::JobQueue::Dynamic );
	addChannelLeaf( 0, _buf );
	while( m_fxChannels[0]->state() != ThreadableJob::Done )
	{
		MixerWorkerThread::startAndWaitForJobs();
	}
	//m_fxChannels[0]->doProcessing( NULL );
	m_sendsMutex.unlock();

	const float v = m_fxChannels[0]->m_volumeModel.value();
	MixHelpers::addMultiplied( _buf, m_fxChannels[0]->m_buffer, v, fpp );

	m_fxChannels[0]->m_peakLeft *= engine::mixer()->masterGain();
	m_fxChannels[0]->m_peakRight *= engine::mixer()->masterGain();

	// clear all channel buffers and
	// reset channel process state
	for( int i = 0; i < numChannels(); ++i)
	{
		engine::mixer()->clearAudioBuffer( m_fxChannels[i]->m_buffer, engine::mixer()->framesPerPeriod() );
		m_fxChannels[i]->reset();
		m_fxChannels[i]->m_queued = false;
		// also reset hasInput
		m_fxChannels[i]->m_hasInput = false;
	}
}




void FxMixer::clear()
{
	while( m_fxChannels.size() > 1 )
	{
		deleteChannel(1);
	}

	clearChannel(0);
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
		while( ! ch->m_sends.isEmpty() )
		{
			deleteChannelSend( index, ch->m_sends.first() );
		}

		// add send to master
		createChannelSend(index, 0);
	}

	// delete receives
	while( ! ch->m_receives.isEmpty() )
	{
		deleteChannelSend( ch->m_receives.first(), index );
	}
}

void FxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i < m_fxChannels.size(); ++i )
	{
		FxChannel * ch = m_fxChannels[i];

		QDomElement fxch = _doc.createElement( QString( "fxchannel" ) );
		_this.appendChild( fxch );

		ch->m_fxChain.saveState( _doc, fxch );
		ch->m_volumeModel.saveSettings( _doc, fxch, "volume" );
		ch->m_muteModel.saveSettings( _doc, fxch, "muted" );
		fxch.setAttribute( "num", i );
		fxch.setAttribute( "name", ch->m_name );

		// add the channel sends
		for( int si = 0; si < ch->m_sends.size(); ++si )
		{
			QDomElement sendsDom = _doc.createElement( QString( "send" ) );
			fxch.appendChild( sendsDom );

			sendsDom.setAttribute( "channel", ch->m_sends[si] );
			ch->m_sendAmount[si]->saveSettings( _doc, sendsDom, "amount");
		}
	}
}

// make sure we have at least num channels
void FxMixer::allocateChannelsTo(int num)
{
	while( num > m_fxChannels.size() - 1 )
	{
		createChannel();

		// delete the default send to master
		deleteChannelSend(m_fxChannels.size()-1, 0);
	}
}


void FxMixer::loadSettings( const QDomElement & _this )
{
	clear();
	QDomNode node = _this.firstChild();
	bool thereIsASend = false;

	while( ! node.isNull() )
	{
		QDomElement fxch = node.toElement();

		// index of the channel we are about to load
		int num = fxch.attribute( "num" ).toInt();

		// allocate enough channels
		allocateChannelsTo( num );

		m_fxChannels[num]->m_volumeModel.loadSettings( fxch, "volume" );
		m_fxChannels[num]->m_muteModel.loadSettings( fxch, "muted" );
		m_fxChannels[num]->m_name = fxch.attribute( "name" );

		m_fxChannels[num]->m_fxChain.restoreState( fxch.firstChildElement(
			m_fxChannels[num]->m_fxChain.nodeName() ) );

		// mixer sends
		QDomNodeList chData = fxch.childNodes();
		for( unsigned int i=0; i<chData.length(); ++i )
		{
			QDomElement chDataItem = chData.at(i).toElement();
			if( chDataItem.nodeName() == QString( "send" ) )
			{
				thereIsASend = true;
				int sendTo = chDataItem.attribute( "channel" ).toInt();
				allocateChannelsTo( sendTo) ;
				float amount = chDataItem.attribute( "amount" ).toFloat();
				createChannelSend( num, sendTo, amount );
			}
		}



		node = node.nextSibling();
	}

	// check for old format. 65 fx channels and no explicit sends.
	if( ! thereIsASend && m_fxChannels.size() == 65 ) {
		// create a send from every channel into master
		for( int i=1; i<m_fxChannels.size(); ++i )
		{
			createChannelSend(i, 0);
		}
	}

	emit dataChanged();
}

