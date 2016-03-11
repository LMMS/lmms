/*
 * FxMixer.cpp - effect mixer for LMMS
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QDomElement>

#include "BufferManager.h"
#include "FxMixer.h"
#include "MixerWorkerThread.h"
#include "MixHelpers.h"
#include "Song.h"

#include "InstrumentTrack.h"
#include "BBTrackContainer.h"
#include "ValueBuffer.h"

FxRoute::FxRoute( FxChannel * from, FxChannel * to, float amount ) :
	m_from( from ),
	m_to( to ),
	m_amount( amount, 0, 1, 0.001, NULL,
			tr( "Amount to send from channel %1 to channel %2" ).arg( m_from->m_channelIndex ).arg( m_to->m_channelIndex ) )
{
	//qDebug( "created: %d to %d", m_from->m_channelIndex, m_to->m_channelIndex );
	// create send amount model
}


FxRoute::~FxRoute()
{
}


void FxRoute::updateName()
{
	m_amount.setDisplayName(
			tr( "Amount to send from channel %1 to channel %2" ).arg( m_from->m_channelIndex ).arg( m_to->m_channelIndex ) );
}


FxChannel::FxChannel( int idx, Model * _parent ) :
	m_fxChain( NULL ),
	m_hasInput( false ),
	m_stillRunning( false ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( new sampleFrame[Engine::mixer()->framesPerPeriod()] ),
	m_muteModel( false, _parent ),
	m_soloModel( false, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.001, _parent ),
	m_name(),
	m_lock(),
	m_channelIndex( idx ),
	m_queued( false ),
	m_dependenciesMet( 0 )
{
	BufferManager::clear( m_buffer, Engine::mixer()->framesPerPeriod() );
}




FxChannel::~FxChannel()
{
	delete[] m_buffer;
}


inline void FxChannel::processed()
{
	foreach( FxRoute * receiverRoute, m_sends )
	{
		if( receiverRoute->receiver()->m_muted == false )
		{
			receiverRoute->receiver()->incrementDeps();
		}
	}
}

void FxChannel::incrementDeps()
{
	int i = m_dependenciesMet.fetchAndAddOrdered( 1 ) + 1;
	if( i >= m_receives.size() && ! m_queued )
	{
		m_queued = true;
		MixerWorkerThread::addJob( this );
	}
}

void FxChannel::unmuteForSolo()
{
	//TODO: Recursively activate every channel, this channel sends to
	m_muteModel.setValue(false);
}



void FxChannel::doProcessing()
{
	const fpp_t fpp = Engine::mixer()->framesPerPeriod();
	const bool exporting = Engine::getSong()->isExporting();

	if( m_muted == false )
	{
		foreach( FxRoute * senderRoute, m_receives )
		{
			FxChannel * sender = senderRoute->sender();
			FloatModel * sendModel = senderRoute->amount();
			if( ! sendModel ) qFatal( "Error: no send model found from %d to %d", senderRoute->senderIndex(), m_channelIndex );

			if( sender->m_hasInput || sender->m_stillRunning )
			{
				// figure out if we're getting sample-exact input
				ValueBuffer * sendBuf = sendModel->valueBuffer();
				ValueBuffer * volBuf = sender->m_volumeModel.valueBuffer();

				// mix it's output with this one's output
				sampleFrame * ch_buf = sender->m_buffer;

				// use sample-exact mixing if sample-exact values are available
				if( ! volBuf && ! sendBuf ) // neither volume nor send has sample-exact data...
				{
					const float v = sender->m_volumeModel.value() * sendModel->value();
					if( exporting ) { MixHelpers::addSanitizedMultiplied( m_buffer, ch_buf, v, fpp ); }
					else { MixHelpers::addMultiplied( m_buffer, ch_buf, v, fpp ); }
				}
				else if( volBuf && sendBuf ) // both volume and send have sample-exact data
				{
					if( exporting ) { MixHelpers::addSanitizedMultipliedByBuffers( m_buffer, ch_buf, volBuf, sendBuf, fpp ); }
					else { MixHelpers::addMultipliedByBuffers( m_buffer, ch_buf, volBuf, sendBuf, fpp ); }
				}
				else if( volBuf ) // volume has sample-exact data but send does not
				{
					const float v = sendModel->value();
					if( exporting ) { MixHelpers::addSanitizedMultipliedByBuffer( m_buffer, ch_buf, v, volBuf, fpp ); }
					else { MixHelpers::addMultipliedByBuffer( m_buffer, ch_buf, v, volBuf, fpp ); }
				}
				else // vice versa
				{
					const float v = sender->m_volumeModel.value();
					if( exporting ) { MixHelpers::addSanitizedMultipliedByBuffer( m_buffer, ch_buf, v, sendBuf, fpp ); }
					else { MixHelpers::addMultipliedByBuffer( m_buffer, ch_buf, v, sendBuf, fpp ); }
				}
				m_hasInput = true;
			}
		}


		const float v = m_volumeModel.value();

		if( m_hasInput )
		{
			// only start fxchain when we have input...
			m_fxChain.startRunning();
		}

		m_stillRunning = m_fxChain.processAudioBuffer( m_buffer, fpp, m_hasInput );

		float peakLeft = 0.;
		float peakRight = 0.;
		Engine::mixer()->getPeakValues( m_buffer, fpp, peakLeft, peakRight );
		m_peakLeft = qMax( m_peakLeft, peakLeft * v );
		m_peakRight = qMax( m_peakRight, peakRight * v );
	}
	else
	{
		m_peakLeft = m_peakRight = 0.0f;
	}

	// increment dependency counter of all receivers
	processed();
}



FxMixer::FxMixer() :
	Model( NULL ),
	JournallingObject(),
	m_fxChannels()
{
	// create master channel
	createChannel();
	m_lastSoloed = -1;
}



FxMixer::~FxMixer()
{
	while( ! m_fxRoutes.isEmpty() )
	{
		deleteChannelSend( m_fxRoutes.first() );
	}
	for( int i = 0; i < m_fxChannels.size(); ++i )
	{
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

void FxMixer::activateSolo()
{
	for (int i = 1; i < m_fxChannels.size(); ++i)
	{
		m_fxChannels[i]->m_muteBeforeSolo = m_fxChannels[i]->m_muteModel.value();
		m_fxChannels[i]->m_muteModel.setValue( true );
	}
}

void FxMixer::deactivateSolo()
{
	for (int i = 1; i < m_fxChannels.size(); ++i)
	{
		m_fxChannels[i]->m_muteModel.setValue( m_fxChannels[i]->m_muteBeforeSolo );
	}
}

void FxMixer::toggledSolo()
{
	int soloedChan = -1;
	bool resetSolo = m_lastSoloed != -1;
	//untoggle if lastsoloed is entered
	if (resetSolo)
	{
		m_fxChannels[m_lastSoloed]->m_soloModel.setValue( false );
	}
	//determine the soloed channel
	for (int i = 0; i < m_fxChannels.size(); ++i)
	{
		if (m_fxChannels[i]->m_soloModel.value() == true)
			soloedChan = i;
	}
	// if no channel is soloed, unmute everything, else mute everything
	if (soloedChan != -1)
	{
		if (resetSolo)
		{
			deactivateSolo();
			activateSolo();
		} else {
			activateSolo();
		}
		// unmute the soloed chan and every channel it sends to
		m_fxChannels[soloedChan]->unmuteForSolo();
	} else {
		deactivateSolo();
	}
	m_lastSoloed = soloedChan;
}



void FxMixer::deleteChannel( int index )
{
	m_fxChannels[index]->m_lock.lock();

	FxChannel * ch = m_fxChannels[index];

	// go through every instrument and adjust for the channel index change
	TrackContainer::TrackList tracks;
	tracks += Engine::getSong()->tracks();
	tracks += Engine::getBBTrackContainer()->tracks();

	foreach( Track* t, tracks )
	{
		if( t->type() == Track::InstrumentTrack )
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
	while( ! ch->m_sends.isEmpty() )
	{
		deleteChannelSend( ch->m_sends.first() );
	}
	while( ! ch->m_receives.isEmpty() )
	{
		deleteChannelSend( ch->m_receives.first() );
	}

	// actually delete the channel
	delete m_fxChannels[index];
	m_fxChannels.remove(index);

	for( int i = index; i < m_fxChannels.size(); ++i )
	{
		validateChannelName( i, i + 1 );

		// set correct channel index
		m_fxChannels[i]->m_channelIndex = i;

		// now check all routes and update names of the send models
		foreach( FxRoute * r, m_fxChannels[i]->m_sends )
		{
			r->updateName();
		}
		foreach( FxRoute * r, m_fxChannels[i]->m_receives )
		{
			r->updateName();
		}
	}
}



void FxMixer::moveChannelLeft( int index )
{
	// can't move master or first channel
	if( index <= 1 || index >= m_fxChannels.size() )
	{
		return;
	}
	// channels to swap
	int a = index - 1, b = index;

	// go through every instrument and adjust for the channel index change
	QVector<Track *> songTrackList = Engine::getSong()->tracks();
	QVector<Track *> bbTrackList = Engine::getBBTrackContainer()->tracks();

	QVector<Track *> trackLists[] = {songTrackList, bbTrackList};
	for(int tl=0; tl<2; ++tl)
	{
		QVector<Track *> trackList = trackLists[tl];
		for(int i=0; i<trackList.size(); ++i)
		{
			if( trackList[i]->type() == Track::InstrumentTrack )
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

	// Swap positions in array
	qSwap(m_fxChannels[index], m_fxChannels[index - 1]);

	// Update m_channelIndex of both channels
	m_fxChannels[index]->m_channelIndex = index;
	m_fxChannels[index - 1]->m_channelIndex = index -1;
}



void FxMixer::moveChannelRight( int index )
{
	moveChannelLeft( index + 1 );
}



FxRoute * FxMixer::createChannelSend( fx_ch_t fromChannel, fx_ch_t toChannel,
								float amount )
{
//	qDebug( "requested: %d to %d", fromChannel, toChannel );
	// find the existing connection
	FxChannel * from = m_fxChannels[fromChannel];
	FxChannel * to = m_fxChannels[toChannel];

	for( int i=0; i<from->m_sends.size(); ++i )
	{
		if( from->m_sends[i]->receiver() == to )
		{
			// simply adjust the amount
			from->m_sends[i]->amount()->setValue( amount );
			return from->m_sends[i];
		}
	}

	// connection does not exist. create a new one
	return createRoute( from, to, amount );
}


FxRoute * FxMixer::createRoute( FxChannel * from, FxChannel * to, float amount )
{
	if( from == to )
	{
		return NULL;
	}
	m_sendsMutex.lock();
	FxRoute * route = new FxRoute( from, to, amount );

	// add us to from's sends
	from->m_sends.append( route );

	// add us to to's receives
	to->m_receives.append( route );

	// add us to fxmixer's list
	Engine::fxMixer()->m_fxRoutes.append( route );
	m_sendsMutex.unlock();

	return route;
}


// delete the connection made by createChannelSend
void FxMixer::deleteChannelSend( fx_ch_t fromChannel, fx_ch_t toChannel )
{
	// delete the send
	FxChannel * from = m_fxChannels[fromChannel];
	FxChannel * to	 = m_fxChannels[toChannel];

	// find and delete the send entry
	for( int i = 0; i < from->m_sends.size(); ++i )
	{
		if( from->m_sends[i]->receiver() == to )
		{
			deleteChannelSend( from->m_sends[i] );
			break;
		}
	}
}


void FxMixer::deleteChannelSend( FxRoute * route )
{
	m_sendsMutex.lock();
	// remove us from from's sends
	route->sender()->m_sends.remove( route->sender()->m_sends.indexOf( route ) );
	// remove us from to's receives
	route->receiver()->m_receives.remove( route->receiver()->m_receives.indexOf( route ) );
	// remove us from fxmixer's list
	Engine::fxMixer()->m_fxRoutes.remove( Engine::fxMixer()->m_fxRoutes.indexOf( route ) );
	delete route;
	m_sendsMutex.unlock();
}


bool FxMixer::isInfiniteLoop( fx_ch_t sendFrom, fx_ch_t sendTo )
{
	if( sendFrom == sendTo ) return true;
	FxChannel * from = m_fxChannels[sendFrom];
	FxChannel * to = m_fxChannels[sendTo];
	bool b = checkInfiniteLoop( from, to );
	return b;
}


bool FxMixer::checkInfiniteLoop( FxChannel * from, FxChannel * to )
{
	// can't send master to anything
	if( from == m_fxChannels[0] )
	{
		return true;
	}

	// can't send channel to itself
	if( from == to )
	{
		return true;
	}

	// follow sendTo's outputs recursively looking for something that sends
	// to sendFrom
	for( int i=0; i < to->m_sends.size(); ++i )
	{
		if( checkInfiniteLoop( from, to->m_sends[i]->receiver() ) )
		{
			return true;
		}
	}

	return false;
}


// how much does fromChannel send its output to the input of toChannel?
FloatModel * FxMixer::channelSendModel( fx_ch_t fromChannel, fx_ch_t toChannel )
{
	if( fromChannel == toChannel )
	{
		return NULL;
	}
	const FxChannel * from = m_fxChannels[fromChannel];
	const FxChannel * to = m_fxChannels[toChannel];

	foreach( FxRoute * route, from->m_sends )
	{
		if( route->receiver() == to )
		{
			return route->amount();
		}
	}

	return NULL;
}



void FxMixer::mixToChannel( const sampleFrame * _buf, fx_ch_t _ch )
{
	// The first check is for the case where the last fxchannel was deleted but
	// there was a race condition where it had to be processed.
	if( _ch < m_fxChannels.size() &&
			m_fxChannels[_ch]->m_muteModel.value() == false )
	{
		m_fxChannels[_ch]->m_lock.lock();
		MixHelpers::add( m_fxChannels[_ch]->m_buffer, _buf, Engine::mixer()->framesPerPeriod() );
		m_fxChannels[_ch]->m_hasInput = true;
		m_fxChannels[_ch]->m_lock.unlock();
	}
}




void FxMixer::prepareMasterMix()
{
	BufferManager::clear( m_fxChannels[0]->m_buffer, 
					Engine::mixer()->framesPerPeriod() );
}



void FxMixer::masterMix( sampleFrame * _buf )
{
	const int fpp = Engine::mixer()->framesPerPeriod();

	if( m_sendsMutex.tryLock() )
	{
		// add the channels that have no dependencies (no incoming senders, ie. no receives)
		// to the jobqueue. The channels that have receives get added when their senders get processed, which
		// is detected by dependency counting.
		// also instantly add all muted channels as they don't need to care about their senders, and can just increment the deps of
		// their recipients right away.
		MixerWorkerThread::resetJobQueue( MixerWorkerThread::JobQueue::Dynamic );

		// Do not use foreach here, as foreach creates a copy of
		// m_fxChannels that causes a race condition with Qt5!
		for ( QVector<FxChannel *>::const_iterator it = m_fxChannels.constBegin(); it != m_fxChannels.constEnd(); ++it )
		{
			(*it)->m_muted = (*it)->m_muteModel.value();
			if( (*it)->m_muted ) // instantly "process" muted channels
			{
				(*it)->processed();
				(*it)->done();
			}
			else if( (*it)->m_receives.size() == 0 )
			{
				(*it)->m_queued = true;
				MixerWorkerThread::addJob( (*it) );
			}
		}
		while( m_fxChannels[0]->state() != ThreadableJob::Done )
		{
			MixerWorkerThread::startAndWaitForJobs();
		}
		m_sendsMutex.unlock();
	}

	// handle sample-exact data in master volume fader
	ValueBuffer * volBuf = m_fxChannels[0]->m_volumeModel.valueBuffer();

	if( volBuf )
	{
		for( int f = 0; f < fpp; f++ )
		{
			m_fxChannels[0]->m_buffer[f][0] *= volBuf->values()[f];
			m_fxChannels[0]->m_buffer[f][1] *= volBuf->values()[f];
		}
	}

	const float v = volBuf
		? 1.0f
		: m_fxChannels[0]->m_volumeModel.value();
	MixHelpers::addSanitizedMultiplied( _buf, m_fxChannels[0]->m_buffer, v, fpp );

	// clear all channel buffers and
	// reset channel process state
	for( int i = 0; i < numChannels(); ++i)
	{
		BufferManager::clear( m_fxChannels[i]->m_buffer, 
				Engine::mixer()->framesPerPeriod() );
		m_fxChannels[i]->reset();
		m_fxChannels[i]->m_queued = false;
		// also reset hasInput
		m_fxChannels[i]->m_hasInput = false;
		m_fxChannels[i]->m_dependenciesMet = 0;
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
	ch->m_soloModel.setValue( false );
	ch->m_name = ( index == 0 ) ? tr( "Master" ) : tr( "FX %1" ).arg( index );
	ch->m_volumeModel.setDisplayName( ch->m_name );

	// send only to master
	if( index > 0)
	{
		// delete existing sends
		while( ! ch->m_sends.isEmpty() )
		{
			deleteChannelSend( ch->m_sends.first() );
		}

		// add send to master
		createChannelSend( index, 0 );
	}

	// delete receives
	while( ! ch->m_receives.isEmpty() )
	{
		deleteChannelSend( ch->m_receives.first() );
	}
}

void FxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// save channels
	for( int i = 0; i < m_fxChannels.size(); ++i )
	{
		FxChannel * ch = m_fxChannels[i];

		QDomElement fxch = _doc.createElement( QString( "fxchannel" ) );
		_this.appendChild( fxch );

		ch->m_fxChain.saveState( _doc, fxch );
		ch->m_volumeModel.saveSettings( _doc, fxch, "volume" );
		ch->m_muteModel.saveSettings( _doc, fxch, "muted" );
		ch->m_soloModel.saveSettings( _doc, fxch, "soloed" );
		fxch.setAttribute( "num", i );
		fxch.setAttribute( "name", ch->m_name );

		// add the channel sends
		for( int si = 0; si < ch->m_sends.size(); ++si )
		{
			QDomElement sendsDom = _doc.createElement( QString( "send" ) );
			fxch.appendChild( sendsDom );

			sendsDom.setAttribute( "channel", ch->m_sends[si]->receiverIndex() );
			ch->m_sends[si]->amount()->saveSettings( _doc, sendsDom, "amount" );
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
		deleteChannelSend( m_fxChannels.size()-1, 0 );
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
		m_fxChannels[num]->m_soloModel.loadSettings( fxch, "soloed" );
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
				allocateChannelsTo( sendTo ) ;
				FxRoute * fxr = createChannelSend( num, sendTo, 1.0f );
				if( fxr ) fxr->amount()->loadSettings( chDataItem, "amount" );
			}
		}



		node = node.nextSibling();
	}

	// check for old format. 65 fx channels and no explicit sends.
	if( ! thereIsASend && m_fxChannels.size() == 65 ) {
		// create a send from every channel into master
		for( int i=1; i<m_fxChannels.size(); ++i )
		{
			createChannelSend( i, 0 );
		}
	}

	emit dataChanged();
}


void FxMixer::validateChannelName( int index, int oldIndex )
{
	if( m_fxChannels[index]->m_name == tr( "FX %1" ).arg( oldIndex ) )
	{
		m_fxChannels[index]->m_name = tr( "FX %1" ).arg( index );
	}
}

