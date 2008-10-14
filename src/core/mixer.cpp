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


#include <math.h>

#include "mixer.h"
#include "fx_mixer.h"
#include "play_handle.h"
#include "effect.h"
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

#include "audio_device.h"
#include "midi_client.h"

// platform-specific audio-interface-classes
#include "audio_alsa.h"
#include "audio_jack.h"
#include "audio_oss.h"
#include "audio_portaudio.h"
#include "audio_pulseaudio.h"
#include "audio_sdl.h"
#include "audio_dummy.h"

// platform-specific midi-interface-classes
#include "midi_alsa_raw.h"
#include "midi_alsa_seq.h"
#include "midi_oss.h"
#include "midi_winmm.h"
#include "midi_dummy.h"


static QVector<fx_ch_t> __fx_channel_jobs( NumFxChannels );



static void aligned_free( void * _buf )
{
	if( _buf != NULL )
	{
		int *ptr2=(int *)_buf - 1;
		_buf = (char *)_buf- *ptr2;
		free(_buf);
	}
}

static void * aligned_malloc( int _bytes )
{
	char *ptr,*ptr2,*aligned_ptr;
	int align_mask = ALIGN_SIZE- 1;
	ptr=(char *)malloc(_bytes +ALIGN_SIZE+ sizeof(int));
	if(ptr==NULL) return(NULL);

	ptr2 = ptr + sizeof(int);
	aligned_ptr = ptr2 + (ALIGN_SIZE- ((size_t)ptr2 & align_mask));


	ptr2 = aligned_ptr - sizeof(int);
	*((int *)ptr2)=(int)(aligned_ptr - ptr);

	return(aligned_ptr);
}



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
			type( InvalidJob ),
			job( NULL ),
			done( false )
		{
		}
		jobQueueItem( JobTypes _type, void * _job ) :
			type( _type ),
			job( _job ),
			done( false )
		{
		}

		JobTypes type;

		union
		{
			playHandle * playHandleJob;
			audioPort * audioPortJob;
			int effectChannelJob;
			volatile void * job;
		};

#if QT_VERSION >= 0x040400
		QAtomicInt done;
#else
		volatile bool done;
#endif
	} ;

	typedef QVector<jobQueueItem> jobQueueItems;
	struct jobQueue
	{
		jobQueueItems items;
#if QT_VERSION < 0x040400
		QMutex lock;
#endif
	} ;

	static jobQueue s_jobQueue;

	mixerWorkerThread( mixer * _mixer ) :
		QThread( _mixer ),
		m_quit( false ),
		m_mixer( _mixer ),
		m_queueReadySem( &m_mixer->m_queueReadySem ),
		m_workersDoneSem( &m_mixer->m_workersDoneSem )
	{
		start( QThread::TimeCriticalPriority );
	}

	virtual ~mixerWorkerThread()
	{
	}

	virtual void quit( void )
	{
		m_quit = true;
	}

private:
	virtual void run( void )
	{
		sampleFrame * working_buf = (sampleFrame *) aligned_malloc(
						m_mixer->framesPerPeriod() *
							sizeof( sampleFrame ) );
		while( m_quit == false )
		{
			m_queueReadySem->acquire();
			jobQueueItems::iterator end_it = s_jobQueue.items.end();
			for( jobQueueItems::iterator it =
						s_jobQueue.items.begin();
							it != end_it; ++it )
			{
#if QT_VERSION >= 0x040400
				if( it->done.fetchAndStoreOrdered( 1 ) == 0 )
				{
#else
				s_jobQueue.lock.lock();
				if( !it->done )
				{
					it->done = true;
					s_jobQueue.lock.unlock();
#endif
					switch( it->type )
					{
						case PlayHandle:
		it->playHandleJob->play( working_buf );
							break;
						case AudioPortEffects:
							{
		audioPort * a = it->audioPortJob;
		const bool me = a->processEffects();
		if( me || a->m_bufferUsage != audioPort::NoUsage )
		{
			engine::getFxMixer()->mixToChannel( a->firstBuffer(),
							a->nextFxChannel() );
			a->nextPeriod();
		}
							}
							break;
						case EffectChannel:
		engine::getFxMixer()->processChannel(
				(fx_ch_t) it->effectChannelJob );
							break;
						default:
fprintf( stderr, "invalid job item type %d at %d in jobqueue(%d:%d)\n",
		(int) it->type, (int) it, (int) s_jobQueue.items.begin(),
							(int) end_it );
							break;
					}
				}
#if QT_VERSION < 0x040400
				else
				{
					s_jobQueue.lock.unlock();
				}
#endif
			}
			m_workersDoneSem->release();
		}
		aligned_free( working_buf );
	}

	volatile bool m_quit;
	mixer * m_mixer;
	QSemaphore * m_queueReadySem;
	QSemaphore * m_workersDoneSem;

} ;

