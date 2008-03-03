#ifndef SINGLE_SOURCE_COMPILE

/*
 * mixer.cpp - audio-device-independent mixer for LMMS
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


#include <QtCore/QHash>
#include <QtCore/QWaitCondition>

#include <math.h>

#include "mixer.h"
#include "play_handle.h"
#include "song.h"
#include "templates.h"
#include "envelope_and_lfo_parameters.h"
#include "note_play_handle.h"
#include "instrument_track.h"
#include "debug.h"
#include "engine.h"
#include "config_mgr.h"
#include "audio_port.h"
#include "sample_play_handle.h"
#include "piano_roll.h"
#include "micro_timer.h"
#include "automatable_model_templates.h"

#include "audio_device.h"
#include "midi_client.h"

// platform-specific audio-interface-classes
#include "audio_alsa.h"
#include "audio_jack.h"
#include "audio_oss.h"
#include "audio_sdl.h"
#include "audio_dummy.h"

// platform-specific midi-interface-classes
#include "midi_alsa_raw.h"
#include "midi_alsa_seq.h"
#include "midi_oss.h"
#include "midi_dummy.h"



sample_rate_t SAMPLE_RATES[QUALITY_LEVELS] = { 44100, 88200 } ;


class mixerWorkerThread : public QThread
{
public:
	enum JobTypes
	{
		InvalidJob,
		PlayHandle,
		AudioPortEffects,
		EffectChannel,
		NumJobTypes
	} ;

	struct jobQueueItem
	{
		jobQueueItem() :
			workerID( -1 ),
			type( InvalidJob ),
			job( NULL ),
			done( FALSE )
		{
		}
		jobQueueItem( int _id, JobTypes _type, void * _job ) :
			workerID( _id ),
			type( _type ),
			job( _job ),
			done( FALSE )
		{
		}
		int workerID;
		JobTypes type;
		void * job;
		volatile bool done;
	} ;

	typedef QVector<jobQueueItem> jobQueueItems;
	struct jobQueue
	{
		jobQueueItems items;
		int remaining;
		QReadWriteLock lock;
	} ;

	mixerWorkerThread( mixer * _mixer, int _id ) :
		QThread( _mixer ),
		m_mixer( _mixer ),
		m_id( _id ),
		m_sem( &m_mixer->m_workerSem ),
		m_jobWait( 1 ),
		m_jobAccepted( 1 ),
		m_jobQueue( NULL ),
		m_idle( FALSE )
	{
		start( QThread::TimeCriticalPriority );
	}

	virtual ~mixerWorkerThread()
	{
	}

	void addJob( jobQueue * _q )
	{
		m_jobQueue = _q;
		m_jobWait.release();
		m_jobAccepted.acquire();
	}

	inline bool idle( void )
	{
		return( m_idle );
		
	}

private:
	virtual void run( void )
	{
		m_jobWait.acquire();
		m_jobAccepted.acquire();
		m_idle = TRUE;
		while( 1 )
		{
			m_jobWait.acquire();
			m_idle = FALSE;
			m_sem->acquire();
			m_jobAccepted.release();
			for( jobQueueItems::iterator it = m_jobQueue->items.begin();
						it != m_jobQueue->items.end(); ++it )
			{
				m_jobQueue->lock.lockForRead();
				const int id = it->workerID;
				const bool done = it->done;
				m_jobQueue->lock.unlock();
				if( !done && id == m_id )
				{
					m_jobQueue->lock.lockForWrite();
					it->done = TRUE;
					--m_jobQueue->remaining;
					m_jobQueue->lock.unlock();
					switch( it->type )
					{
						case PlayHandle:
		( (playHandle *) it->job )->play();
							break;
						case AudioPortEffects:
		{
		audioPort * a = (audioPort *) it->job;
		bool me = a->processEffects();
		if( a->m_bufferUsage != audioPort::NoUsage || me )
		{
			m_mixer->processBuffer(
					a->firstBuffer(),
					a->nextFxChannel() );
			a->nextPeriod();
		}
		}
							break;
						default:
							break;
					}
				}
			}
			m_idle = TRUE;
			m_sem->release();
		}
	}

	mixer * m_mixer;
	int m_id;
	QSemaphore * m_sem;
	QSemaphore m_jobWait;
	QSemaphore m_jobAccepted;
	jobQueue * m_jobQueue;
	volatile bool m_idle;

} ;



mixer::mixer( void ) :
	m_framesPerPeriod( DEFAULT_BUFFER_SIZE ),
	m_readBuf( NULL ),
	m_writeBuf( NULL ),
	m_cpuLoad( 0 ),
	m_multiThreaded( QThread::idealThreadCount() > 1 ),
	m_workers(),
	m_numWorkers( m_multiThreaded ? QThread::idealThreadCount() : 0 ),
	m_workerSem( m_numWorkers  ),
	m_qualityLevel( DEFAULT_QUALITY_LEVEL ),
	m_masterGain( 1.0f ),
	m_audioDev( NULL ),
	m_oldAudioDev( NULL ),
	m_globalMutex( QMutex::Recursive )
{
	if( configManager::inst()->value( "mixer", "framesperaudiobuffer"
						).toInt() >= 32 )
	{
		m_framesPerPeriod = configManager::inst()->value( "mixer",
					"framesperaudiobuffer" ).toInt();

		if( m_framesPerPeriod > DEFAULT_BUFFER_SIZE )
		{
			m_fifo = new fifo( m_framesPerPeriod
							/ DEFAULT_BUFFER_SIZE );
			m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		}
		else
		{
			m_fifo = new fifo( 1 );
		}
	}
	else
	{
		configManager::inst()->setValue( "mixer",
							"framesperaudiobuffer",
				QString::number( m_framesPerPeriod ) );
		m_fifo = new fifo( 1 );
	}

	for( Uint8 i = 0; i < 3; i++ )
	{
		m_readBuf = new surroundSampleFrame[m_framesPerPeriod];
		
		clearAudioBuffer( m_readBuf, m_framesPerPeriod );
		m_bufferPool.push_back( m_readBuf );
	}

	if( m_multiThreaded )
	{
		for( int i = 0; i < m_numWorkers; ++i )
		{
			m_workers.push_back( new mixerWorkerThread( this, i ) );
		}
	}

	setClipScaling( FALSE );
}




mixer::~mixer()
{
	while( m_fifo->available() )
	{
		delete[] m_fifo->read();
	}
	delete m_fifo;

	delete m_audioDev;
	delete m_midiClient;

	for( Uint8 i = 0; i < 3; i++ )
	{
		delete[] m_bufferPool[i];
	}
}




void mixer::initDevices( void )
{
	m_audioDev = tryAudioDevices();
	m_midiClient = tryMIDIClients();
}




void mixer::startProcessing( void )
{
	m_fifo_writer = new fifoWriter( this, m_fifo );
	m_fifo_writer->start();

	m_audioDev->startProcessing();
}




void mixer::stopProcessing( void )
{
	m_fifo_writer->finish();

	m_audioDev->stopProcessing();

	m_fifo_writer->wait( 1000 );
	m_fifo_writer->terminate();
	delete m_fifo_writer;
}




bool mixer::criticalXRuns( void ) const
{
	return( ( m_cpuLoad >= 99 &&
				engine::getSong()->realTimeTask() == TRUE ) );
}




void mixer::setClipScaling( bool _state )
{
	lock();

	m_scaleClip = _state;

	if( _state )
	{
		m_poolDepth = 3;
		m_readBuffer = 0;
		m_analBuffer = m_readBuffer + 1;
		m_writeBuffer = m_poolDepth - 1;

		for( ch_cnt_t chnl=0; chnl < m_audioDev->channels(); ++chnl )
		{
			m_clipped[chnl] = FALSE;
			m_halfStart[chnl] = m_framesPerPeriod;
			m_maxClip[chnl] = 1.0f;
			m_previousSample[chnl] = 0.0;
			m_newBuffer[chnl] = FALSE;
		}
		// FIXME: why assign buffer-ptr to m_readBuf just for calling
		//        another method?
		//        clearAudioBuffer(m_bufferPool[i],...) would do as well
		for( Uint8 i = 0; i < 3; i++ )
		{
			m_readBuf = m_bufferPool[i];
			clearAudioBuffer( m_readBuf, m_framesPerPeriod );
		}
	}
	else
	{
		m_poolDepth = 2;
		m_readBuffer = 0;
		m_writeBuffer = 1;
		m_analBuffer = 1;
	}

	unlock();
}



#define FILL_JOB_QUEUE(_jq,_vec_type,_vec,_job_type,_condition)		\
	int id = 0;							\
	for( _vec_type::iterator it = _vec.begin();			\
				it != _vec.end(); ++it )		\
	{								\
		if( _condition )					\
		{							\
			_jq.items.push_back(				\
			mixerWorkerThread::jobQueueItem( id, _job_type,	\
								*it ) );\
			id = (id+1) % m_numWorkers;			\
		}							\
	}

#define DISTRIBUTE_JOB_QUEUE(_jq)					\
	_jq.remaining = _jq.items.size();				\
	for( int i = 0; i < m_numWorkers; ++i )				\
	{								\
		m_workers[i]->addJob( &_jq );				\
	}

#define WAIT_FOR_JOBS(_cond)						\
	while( m_workerSem.available() < m_numWorkers )			\
	{								\
		m_workerSem.acquire( 1 );				\
		m_workerSem.release( 1 );				\
		jq.lock.lockForRead();					\
		const int r = jq.remaining;				\
		jq.lock.unlock();					\
		if( m_workerSem.available() >= m_numWorkers ||		\
						r <= m_numWorkers )	\
			break;						\
		/* in case a worker has finished, try to re-assign */	\
		/* jobs of busy workers				*/	\
		for( int i = 0; i < m_numWorkers; ++i )			\
		{							\
			if( m_workers[i]->idle() )			\
			{						\
				int n = m_numWorkers-1;			\
for( mixerWorkerThread::jobQueueItems::iterator it = jq.items.end();	\
					it != jq.items.begin(); )	\
{									\
	--it;								\
	jq.lock.lockForRead();						\
	if( _cond && !it->done && it->workerID != i &&			\
				(++n) % m_numWorkers == 0 )		\
	{								\
		jq.lock.unlock();					\
		jq.lock.lockForWrite();					\
		it->workerID = i;					\
	}								\
	jq.lock.unlock();						\
}									\
				m_workers[i]->addJob( &jq );		\
				break;					\
			}						\
		}							\
	}								\
	m_workerSem.acquire( m_numWorkers );				\
	m_workerSem.release( m_numWorkers );



