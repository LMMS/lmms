/*
 * Mixer.cpp - effect mixer for LMMS
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDomElement>

#include "AudioEngine.h"
#include "AudioEngineWorkerThread.h"
#include "BufferManager.h"
#include "Mixer.h"
#include "MixHelpers.h"
#include "Song.h"

#include "InstrumentTrack.h"
#include "PatternStore.h"
#include "SampleTrack.h"
#include "TrackContainer.h" // For TrackContainer::TrackList typedef

namespace lmms
{


MixerRoute::MixerRoute( MixerChannel * from, MixerChannel * to, float amount ) :
	m_from( from ),
	m_to( to ),
	m_amount( amount, 0, 1, 0.001, nullptr,
			tr( "Amount to send from channel %1 to channel %2" ).arg( m_from->m_channelIndex ).arg( m_to->m_channelIndex ) )
{
	//qDebug( "created: %d to %d", m_from->m_channelIndex, m_to->m_channelIndex );
	// create send amount model
}


void MixerRoute::updateName()
{
	m_amount.setDisplayName(
			tr( "Amount to send from channel %1 to channel %2" ).arg( m_from->m_channelIndex ).arg( m_to->m_channelIndex ) );
}


MixerChannel::MixerChannel( int idx, Model * _parent ) :
	m_fxChain( nullptr ),
	m_hasInput( false ),
	m_stillRunning( false ),
	m_peakLeft( 0.0f ),
	m_peakRight( 0.0f ),
	m_buffer( new sampleFrame[Engine::audioEngine()->framesPerPeriod()] ),
	m_muteModel( false, _parent ),
	m_soloModel( false, _parent ),
	m_volumeModel( 1.0, 0.0, 2.0, 0.001, _parent ),
	m_name(),
	m_lock(),
	m_channelIndex( idx ),
	m_queued( false ),
	m_hasColor( false ),
	m_dependenciesMet(0)
{
	BufferManager::clear( m_buffer, Engine::audioEngine()->framesPerPeriod() );
}




MixerChannel::~MixerChannel()
{
	delete[] m_buffer;
}


inline void MixerChannel::processed()
{
	for( const MixerRoute * receiverRoute : m_sends )
	{
		if( receiverRoute->receiver()->m_muted == false )
		{
			receiverRoute->receiver()->incrementDeps();
		}
	}
}

void MixerChannel::incrementDeps()
{
	int i = m_dependenciesMet++ + 1;
	if( i >= m_receives.size() && ! m_queued )
	{
		m_queued = true;
		AudioEngineWorkerThread::addJob( this );
	}
}

void MixerChannel::unmuteForSolo()
{
	//TODO: Recursively activate every channel, this channel sends to
	m_muteModel.setValue(false);
}



void MixerChannel::doProcessing()
{
	const fpp_t fpp = Engine::audioEngine()->framesPerPeriod();

	if( m_muted == false )
	{
		for( MixerRoute * senderRoute : m_receives )
		{
			MixerChannel * sender = senderRoute->sender();
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
					MixHelpers::addSanitizedMultiplied( m_buffer, ch_buf, v, fpp );
				}
				else if( volBuf && sendBuf ) // both volume and send have sample-exact data
				{
					MixHelpers::addSanitizedMultipliedByBuffers( m_buffer, ch_buf, volBuf, sendBuf, fpp );
				}
				else if( volBuf ) // volume has sample-exact data but send does not
				{
					const float v = sendModel->value();
					MixHelpers::addSanitizedMultipliedByBuffer( m_buffer, ch_buf, v, volBuf, fpp );
				}
				else // vice versa
				{
					const float v = sender->m_volumeModel.value();
					MixHelpers::addSanitizedMultipliedByBuffer( m_buffer, ch_buf, v, sendBuf, fpp );
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

		AudioEngine::StereoSample peakSamples = Engine::audioEngine()->getPeakValues(m_buffer, fpp);
		m_peakLeft = qMax( m_peakLeft, peakSamples.left * v );
		m_peakRight = qMax( m_peakRight, peakSamples.right * v );
	}
	else
	{
		m_peakLeft = m_peakRight = 0.0f;
	}

	// increment dependency counter of all receivers
	processed();
}



Mixer::Mixer() :
	Model( nullptr ),
	JournallingObject(),
	m_mixerChannels()
{
	// create master channel
	createChannel();
	m_lastSoloed = -1;
}



Mixer::~Mixer()
{
	while( ! m_mixerRoutes.isEmpty() )
	{
		deleteChannelSend( m_mixerRoutes.first() );
	}
	while( m_mixerChannels.size() )
	{
		MixerChannel * f = m_mixerChannels[m_mixerChannels.size() - 1];
		m_mixerChannels.pop_back();
		delete f;
	}
}



int Mixer::createChannel()
{
	const int index = m_mixerChannels.size();
	// create new channel
	m_mixerChannels.push_back( new MixerChannel( index, this ) );

	// reset channel state
	clearChannel( index );

	return index;
}

void Mixer::activateSolo()
{
	for (int i = 1; i < m_mixerChannels.size(); ++i)
	{
		m_mixerChannels[i]->m_muteBeforeSolo = m_mixerChannels[i]->m_muteModel.value();
		m_mixerChannels[i]->m_muteModel.setValue( true );
	}
}

void Mixer::deactivateSolo()
{
	for (int i = 1; i < m_mixerChannels.size(); ++i)
	{
		m_mixerChannels[i]->m_muteModel.setValue( m_mixerChannels[i]->m_muteBeforeSolo );
	}
}

void Mixer::toggledSolo()
{
	int soloedChan = -1;
	bool resetSolo = m_lastSoloed != -1;
	//untoggle if lastsoloed is entered
	if (resetSolo)
	{
		m_mixerChannels[m_lastSoloed]->m_soloModel.setValue( false );
	}
	//determine the soloed channel
	for (int i = 0; i < m_mixerChannels.size(); ++i)
	{
		if (m_mixerChannels[i]->m_soloModel.value() == true)
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
		m_mixerChannels[soloedChan]->unmuteForSolo();
	} else {
		deactivateSolo();
	}
	m_lastSoloed = soloedChan;
}



void Mixer::deleteChannel( int index )
{
	// channel deletion is performed between mixer rounds
	Engine::audioEngine()->requestChangeInModel();

	// go through every instrument and adjust for the channel index change
	TrackContainer::TrackList tracks;
	tracks += Engine::getSong()->tracks();
	tracks += Engine::patternStore()->tracks();

	for( Track* t : tracks )
	{
		if( t->type() == Track::InstrumentTrack )
		{
			auto inst = dynamic_cast<InstrumentTrack*>(t);
			int val = inst->mixerChannelModel()->value(0);
			if( val == index )
			{
				// we are deleting this track's channel send
				// send to master
				inst->mixerChannelModel()->setValue(0);
			}
			else if( val > index )
			{
				// subtract 1 to make up for the missing channel
				inst->mixerChannelModel()->setValue(val-1);
			}
		}
		else if( t->type() == Track::SampleTrack )
		{
			auto strk = dynamic_cast<SampleTrack*>(t);
			int val = strk->mixerChannelModel()->value(0);
			if( val == index )
			{
				// we are deleting this track's channel send
				// send to master
				strk->mixerChannelModel()->setValue(0);
			}
			else if( val > index )
			{
				// subtract 1 to make up for the missing channel
				strk->mixerChannelModel()->setValue(val-1);
			}
		}
	}

	MixerChannel * ch = m_mixerChannels[index];

	// delete all of this channel's sends and receives
	while( ! ch->m_sends.isEmpty() )
	{
		deleteChannelSend( ch->m_sends.first() );
	}
	while( ! ch->m_receives.isEmpty() )
	{
		deleteChannelSend( ch->m_receives.first() );
	}

	// if m_lastSoloed was our index, reset it
	if (m_lastSoloed == index) { m_lastSoloed = -1; }
	// if m_lastSoloed is > delete index, it will move left
	else if (m_lastSoloed > index) { --m_lastSoloed; }

	// actually delete the channel
	m_mixerChannels.remove(index);
	delete ch;

	for( int i = index; i < m_mixerChannels.size(); ++i )
	{
		validateChannelName( i, i + 1 );

		// set correct channel index
		m_mixerChannels[i]->m_channelIndex = i;

		// now check all routes and update names of the send models
		for( MixerRoute * r : m_mixerChannels[i]->m_sends )
		{
			r->updateName();
		}
		for( MixerRoute * r : m_mixerChannels[i]->m_receives )
		{
			r->updateName();
		}
	}

	Engine::audioEngine()->doneChangeInModel();
}



void Mixer::moveChannelLeft( int index )
{
	// can't move master or first channel
	if( index <= 1 || index >= m_mixerChannels.size() )
	{
		return;
	}
	// channels to swap
	int a = index - 1, b = index;

	// check if m_lastSoloed is one of our swaps
	if (m_lastSoloed == a) { m_lastSoloed = b; }
	else if (m_lastSoloed == b) { m_lastSoloed = a; }

	// go through every instrument and adjust for the channel index change
	TrackContainer::TrackList songTrackList = Engine::getSong()->tracks();
	TrackContainer::TrackList patternTrackList = Engine::patternStore()->tracks();

	for (const auto& trackList : {songTrackList, patternTrackList})
	{
		for (const auto& track : trackList)
		{
			if (track->type() == Track::InstrumentTrack)
			{
				auto inst = (InstrumentTrack*)track;
				int val = inst->mixerChannelModel()->value(0);
				if( val == a )
				{
					inst->mixerChannelModel()->setValue(b);
				}
				else if( val == b )
				{
					inst->mixerChannelModel()->setValue(a);
				}
			}
			else if (track->type() == Track::SampleTrack)
			{
				auto strk = (SampleTrack*)track;
				int val = strk->mixerChannelModel()->value(0);
				if( val == a )
				{
					strk->mixerChannelModel()->setValue(b);
				}
				else if( val == b )
				{
					strk->mixerChannelModel()->setValue(a);
				}
			}
		}
	}

	// Swap positions in array
	qSwap(m_mixerChannels[index], m_mixerChannels[index - 1]);

	// Update m_channelIndex of both channels
	m_mixerChannels[index]->m_channelIndex = index;
	m_mixerChannels[index - 1]->m_channelIndex = index -1;
}



void Mixer::moveChannelRight( int index )
{
	moveChannelLeft( index + 1 );
}



MixerRoute * Mixer::createChannelSend( mix_ch_t fromChannel, mix_ch_t toChannel,
								float amount )
{
//	qDebug( "requested: %d to %d", fromChannel, toChannel );
	// find the existing connection
	MixerChannel * from = m_mixerChannels[fromChannel];
	MixerChannel * to = m_mixerChannels[toChannel];

	for (const auto& send : from->m_sends)
	{
		if (send->receiver() == to)
		{
			// simply adjust the amount
			send->amount()->setValue(amount);
			return send;
		}
	}

	// connection does not exist. create a new one
	return createRoute( from, to, amount );
}


MixerRoute * Mixer::createRoute( MixerChannel * from, MixerChannel * to, float amount )
{
	if( from == to )
	{
		return nullptr;
	}
	Engine::audioEngine()->requestChangeInModel();
	auto route = new MixerRoute(from, to, amount);

	// add us to from's sends
	from->m_sends.append( route );

	// add us to to's receives
	to->m_receives.append( route );

	// add us to mixer's list
	Engine::mixer()->m_mixerRoutes.append( route );
	Engine::audioEngine()->doneChangeInModel();

	return route;
}


// delete the connection made by createChannelSend
void Mixer::deleteChannelSend( mix_ch_t fromChannel, mix_ch_t toChannel )
{
	// delete the send
	MixerChannel * from = m_mixerChannels[fromChannel];
	MixerChannel * to	 = m_mixerChannels[toChannel];

	// find and delete the send entry
	for (const auto& send : from->m_sends)
	{
		if (send->receiver() == to)
		{
			deleteChannelSend(send);
			break;
		}
	}
}


void Mixer::deleteChannelSend( MixerRoute * route )
{
	Engine::audioEngine()->requestChangeInModel();
	// remove us from from's sends
	route->sender()->m_sends.remove( route->sender()->m_sends.indexOf( route ) );
	// remove us from to's receives
	route->receiver()->m_receives.remove( route->receiver()->m_receives.indexOf( route ) );
	// remove us from mixer's list
	Engine::mixer()->m_mixerRoutes.remove( Engine::mixer()->m_mixerRoutes.indexOf( route ) );
	delete route;
	Engine::audioEngine()->doneChangeInModel();
}


bool Mixer::isInfiniteLoop( mix_ch_t sendFrom, mix_ch_t sendTo )
{
	if( sendFrom == sendTo ) return true;
	MixerChannel * from = m_mixerChannels[sendFrom];
	MixerChannel * to = m_mixerChannels[sendTo];
	bool b = checkInfiniteLoop( from, to );
	return b;
}


bool Mixer::checkInfiniteLoop( MixerChannel * from, MixerChannel * to )
{
	// can't send master to anything
	if( from == m_mixerChannels[0] )
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
	for (const auto& send : to->m_sends)
	{
		if (checkInfiniteLoop(from, send->receiver()))
		{
			return true;
		}
	}

	return false;
}


// how much does fromChannel send its output to the input of toChannel?
FloatModel * Mixer::channelSendModel( mix_ch_t fromChannel, mix_ch_t toChannel )
{
	if( fromChannel == toChannel )
	{
		return nullptr;
	}
	const MixerChannel * from = m_mixerChannels[fromChannel];
	const MixerChannel * to = m_mixerChannels[toChannel];

	for( MixerRoute * route : from->m_sends )
	{
		if( route->receiver() == to )
		{
			return route->amount();
		}
	}

	return nullptr;
}



void Mixer::mixToChannel( const sampleFrame * _buf, mix_ch_t _ch )
{
	if( m_mixerChannels[_ch]->m_muteModel.value() == false )
	{
		m_mixerChannels[_ch]->m_lock.lock();
		MixHelpers::add( m_mixerChannels[_ch]->m_buffer, _buf, Engine::audioEngine()->framesPerPeriod() );
		m_mixerChannels[_ch]->m_hasInput = true;
		m_mixerChannels[_ch]->m_lock.unlock();
	}
}




void Mixer::prepareMasterMix()
{
	BufferManager::clear( m_mixerChannels[0]->m_buffer,
					Engine::audioEngine()->framesPerPeriod() );
}



void Mixer::masterMix( sampleFrame * _buf )
{
	const int fpp = Engine::audioEngine()->framesPerPeriod();

	// add the channels that have no dependencies (no incoming senders, ie.
	// no receives) to the jobqueue. The channels that have receives get
	// added when their senders get processed, which is detected by
	// dependency counting.
	// also instantly add all muted channels as they don't need to care
	// about their senders, and can just increment the deps of their
	// recipients right away.
	AudioEngineWorkerThread::resetJobQueue( AudioEngineWorkerThread::JobQueue::Dynamic );
	for( MixerChannel * ch : m_mixerChannels )
	{
		ch->m_muted = ch->m_muteModel.value();
		if( ch->m_muted ) // instantly "process" muted channels
		{
			ch->processed();
			ch->done();
		}
		else if( ch->m_receives.size() == 0 )
		{
			ch->m_queued = true;
			AudioEngineWorkerThread::addJob( ch );
		}
	}
	while (m_mixerChannels[0]->state() != ThreadableJob::ProcessingState::Done)
	{
		bool found = false;
		for( MixerChannel * ch : m_mixerChannels )
		{
			const auto s = ch->state();
			if (s == ThreadableJob::ProcessingState::Queued
				|| s == ThreadableJob::ProcessingState::InProgress)
			{
				found = true;
				break;
			}
		}
		if( !found )
		{
			break;
		}
		AudioEngineWorkerThread::startAndWaitForJobs();
	}

	// handle sample-exact data in master volume fader
	ValueBuffer * volBuf = m_mixerChannels[0]->m_volumeModel.valueBuffer();

	if( volBuf )
	{
		for( int f = 0; f < fpp; f++ )
		{
			m_mixerChannels[0]->m_buffer[f][0] *= volBuf->values()[f];
			m_mixerChannels[0]->m_buffer[f][1] *= volBuf->values()[f];
		}
	}

	const float v = volBuf
		? 1.0f
		: m_mixerChannels[0]->m_volumeModel.value();
	MixHelpers::addSanitizedMultiplied( _buf, m_mixerChannels[0]->m_buffer, v, fpp );

	// clear all channel buffers and
	// reset channel process state
	for( int i = 0; i < numChannels(); ++i)
	{
		BufferManager::clear( m_mixerChannels[i]->m_buffer,
				Engine::audioEngine()->framesPerPeriod() );
		m_mixerChannels[i]->reset();
		m_mixerChannels[i]->m_queued = false;
		// also reset hasInput
		m_mixerChannels[i]->m_hasInput = false;
		m_mixerChannels[i]->m_dependenciesMet = 0;
	}
}




void Mixer::clear()
{
	while( m_mixerChannels.size() > 1 )
	{
		deleteChannel(1);
	}

	clearChannel(0);
}



void Mixer::clearChannel(mix_ch_t index)
{
	MixerChannel * ch = m_mixerChannels[index];
	ch->m_fxChain.clear();
	ch->m_volumeModel.setValue( 1.0f );
	ch->m_muteModel.setValue( false );
	ch->m_soloModel.setValue( false );
	ch->m_name = ( index == 0 ) ? tr( "Master" ) : tr( "Channel %1" ).arg( index );
	ch->m_volumeModel.setDisplayName( ch->m_name + ">" + tr( "Volume" ) );
	ch->m_muteModel.setDisplayName( ch->m_name + ">" + tr( "Mute" ) );
	ch->m_soloModel.setDisplayName( ch->m_name + ">" + tr( "Solo" ) );

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

void Mixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	// save channels
	for( int i = 0; i < m_mixerChannels.size(); ++i )
	{
		MixerChannel * ch = m_mixerChannels[i];

		QDomElement mixch = _doc.createElement( QString( "mixerchannel" ) );
		_this.appendChild( mixch );

		ch->m_fxChain.saveState( _doc, mixch );
		ch->m_volumeModel.saveSettings( _doc, mixch, "volume" );
		ch->m_muteModel.saveSettings( _doc, mixch, "muted" );
		ch->m_soloModel.saveSettings( _doc, mixch, "soloed" );
		mixch.setAttribute( "num", i );
		mixch.setAttribute( "name", ch->m_name );
		if( ch->m_hasColor ) mixch.setAttribute( "color", ch->m_color.name() );

		// add the channel sends
		for (const auto& send : ch->m_sends)
		{
			QDomElement sendsDom = _doc.createElement( QString( "send" ) );
			mixch.appendChild( sendsDom );

			sendsDom.setAttribute("channel", send->receiverIndex());
			send->amount()->saveSettings(_doc, sendsDom, "amount");
		}
	}
}

// make sure we have at least num channels
void Mixer::allocateChannelsTo(int num)
{
	while( num > m_mixerChannels.size() - 1 )
	{
		createChannel();

		// delete the default send to master
		deleteChannelSend( m_mixerChannels.size()-1, 0 );
	}
}


void Mixer::loadSettings( const QDomElement & _this )
{
	clear();
	QDomNode node = _this.firstChild();

	while( ! node.isNull() )
	{
		QDomElement mixch = node.toElement();

		// index of the channel we are about to load
		int num = mixch.attribute( "num" ).toInt();

		// allocate enough channels
		allocateChannelsTo( num );

		m_mixerChannels[num]->m_volumeModel.loadSettings( mixch, "volume" );
		m_mixerChannels[num]->m_muteModel.loadSettings( mixch, "muted" );
		m_mixerChannels[num]->m_soloModel.loadSettings( mixch, "soloed" );
		m_mixerChannels[num]->m_name = mixch.attribute( "name" );
		if( mixch.hasAttribute( "color" ) )
		{
			m_mixerChannels[num]->m_hasColor = true;
			m_mixerChannels[num]->m_color.setNamedColor( mixch.attribute( "color" ) );
		}

		m_mixerChannels[num]->m_fxChain.restoreState( mixch.firstChildElement(
			m_mixerChannels[num]->m_fxChain.nodeName() ) );

		// mixer sends
		QDomNodeList chData = mixch.childNodes();
		for( unsigned int i=0; i<chData.length(); ++i )
		{
			QDomElement chDataItem = chData.at(i).toElement();
			if( chDataItem.nodeName() == QString( "send" ) )
			{
				int sendTo = chDataItem.attribute( "channel" ).toInt();
				allocateChannelsTo( sendTo ) ;
				MixerRoute * mxr = createChannelSend( num, sendTo, 1.0f );
				if( mxr ) mxr->amount()->loadSettings( chDataItem, "amount" );
			}
		}



		node = node.nextSibling();
	}

	emit dataChanged();
}


void Mixer::validateChannelName( int index, int oldIndex )
{
	if( m_mixerChannels[index]->m_name == tr( "Channel %1" ).arg( oldIndex ) )
	{
		m_mixerChannels[index]->m_name = tr( "Channel %1" ).arg( index );
	}
}


} // namespace lmms