mixerWorkerThread::jobQueue mixerWorkerThread::s_jobQueue;


#define FILL_JOB_QUEUE(_vec_type,_vec,_job_type,_condition)		\
	mixerWorkerThread::s_jobQueue.items.clear();			\
	for( _vec_type::iterator it = _vec.begin();			\
				it != _vec.end(); ++it )		\
	{								\
		if( _condition )					\
		{							\
			mixerWorkerThread::s_jobQueue.items.		\
					push_back(			\
			mixerWorkerThread::jobQueueItem( _job_type,	\
							(void *)*it ) );\
		}							\
	}

#define START_JOBS()							\
	m_queueReadySem.release( m_numWorkers );			\

#define WAIT_FOR_JOBS()							\
	m_workersDoneSem.acquire( m_numWorkers );




mixer::mixer( void ) :
	m_framesPerPeriod( DEFAULT_BUFFER_SIZE ),
	m_workingBuf( NULL ),
	m_inputBufferRead( 0 ),
	m_inputBufferWrite( 1 ),
	m_readBuf( NULL ),
	m_writeBuf( NULL ),
	m_cpuLoad( 0 ),
	m_multiThreaded( QThread::idealThreadCount() > 1 ),
	m_workers(),
	m_numWorkers( m_multiThreaded ? QThread::idealThreadCount() : 0 ),
	m_queueReadySem( m_numWorkers ),
	m_workersDoneSem( m_numWorkers ),
	m_qualitySettings( qualitySettings::Mode_Draft ),
	m_masterGain( 1.0f ),
	m_audioDev( NULL ),
	m_oldAudioDev( NULL ),
	m_globalMutex( QMutex::Recursive )
{
	for( int i = 0; i < 2; ++i )
	{
		m_inputBufferFrames[i] = 0;
		m_inputBufferSize[i] = DEFAULT_BUFFER_SIZE * 100;
		m_inputBuffer[i] = new sampleFrame[ DEFAULT_BUFFER_SIZE * 100 ];
		clearAudioBuffer( m_inputBuffer[i], m_inputBufferSize[i] );
	}

	for( int i = 1; i < NumFxChannels+1; ++i )
	{
		__fx_channel_jobs[i-1] = (fx_ch_t) i;
	}

	// just rendering?
	if( !engine::hasGUI() )
	{
		m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		m_fifo = new fifo( 1 );
	}
	else if( configManager::inst()->value( "mixer", "framesperaudiobuffer"
						).toInt() >= 32 )
	{
		m_framesPerPeriod =
			(fpp_t) configManager::inst()->value( "mixer",
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

	m_workingBuf = (sampleFrame*) aligned_malloc( m_framesPerPeriod *
							sizeof( sampleFrame ) );
	for( Uint8 i = 0; i < 3; i++ )
	{
		m_readBuf = (surroundSampleFrame*)
			aligned_malloc( m_framesPerPeriod *
						sizeof( surroundSampleFrame ) );

		clearAudioBuffer( m_readBuf, m_framesPerPeriod );
		m_bufferPool.push_back( m_readBuf );
	}

	if( m_multiThreaded )
	{
		m_queueReadySem.acquire( m_numWorkers );
		m_workersDoneSem.acquire( m_numWorkers );
		for( int i = 0; i < m_numWorkers; ++i )
		{
			m_workers.push_back( new mixerWorkerThread( this ) );
		}
	}

	m_poolDepth = 2;
	m_readBuffer = 0;
	m_writeBuffer = 1;
	m_analBuffer = 1;
}




mixer::~mixer()
{
	if( m_multiThreaded )
	{
		// distribute an empty job-queue so that worker-threads
		// get out of their processing-loop
		mixerWorkerThread::s_jobQueue.items.clear();
		for( int w = 0; w < m_numWorkers; ++w )
		{
			m_workers[w]->quit();
		}
		START_JOBS();
		for( int w = 0; w < m_numWorkers; ++w )
		{
			m_workers[w]->wait( 500 );
		}
	}

	while( m_fifo->available() )
	{
		delete[] m_fifo->read();
	}
	delete m_fifo;

	delete m_audioDev;
	delete m_midiClient;

	for( Uint8 i = 0; i < 3; i++ )
	{
		aligned_free( m_bufferPool[i] );
	}

	aligned_free( m_workingBuf );
}




void mixer::initDevices( void )
{
	m_audioDev = tryAudioDevices();
	m_midiClient = tryMidiClients();
}




void mixer::startProcessing( bool _needs_fifo )
{
	if( _needs_fifo )
	{
		m_fifoWriter = new fifoWriter( this, m_fifo );
		m_fifoWriter->start();
	}
	else
	{
		m_fifoWriter = NULL;
	}

	m_audioDev->startProcessing();
}




void mixer::stopProcessing( void )
{
	if( m_fifoWriter != NULL )
	{
		m_fifoWriter->finish();
		m_audioDev->stopProcessing();
		m_fifoWriter->wait( 1000 );
		m_fifoWriter->terminate();
		delete m_fifoWriter;
		m_fifoWriter = NULL;
	}
	else
	{
		m_audioDev->stopProcessing();
	}
}




sample_rate_t mixer::baseSampleRate( void ) const
{
	sample_rate_t sr =
		configManager::inst()->value( "mixer", "samplerate" ).toInt();
	if( sr < 44100 )
	{
		sr = 44100;
	}
	return( sr );
}




sample_rate_t mixer::outputSampleRate( void ) const
{
	return( m_audioDev != NULL ? m_audioDev->sampleRate() :
							baseSampleRate() );
}




sample_rate_t mixer::inputSampleRate( void ) const
{
	return( m_audioDev != NULL ? m_audioDev->sampleRate() :
							baseSampleRate() );
}




sample_rate_t mixer::processingSampleRate( void ) const
{
	return( outputSampleRate() * m_qualitySettings.sampleRateMultiplier() );
}




bool mixer::criticalXRuns( void ) const
{
	return( ( m_cpuLoad >= 99 &&
				engine::getSong()->realTimeTask() == true ) );
}




void mixer::pushInputFrames( sampleFrame * _ab, const f_cnt_t _frames )
{
	lockInputFrames();

	f_cnt_t frames = m_inputBufferFrames[ m_inputBufferWrite ];
	int size = m_inputBufferSize[ m_inputBufferWrite ];
	sampleFrame * buf = m_inputBuffer[ m_inputBufferWrite ];
	
	if( frames + _frames > size )
	{
		size = qMax( size * 2, frames + _frames );
		sampleFrame * ab = new sampleFrame[ size ];
		memcpy( ab, buf, frames * sizeof( sampleFrame ) );
		delete [] buf;

		m_inputBufferSize[ m_inputBufferWrite ] = size;
		m_inputBuffer[ m_inputBufferWrite ] = ab;

		buf = ab;
	}
	
	memcpy( &buf[ frames ], _ab, _frames * sizeof( sampleFrame ) );
	m_inputBufferFrames[ m_inputBufferWrite ] += _frames;
	
	unlockInputFrames();
}




const surroundSampleFrame * mixer::renderNextBuffer( void )
{
	microTimer timer;
	static song::playPos last_metro_pos = -1;

	song::playPos p = engine::getSong()->getPlayPos(
						song::Mode_PlayPattern );
	if( engine::getSong()->playMode() == song::Mode_PlayPattern &&
		engine::getPianoRoll()->isRecording() == true &&
		p != last_metro_pos && p.getTicks() %
					(DefaultTicksPerTact / 4 ) == 0 )
	{
		addPlayHandle( new samplePlayHandle( "misc/metronome01.ogg" ) );
		last_metro_pos = p;
	}

	lockInputFrames();
	// swap buffer
	m_inputBufferWrite++;
	m_inputBufferWrite %= 2;
	m_inputBufferRead++;
	m_inputBufferRead %= 2;
	// clear new write buffer
	m_inputBufferFrames[ m_inputBufferWrite ] = 0;
	unlockInputFrames();
	
	// now we have to make sure no other thread does anything bad
	// while we're acting...
	lock();

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
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

		it_rem = m_playHandlesToRemove.erase( it_rem );
	}

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
//	if( criticalXRuns() == false )
	{
		engine::getFxMixer()->prepareMasterMix();
		engine::getSong()->processNextBuffer();

		if( m_multiThreaded )
		{
			FILL_JOB_QUEUE(playHandleVector,m_playHandles,
					mixerWorkerThread::PlayHandle,
					!( *it )->done());
			START_JOBS();
			WAIT_FOR_JOBS();
		}
		else
		{
			for( playHandleVector::iterator it =
						m_playHandles.begin();
					it != m_playHandles.end(); ++it )
			{
				if( !( *it )->done() )
				{
					// play now and don't try to
					// parallelize as we're on single-core
					// system
					( *it )->play( m_workingBuf );
				}
			}
		}
		for( playHandleVector::iterator it = m_playHandles.begin();
					it != m_playHandles.end(); )
		{
			if( ( *it )->affinityMatters() &&
				( *it )->affinity() != QThread::currentThread() )
			{
				++it;
				continue;
			}
			if( ( *it )->done() )
			{
				delete *it;
				it = m_playHandles.erase( it );
			}
			else
			{
				++it;
			}
		}

		if( m_multiThreaded )
		{
			FILL_JOB_QUEUE(QVector<audioPort*>,m_audioPorts,
					mixerWorkerThread::AudioPortEffects,1);
			START_JOBS();
			WAIT_FOR_JOBS();

			FILL_JOB_QUEUE(QVector<fx_ch_t>,__fx_channel_jobs,
					mixerWorkerThread::EffectChannel,1);
			START_JOBS();
			WAIT_FOR_JOBS();
		}
		else
		{
			bool more_effects = false;
			for( QVector<audioPort *>::iterator it =
							m_audioPorts.begin();
						it != m_audioPorts.end(); ++it )
			{
				more_effects = ( *it )->processEffects();
				if( ( *it )->m_bufferUsage !=
							audioPort::NoUsage ||
								more_effects )
				{
					engine::getFxMixer()->mixToChannel(
							( *it )->firstBuffer(),
						( *it )->nextFxChannel() );
					( *it )->nextPeriod();
				}
			}
			for( int i = 1; i < NumFxChannels+1; ++i )
			{
				engine::getFxMixer()->processChannel(
								(fx_ch_t) i );
			}
		}
		engine::getFxMixer()->masterMix( m_writeBuf );
	}

	unlock();

	emit nextAudioBuffer();

	// and trigger LFOs
	envelopeAndLFOParameters::triggerLFO();
	controller::triggerFrameCounter();

	const float new_cpu_load = timer.elapsed() / 10000.0f *
				processingSampleRate() / m_framesPerPeriod;
	m_cpuLoad = tLimit( (int) ( new_cpu_load * 0.1f + m_cpuLoad * 0.9f ), 0,
									100 );

	return( m_readBuf );
}




// removes all play-handles. this is neccessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void mixer::clear( void )
{
	// TODO: m_midiClient->noteOffAll();
	lock();
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
	unlock();
}




void mixer::bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					stereoVolumeVector _vv,
						audioPort * _port )
{
	const int start_frame = _offset % m_framesPerPeriod;
	int end_frame = start_frame + _frames;
	const int loop1_frame = qMin<int>( end_frame, m_framesPerPeriod );

	_port->lockFirstBuffer();
	for( int frame = start_frame; frame < loop1_frame; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_port->firstBuffer()[frame][chnl] +=
					_buf[frame - start_frame][chnl] *
								_vv.vol[chnl];
		}
	}
	_port->unlockFirstBuffer();

	_port->lockSecondBuffer();
	if( end_frame > m_framesPerPeriod )
	{
		const int frames_done = m_framesPerPeriod - start_frame;
		end_frame -= m_framesPerPeriod;
		end_frame = qMin<int>( end_frame, m_framesPerPeriod );
		for( fpp_t frame = 0; frame < end_frame; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				_port->secondBuffer()[frame][chnl] +=
					_buf[frames_done + frame][chnl] *
								_vv.vol[chnl];
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




void mixer::clearAudioBuffer( sampleFrame * _ab, const f_cnt_t _frames,
							const f_cnt_t _offset )
{
	memset( _ab+_offset, 0, sizeof( *_ab ) * _frames );
}



#ifndef LMMS_DISABLE_SURROUND
void mixer::clearAudioBuffer( surroundSampleFrame * _ab, const f_cnt_t _frames,
							const f_cnt_t _offset )
{
	memset( _ab+_offset, 0, sizeof( *_ab ) * _frames );
}
#endif




float mixer::peakValueLeft( sampleFrame * _ab, const f_cnt_t _frames )
{
	float p = 0.0f;
	for( f_cnt_t f = 0; f < _frames; ++f )
	{
		if( _ab[f][0] > p )
		{
			p = _ab[f][0];
		}
		else if( -_ab[f][0] > p )
		{
			p = -_ab[f][0];
		}
	}
	return( p );
}




float mixer::peakValueRight( sampleFrame * _ab, const f_cnt_t _frames )
{
	float p = 0.0f;
	for( f_cnt_t f = 0; f < _frames; ++f )
	{
		if( _ab[f][1] > p )
		{
			p = _ab[f][1];
		}
		else if( -_ab[f][1] > p )
		{
			p = -_ab[f][1];
		}
	}
	return( p );
}




void mixer::changeQuality( const struct qualitySettings & _qs )
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = _qs;
	m_audioDev->applyQualitySettings();

	emit sampleRateChanged();
	emit qualitySettingsChanged();

	startProcessing();
}




void mixer::setAudioDevice( audioDevice * _dev )
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

	emit sampleRateChanged();

	startProcessing();
}