const surroundSampleFrame * mixer::renderNextBuffer( void )
{
	microTimer timer;
	static song::playPos last_metro_pos = -1;

	song::playPos p = engine::getSong()->getPlayPos(
						song::Mode_PlayPattern );
	if( engine::getSong()->playMode() == song::Mode_PlayPattern &&
		engine::getPianoRoll()->isRecording() == TRUE &&
		p != last_metro_pos && p.getTact64th() % 16 == 0 )
	{
		addPlayHandle( new samplePlayHandle( "misc/metronome01.ogg" ) );
		last_metro_pos = p;
	}

	// now we have to make sure no other thread does anything bad
	// while we're acting...
	lock();

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
	lockPlayHandles();
	lockPlayHandlesToRemove();
	constPlayHandleVector::iterator it_rem = m_playHandlesToRemove.begin();
	while( it_rem != m_playHandlesToRemove.end() )
	{
		playHandleVector::iterator it = qFind( m_playHandles.begin(),
						m_playHandles.end(), *it_rem );

		if( it != m_playHandles.end() )
		{
			delete *it;
			m_playHandles.erase( it );
		}

		m_playHandlesToRemove.erase( it_rem );
	}
	unlockPlayHandlesToRemove();
	unlockPlayHandles();

	// now swap the buffers... current buffer becomes next (last)
	// buffer and the next buffer becomes current (first) buffer
//	qSwap( m_curBuf, m_nextBuf );
	m_writeBuffer++;
	m_writeBuffer %= m_poolDepth;

	m_readBuffer++;
	m_readBuffer %= m_poolDepth;

	m_analBuffer++;
	m_analBuffer %= m_poolDepth;

	m_writeBuf = m_bufferPool[m_writeBuffer];
	m_readBuf = m_bufferPool[m_readBuffer];

	// clear last audio-buffer
	clearAudioBuffer( m_writeBuf, m_framesPerPeriod );
//printf("---------------------------next period\n");
//	if( criticalXRuns() == FALSE )
	{
		engine::getSong()->processNextBuffer();

		lockPlayHandles();
		int idx = 0;
		if( m_multiThreaded )
		{
			playHandleVector par_hndls;
			while( idx < m_playHandles.size() )
			{
				playHandle * n = m_playHandles[idx];
				if( !n->done() && n->supportsParallelizing() )
				{
					n->play( TRUE );
					par_hndls.push_back( n );
				}
				++idx;
			}
			mixerWorkerThread::jobQueue jq;
			FILL_JOB_QUEUE(jq,playHandleVector,m_playHandles,
					mixerWorkerThread::PlayHandle,
					!( *it )->done() &&
					!( *it )->supportsParallelizing() );
// we have to process all note-play-handles of a monophonic instrument by the
// same thread serially as monophonic instruments rely on processing note-play-
// handles in correct order
			QHash<instrumentTrack *, int> h;
			for( mixerWorkerThread::jobQueueItems::iterator it =
				jq.items.begin(); it != jq.items.end(); ++it )
			{
#define COND_NPH static_cast<playHandle *>( it->job )->type() == playHandle::NotePlayHandle
#define COND_MONOPHONIC static_cast<notePlayHandle *>( it->job )->	\
				getInstrumentTrack()->			\
				getInstrument()->isMonophonic()
if( COND_NPH )
{
	if( COND_MONOPHONIC )
	{
		notePlayHandle * n = static_cast<notePlayHandle *>( it->job );
		if( h.contains( n->getInstrumentTrack() ) )
		{
			it->workerID = h[n->getInstrumentTrack()];
		}
		else
		{
			h[n->getInstrumentTrack()] = it->workerID;
		}
	}
}
			}
			DISTRIBUTE_JOB_QUEUE(jq);
			for( playHandleVector::iterator it = par_hndls.begin();
						it != par_hndls.end(); ++it )
			{
				( *it )->waitForWorkerThread();
			}
			WAIT_FOR_JOBS( h.size() > 0 && ( COND_NPH ? !COND_MONOPHONIC : TRUE ) );
		}
		else
		{
			for( playHandleVector::iterator it =
						m_playHandles.begin();
					it != m_playHandles.end(); ++it )
			{
				if( !( *it )->done() )
				{

					( *it )->play();
				}
			}
		}
		idx = 0;
		while( idx < m_playHandles.size() )
		{
			playHandle * n = m_playHandles[idx];
			if( n->done() )
			{
				delete n;
				m_playHandles.erase(
					m_playHandles.begin() + idx );
			}
			else
			{
				++idx;
			}
		}
		unlockPlayHandles();
		if( m_multiThreaded )
		{
			mixerWorkerThread::jobQueue jq;
			FILL_JOB_QUEUE(jq,QVector<audioPort*>,m_audioPorts,
					mixerWorkerThread::AudioPortEffects,1);
			DISTRIBUTE_JOB_QUEUE(jq);
			WAIT_FOR_JOBS(TRUE);
		}
		else
		{
			bool more_effects = FALSE;
			for( QVector<audioPort *>::iterator it = m_audioPorts.begin();
						it != m_audioPorts.end(); ++it )
			{
				more_effects = ( *it )->processEffects();
				if( ( *it )->m_bufferUsage != audioPort::NoUsage ||
								more_effects )
				{
					processBuffer( ( *it )->firstBuffer(),
							( *it )->nextFxChannel() );
					( *it )->nextPeriod();
				}
			}
		}
	}

	unlock();

	emit nextAudioBuffer();

	// and trigger LFOs
	envelopeAndLFOParameters::triggerLFO();

	const float new_cpu_load = timer.elapsed() / 10000.0f * sampleRate() /
							m_framesPerPeriod;
	m_cpuLoad = tLimit( (int) ( new_cpu_load * 0.1f + m_cpuLoad * 0.9f ), 0,
									100 );

	return( m_readBuf );
}




// removes all play-handles. this is neccessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void mixer::clear( void )
{
	// TODO: m_midiClient->noteOffAll();
	lockPlayHandlesToRemove();
	for( playHandleVector::iterator it = m_playHandles.begin();
					it != m_playHandles.end(); ++it )
	{
		// we must not delete instrument-play-handles as they exist
		// during the whole lifetime of an instrument
		if( ( *it )->type() != playHandle::InstrumentPlayHandle )
		{
			m_playHandlesToRemove.push_back( *it );
		}
	}
	unlockPlayHandlesToRemove();
}




void FASTCALL mixer::clearAudioBuffer( sampleFrame * _ab,
							const f_cnt_t _frames,
							const f_cnt_t _offset )
{
	memset( _ab+_offset, 0, sizeof( *_ab ) * _frames );
}



#ifndef DISABLE_SURROUND
void FASTCALL mixer::clearAudioBuffer( surroundSampleFrame * _ab,
							const f_cnt_t _frames,
							const f_cnt_t _offset )
{
	memset( _ab+_offset, 0, sizeof( *_ab ) * _frames );
}
#endif



void FASTCALL mixer::bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					const volumeVector & _volume_vector,
						audioPort * _port )
{
	const fpp_t start_frame = _offset % m_framesPerPeriod;
	fpp_t end_frame = start_frame + _frames;
	const fpp_t loop1_frame = tMin( end_frame, m_framesPerPeriod );

	_port->lockFirstBuffer();
	for( fpp_t frame = start_frame; frame < loop1_frame; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < m_audioDev->channels(); ++chnl )
		{
			_port->firstBuffer()[frame][chnl] +=
					_buf[frame - start_frame][chnl %
							DEFAULT_CHANNELS] *
						_volume_vector.vol[chnl];
		}
	}
	_port->unlockFirstBuffer();

	_port->lockSecondBuffer();
	if( end_frame > m_framesPerPeriod )
	{
		fpp_t frames_done = m_framesPerPeriod - start_frame;
		end_frame = tMin( end_frame -= m_framesPerPeriod,
						m_framesPerPeriod );
		for( fpp_t frame = 0; frame < end_frame; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < m_audioDev->channels();
									++chnl )
			{
				_port->secondBuffer()[frame][chnl] +=
					_buf[frames_done + frame][chnl %
							DEFAULT_CHANNELS] *
						_volume_vector.vol[chnl];
			}
		}
		// we used both buffers so set flags
		_port->m_bufferUsage = audioPort::BothBuffers;
	}
	else if( _port->m_bufferUsage == audioPort::NoUsage )
	{
		// only first buffer touched
		_port->m_bufferUsage = audioPort::FirstBuffer;
	}
	_port->unlockSecondBuffer();
}