void mixer::setAudioDevice( audioDevice * _dev,
				const struct qualitySettings & _qs,
				bool _needs_fifo )
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = _qs;
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

	emit qualitySettingsChanged();
	emit sampleRateChanged();

	startProcessing( _needs_fifo );
}




void mixer::restoreAudioDevice( void )
{
	if( m_oldAudioDev != NULL )
	{
		stopProcessing();
		delete m_audioDev;

		m_audioDev = m_oldAudioDev;
		emit sampleRateChanged();

		m_oldAudioDev = NULL;
		startProcessing();
	}
}




void mixer::removeAudioPort( audioPort * _port )
{
	QVector<audioPort *>::iterator it = qFind( m_audioPorts.begin(),
							m_audioPorts.end(),
							_port );
	if( it != m_audioPorts.end() )
	{
		lock();
		m_audioPorts.erase( it );
		unlock();
	}
}




void mixer::removePlayHandle( playHandle * _ph )
{
	lock();
	// check thread affinity as we must not delete play-handles
	// which were created in a thread different than mixer thread
	if( _ph->affinityMatters() &&
				_ph->affinity() == QThread::currentThread() )
	{
		playHandleVector::iterator it =
				qFind( m_playHandles.begin(),
						m_playHandles.end(), _ph );
		if( it != m_playHandles.end() )
		{
			m_playHandles.erase( it );
			delete _ph;
		}
	}
	else
	{
		m_playHandlesToRemove.push_back( _ph );
	}
	unlock();
}