void mixer::setHighQuality( bool _hq_on )
{
	// don't delete the audio-device
	stopProcessing();

	// set new quality-level...
	m_qualityLevel = ( _hq_on == TRUE ) ? HIGH_QUALITY_LEVEL :
							DEFAULT_QUALITY_LEVEL;

	startProcessing();

	emit( sampleRateChanged() );

}




void FASTCALL mixer::setAudioDevice( audioDevice * _dev, bool _hq )
{
	stopProcessing();

	m_oldAudioDev = m_audioDev;

	if( _dev == NULL )
	{
		printf( "param _dev == NULL in mixer::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
	else
	{
		m_audioDev = _dev;
	}

	m_qualityLevel = _hq ? HIGH_QUALITY_LEVEL : DEFAULT_QUALITY_LEVEL;
	emit sampleRateChanged();

	startProcessing();
}




void mixer::restoreAudioDevice( void )
{
	if( m_oldAudioDev != NULL )
	{
		stopProcessing();
		delete m_audioDev;

		m_audioDev = m_oldAudioDev;
		for( Uint8 qli = DEFAULT_QUALITY_LEVEL;
						qli < QUALITY_LEVELS; ++qli )
		{
			if( SAMPLE_RATES[qli] == m_audioDev->sampleRate() )
			{
				m_qualityLevel =
					static_cast<qualityLevels>( qli );
				emit sampleRateChanged();
				break;
			}
		}
		m_oldAudioDev = NULL;
		startProcessing();
	}
}




void mixer::removePlayHandles( track * _track )
{
	lockPlayHandles();
	playHandleVector::iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		if( ( *it )->isFromTrack( _track ) )
		{
			delete *it;
			m_playHandles.erase( it );
		}
		else
		{
			++it;
		}
	}
	unlockPlayHandles();
}





audioDevice * mixer::tryAudioDevices( void )
{
	bool success_ful = FALSE;
	audioDevice * dev = NULL;
	QString dev_name = configManager::inst()->value( "mixer", "audiodev" );

#ifdef ALSA_SUPPORT
	if( dev_name == audioALSA::name() || dev_name == "" )
	{
		dev = new audioALSA( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioALSA::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef OSS_SUPPORT
	if( dev_name == audioOSS::name() || dev_name == "" )
	{
		dev = new audioOSS( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioOSS::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef JACK_SUPPORT
	if( dev_name == audioJACK::name() || dev_name == "" )
	{
		dev = new audioJACK( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioJACK::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef SDL_AUDIO_SUPPORT
	if( dev_name == audioSDL::name() || dev_name == "" )
	{
		dev = new audioSDL( SAMPLE_RATES[DEFAULT_QUALITY_LEVEL],
							success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioSDL::name();
			return( dev );
		}
		delete dev;
	}
#endif

	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, this );
	//if( sucess_ful )
	//{
	//	return( dev );
	//}
	//delete dev

	printf( "No audio-driver working - falling back to dummy-audio-"
		"driver\nYou can render your songs and listen to the output "
		"files...\n" );

	m_audioDevName = audioDummy::name();

	return( new audioDummy( SAMPLE_RATES[m_qualityLevel], success_ful,
								this ) );
}




midiClient * mixer::tryMIDIClients( void )
{
	QString client_name = configManager::inst()->value( "mixer",
								"mididev" );

#ifdef ALSA_SUPPORT
	if( client_name == midiALSASeq::name() || client_name == "" )
	{
		midiALSASeq * malsas = new midiALSASeq;
		if( malsas->isRunning() )
		{
			m_midiClientName = midiALSASeq::name();
			return( malsas );
		}
		delete malsas;
	}

	if( client_name == midiALSARaw::name() || client_name == "" )
	{
		midiALSARaw * malsar = new midiALSARaw;
		if( malsar->isRunning() )
		{
			m_midiClientName = midiALSARaw::name();
			return( malsar );
		}
		delete malsar;
	}
#endif

#ifdef OSS_SUPPORT
	if( client_name == midiOSS::name() || client_name == "" )
	{
		midiOSS * moss = new midiOSS;
		if( moss->isRunning() )
		{
			m_midiClientName = midiOSS::name();
			return( moss );
		}
		delete moss;
	}
#endif

	printf( "Couldn't create MIDI-client, neither with ALSA nor with "
		"OSS. Will use dummy-MIDI-client.\n" );

	m_midiClientName = midiDummy::name();

	return( new midiDummy );
}




void mixer::processBuffer( const surroundSampleFrame * _buf,
						fx_ch_t/* _fx_chnl */ )
{
	// TODO: process according effect-channel
	
/*	if( m_scaleClip )
	{
		for( ch_cnt_t chnl=0; 
			chnl < m_audioDev->channels(); 
			++chnl )
		{
			m_newBuffer[chnl] = TRUE;
		}
	}*/

	static QMutex m;
	m.lock();
	for( fpp_t frame = 0; frame < m_framesPerPeriod; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < m_audioDev->channels(); ++chnl )
		{
			m_writeBuf[frame][chnl] += _buf[frame][chnl];
/*
			if( m_scaleClip )
			{
				scaleClip( frame, chnl );
			}*/
		}
	}
	m.unlock();
}




void FASTCALL mixer::scaleClip( fpp_t _frame, ch_cnt_t _chnl )
{
	// Check for zero crossing
	if( ( m_writeBuf[_frame][_chnl] >=0 &&
		     m_previousSample[_chnl] < 0 ) ||
		     ( m_writeBuf[_frame][_chnl] <=0 &&
		     m_previousSample[_chnl] > 0 ) )
	{
		// if a clip occurred between the zero
		// crossings, scale the half-wave
		if( m_clipped[_chnl] )
		{
			if( m_newBuffer[_chnl] )
			{
				for( fpp_t i = m_halfStart[_chnl];
					i < m_framesPerPeriod;
					i++ )
				{
					m_bufferPool[m_analBuffer][i][_chnl] /=
							m_maxClip[_chnl];
				}
					
				for( fpp_t i = 0;
					i < _frame;
					i++ )
				{
					m_writeBuf[i][_chnl] /= 
							m_maxClip[_chnl];
				}
			}
			else
			{
				for( fpp_t i = m_halfStart[_chnl];
					 i < _frame;
					 i++ )
				{
					m_writeBuf[i][_chnl] /= m_maxClip[_chnl];
				}
			}
		}
		m_halfStart[_chnl] = _frame;
		m_clipped[_chnl] = FALSE;
		m_newBuffer[_chnl] = FALSE;
		m_maxClip[_chnl] = 1.0;
	}
			
	// check for clip
	if( fabsf( m_writeBuf[_frame][_chnl] ) > 1.0f )
	{
		m_clipped[_chnl] = TRUE;
		if( fabs( m_writeBuf[_frame][_chnl] ) >
				  m_maxClip[_chnl] )
		{
			m_maxClip[_chnl] = fabs( 
					m_writeBuf[_frame][_chnl] );
		}
	}
			
	m_previousSample[_chnl] = m_writeBuf[_frame][_chnl];
}








mixer::fifoWriter::fifoWriter( mixer * _mixer, fifo * _fifo ) :
	m_mixer( _mixer ),
	m_fifo( _fifo ),
	m_writing( TRUE )
{
}




void mixer::fifoWriter::finish( void )
{
	m_writing = FALSE;
}




void mixer::fifoWriter::run( void )
{
	while( m_writing )
	{
		fpp_t frames = m_mixer->framesPerPeriod();
		surroundSampleFrame * buffer = new surroundSampleFrame[frames];
		const surroundSampleFrame * b = m_mixer->renderNextBuffer();
		memcpy( buffer, b, frames * sizeof( surroundSampleFrame ) );
		m_fifo->write( buffer );
	}

	m_fifo->write( NULL );
}




#include "mixer.moc"


#endif