void mixer::removePlayHandles( track * _track )
{
	lock();
	playHandleVector::iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		if( ( *it )->isFromTrack( _track ) )
		{
			delete *it;
			it = m_playHandles.erase( it );
		}
		else
		{
			++it;
		}
	}
	unlock();
}




audioDevice * mixer::tryAudioDevices( void )
{
	bool success_ful = false;
	audioDevice * dev = NULL;
	QString dev_name = configManager::inst()->value( "mixer", "audiodev" );

	if( dev_name == audioDummy::name() )
	{
		dev_name = "";
	}

#ifdef LMMS_HAVE_ALSA
	if( dev_name == audioALSA::name() || dev_name == "" )
	{
		dev = new audioALSA( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioALSA::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if( dev_name == audioPortAudio::name() || dev_name == "" )
	{
		dev = new audioPortAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioPortAudio::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if( dev_name == audioPulseAudio::name() || dev_name == "" )
	{
		dev = new audioPulseAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioPulseAudio::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if( dev_name == audioOSS::name() || dev_name == "" )
	{
		dev = new audioOSS( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioOSS::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_JACK
	if( dev_name == audioJACK::name() || dev_name == "" )
	{
		dev = new audioJACK( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = audioJACK::name();
			return( dev );
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_SDL
	if( dev_name == audioSDL::name() || dev_name == "" )
	{
		dev = new audioSDL( success_ful, this );
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

	return( new audioDummy( success_ful, this ) );
}




midiClient * mixer::tryMidiClients( void )
{
	QString client_name = configManager::inst()->value( "mixer",
								"mididev" );

#ifdef LMMS_HAVE_ALSA
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

#ifdef LMMS_HAVE_OSS
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

#ifdef LMMS_BUILD_WIN32
	if( client_name == midiWinMM::name() || client_name == "" )
	{
		midiWinMM * mwmm = new midiWinMM;
//		if( moss->isRunning() )
		{
			m_midiClientName = midiWinMM::name();
			return( mwmm );
		}
		delete mwmm;
	}
#endif

	printf( "Couldn't create MIDI-client, neither with ALSA nor with "
		"OSS. Will use dummy-MIDI-client.\n" );

	m_midiClientName = midiDummy::name();

	return( new midiDummy );
}









mixer::fifoWriter::fifoWriter( mixer * _mixer, fifo * _fifo ) :
	m_mixer( _mixer ),
	m_fifo( _fifo ),
	m_writing( true )
{
}




void mixer::fifoWriter::finish( void )
{
	m_writing = false;
}




void mixer::fifoWriter::run( void )
{
	const fpp_t frames = m_mixer->framesPerPeriod();
	while( m_writing )
	{
		surroundSampleFrame * buffer = new surroundSampleFrame[frames];
		const surroundSampleFrame * b = m_mixer->renderNextBuffer();
		memcpy( buffer, b, frames * sizeof( surroundSampleFrame ) );
		m_fifo->write( buffer );
	}

	m_fifo->write( NULL );
}




#include "moc_mixer.cxx"

