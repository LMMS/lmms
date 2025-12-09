/*
 * AudioEngine.cpp - device-independent audio engine for LMMS
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <algorithm>
#include <cstdio>
#include <cassert> 
#include <vector>

#include "AudioEngine.h"

#include "MixHelpers.h"
#include "denormals.h"

#include "lmmsconfig.h"

#include "AudioEngineWorkerThread.h"
#include "AudioBusHandle.h"
#include "Mixer.h"
#include "Song.h"
#include "EnvelopeAndLfoParameters.h"
#include "NotePlayHandle.h"
#include "ConfigManager.h"

// platform-specific audio-interface-classes
#include "AudioAlsa.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioSndio.h"
#include "AudioPortAudio.h"
#include "AudioSoundIo.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioDummy.h"

// platform-specific midi-interface-classes
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"
#include "MidiApple.h"
#include "MidiDummy.h"

#include "BufferManager.h"

#include "LocklessRingBuffer.h"


namespace lmms
{

using LocklessListElement = LocklessList<PlayHandle*>::Element;

static thread_local bool s_renderingThread = false;
static const f_cnt_t FIXED_INPUT_BUFFER_CAPACITY = DEFAULT_BUFFER_SIZE * 100;

AudioEngine::AudioEngine( bool renderOnly ) :
	m_renderOnly( renderOnly ),
	m_framesPerPeriod( DEFAULT_BUFFER_SIZE ),
	m_baseSampleRate(std::max(ConfigManager::inst()->value("audioengine", "samplerate").toInt(), SUPPORTED_SAMPLERATES.front())),
	m_inputBufferRead( 0 ),
	m_inputBufferWrite( 1 ),
	m_outputBufferRead(nullptr),
	m_outputBufferWrite(nullptr),
	m_workers(),
	m_numWorkers( QThread::idealThreadCount()-1 ),
	m_newPlayHandles( PlayHandle::MaxNumber ),
	m_qualitySettings(qualitySettings::Interpolation::Linear),
	m_masterGain( 1.0f ),
	m_audioDev( nullptr ),
	m_oldAudioDev( nullptr ),
	m_audioDevStartFailed( false ),
	m_profiler(),
	m_clearSignal(false)
{
	for( int i = 0; i < 2; ++i )
	{
		m_inputBuffer[i].reserve(FIXED_INPUT_BUFFER_CAPACITY);
	}

	// determine FIFO size and number of frames per period
	int fifoSize = 1;

	// if not only rendering (that is, using the GUI), load the buffer
	// size from user configuration
	if( renderOnly == false )
	{
		m_framesPerPeriod = 
			( fpp_t ) ConfigManager::inst()->value( "audioengine", "framesperaudiobuffer" ).toInt();

		// if the value read from user configuration is not set or
		// lower than the minimum allowed, use the default value and
		// save it to the configuration
		if( m_framesPerPeriod < MINIMUM_BUFFER_SIZE )
		{
			ConfigManager::inst()->setValue( "audioengine",
						"framesperaudiobuffer",
						QString::number( DEFAULT_BUFFER_SIZE ) );

			m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		}
		// lmms works with chunks of size DEFAULT_BUFFER_SIZE (256) and only the final mix will use the actual
		// buffer size. Plugins don't see a larger buffer size than 256. If m_framesPerPeriod is larger than
		// DEFAULT_BUFFER_SIZE, it's set to DEFAULT_BUFFER_SIZE and the rest is handled by an increased fifoSize.
		else if( m_framesPerPeriod > DEFAULT_BUFFER_SIZE )
		{
			fifoSize = m_framesPerPeriod / DEFAULT_BUFFER_SIZE;
			m_framesPerPeriod = DEFAULT_BUFFER_SIZE;
		}
	}

	// allocte the FIFO from the determined size
	m_fifo = new Fifo( fifoSize );

	// now that framesPerPeriod is fixed initialize global BufferManager
	BufferManager::init( m_framesPerPeriod );

	m_outputBufferRead = std::make_unique<SampleFrame[]>(m_framesPerPeriod);
	m_outputBufferWrite = std::make_unique<SampleFrame[]>(m_framesPerPeriod);

	if (m_numWorkers < 0) { m_numWorkers = 0; }

	for (int i = 0; i < m_numWorkers + 1; ++i)
	{
		auto workerThread = new AudioEngineWorkerThread(this);
		if( i < m_numWorkers )
		{
			workerThread->start(QThread::TimeCriticalPriority);
		}
		m_workers.push_back(workerThread);
	}

	m_inputAudioRingBuffer = std::make_unique<LocklessRingBuffer<SampleFrame>>(FIXED_INPUT_BUFFER_CAPACITY);
	m_inputAudioRingBufferReader = std::make_unique<LocklessRingBufferReader<SampleFrame>>(*m_inputAudioRingBuffer);
}




AudioEngine::~AudioEngine()
{
	if (m_audioDev && m_audioDev->isProcessing())
	{
		stopProcessing();
	}
	else if (m_fifoWriter)
	{
		m_fifoWriter->finish();
		m_fifoWriter->wait();
		delete m_fifoWriter;
		m_fifoWriter = nullptr;
	}

	for( int w = 0; w < m_numWorkers; ++w )
	{
		if (m_workers[w]) { m_workers[w]->quit(); }
	}

	AudioEngineWorkerThread::startAndWaitForJobs();

	for (int w = 0; w < m_numWorkers; ++w)
	{
		if (m_workers[w])
		{
			if (!m_workers[w]->wait(500))
			{
				fprintf(stderr, "AudioEngine: Worker thread %d did not finish gracefully after quit() and wait().\n", w);
			}
			delete m_workers[w];
			m_workers[w] = nullptr;
		}
	}

	if (m_numWorkers >= 0 && std::cmp_less(m_numWorkers, m_workers.size()) && m_workers[m_numWorkers])
	{
		delete m_workers[m_numWorkers];
		m_workers[m_numWorkers] = nullptr;
	}
	m_workers.clear();

	if (m_fifo)
	{
		while (m_fifo->available())
		{
			SampleFrame* bufToDelete = m_fifo->read();
			if (bufToDelete) { delete[] bufToDelete; }
		}
		delete m_fifo;
		m_fifo = nullptr;
	}

	delete m_midiClient;
	m_midiClient = nullptr;
	delete m_audioDev;
	m_audioDev = nullptr;

	if (m_oldAudioDev)
	{
		delete m_oldAudioDev;
	}
	m_oldAudioDev = nullptr;

	// Input buffers are std::vectors and clean up automatically
}




void AudioEngine::initDevices()
{
	bool success_ful = false;
	if( m_renderOnly ) {
		m_audioDev = new AudioDummy( success_ful, this );
		m_audioDevName = AudioDummy::name();
		m_midiClient = new MidiDummy;
		m_midiClientName = MidiDummy::name();
	} else {
		m_audioDev = tryAudioDevices();
		m_midiClient = tryMidiClients();
	}
	// Loading audio device may have changed the sample rate
	emit sampleRateChanged();
}




void AudioEngine::startProcessing(bool needsFifo)
{
	if (needsFifo)
	{
		m_fifoWriter = new fifoWriter( this, m_fifo );
		m_fifoWriter->start( QThread::HighPriority );
	}
	else
	{
		m_fifoWriter = nullptr;
	}

	m_audioDev->startProcessing();
}




void AudioEngine::stopProcessing()
{
	if( m_fifoWriter != nullptr )
	{
		m_fifoWriter->finish();
		m_fifoWriter->wait();
		m_audioDev->stopProcessing();
		delete m_fifoWriter;
		m_fifoWriter = nullptr;
	}
	else
	{
		m_audioDev->stopProcessing();
	}
}




bool AudioEngine::criticalXRuns() const
{
	return cpuLoad() >= 99 && Engine::getSong()->isExporting() == false;
}




void AudioEngine::pushInputFrames(SampleFrame* inputAudioBlock, const f_cnt_t frameCount)
{
	if (!m_inputAudioRingBuffer || frameCount == 0) return;

	std::size_t availableSpace = m_inputAudioRingBuffer->free();

	if (availableSpace == 0)
	{
		fprintf(stderr, "AudioEngine: Critical input ring buffer overflow, %u frames dropped.\n", static_cast<unsigned int>(frameCount));
		return;
	}

	size_t framesToWrite = frameCount;
	if (availableSpace < frameCount)
	{
		framesToWrite = availableSpace;
		fprintf(stderr, "AudioEngine: Partial input ring buffer overflow, %zu of %u frames will be written, %zu frames dropped.\n",
				 framesToWrite, static_cast<unsigned int>(frameCount), frameCount - framesToWrite);
	}

	std::size_t framesWritten = m_inputAudioRingBuffer->write(inputAudioBlock, framesToWrite, false);

	if (framesWritten < framesToWrite)
	{
		fprintf(stderr, "AudioEngine: Ring buffer write issue, %zu of %zu frames written (intended to write %zu).\n",
				 framesWritten, frameCount, framesToWrite);
	}
}



void AudioEngine::processBufferedInputFrames()
{
	if (!m_inputAudioRingBufferReader || !m_inputAudioRingBuffer) return;

	std::size_t availableInRing = m_inputAudioRingBufferReader->read_space();
	if (availableInRing == 0) return;

	const std::size_t maxFramesPerCall = DEFAULT_BUFFER_SIZE * 2; 
	std::size_t framesToProcess = std::min(availableInRing, maxFramesPerCall);

	ringbuffer_reader_t<SampleFrame>::read_sequence_t sequence = m_inputAudioRingBufferReader->read_max(framesToProcess);
	
	std::size_t framesInSequence = sequence.size();

	if (framesInSequence == 0) return;

	requestChangeInModel();

	auto& currentWriteBuf = m_inputBuffer[m_inputBufferWrite];
	f_cnt_t currentWriteBufCapacity = currentWriteBuf.capacity();
	f_cnt_t currentFramesInWriteBuf = currentWriteBuf.size();

	f_cnt_t totalFramesSuccessfullyCopiedToMain = 0;

	std::size_t len1 = sequence.first_half_size();
	std::size_t len2 = sequence.second_half_size();

	f_cnt_t spaceLeftInMainBuffer{(currentFramesInWriteBuf >= currentWriteBufCapacity)
	? 0 : currentWriteBufCapacity - currentFramesInWriteBuf};

	f_cnt_t framesWeCanActuallyCopy = static_cast<f_cnt_t>(framesInSequence);
	framesWeCanActuallyCopy = framesWeCanActuallyCopy > spaceLeftInMainBuffer ?
		spaceLeftInMainBuffer : framesWeCanActuallyCopy;

	if (len1 > 0 && totalFramesSuccessfullyCopiedToMain < framesWeCanActuallyCopy)
	{
		f_cnt_t framesToCopyFromPart1 = std::min(static_cast<f_cnt_t>(len1),
		framesWeCanActuallyCopy - totalFramesSuccessfullyCopiedToMain);
		if (framesToCopyFromPart1 > 0)
		{
			currentWriteBuf.resize(currentFramesInWriteBuf + totalFramesSuccessfullyCopiedToMain + framesToCopyFromPart1);
			memcpy(&currentWriteBuf[currentFramesInWriteBuf + totalFramesSuccessfullyCopiedToMain],
				sequence.first_half_ptr(), framesToCopyFromPart1 * sizeof(SampleFrame));
			totalFramesSuccessfullyCopiedToMain += framesToCopyFromPart1;
		}
	}

	if (len2 > 0 && totalFramesSuccessfullyCopiedToMain < framesWeCanActuallyCopy)
	{
		f_cnt_t framesToCopyFromPart2 = std::min(static_cast<f_cnt_t>(len2),
		framesWeCanActuallyCopy - totalFramesSuccessfullyCopiedToMain);
		if (framesToCopyFromPart2 > 0)
		{
			currentWriteBuf.resize(currentFramesInWriteBuf + totalFramesSuccessfullyCopiedToMain + framesToCopyFromPart2);
			memcpy(&currentWriteBuf[currentFramesInWriteBuf + totalFramesSuccessfullyCopiedToMain],
				sequence.second_half_ptr(), framesToCopyFromPart2 * sizeof(SampleFrame));
			totalFramesSuccessfullyCopiedToMain += framesToCopyFromPart2;
		}
	}
	
	
	doneChangeInModel();
}





void AudioEngine::renderStageNoteSetup()
{
	AudioEngineProfiler::Probe profilerProbe(m_profiler, AudioEngineProfiler::DetailType::NoteSetup);

	processBufferedInputFrames();

	if( m_clearSignal )
	{
		m_clearSignal = false;
		clearInternal();
	}

	// remove all play-handles that have to be deleted and delete
	// them if they still exist...
	// maybe this algorithm could be optimized...
	ConstPlayHandleList::Iterator it_rem = m_playHandlesToRemove.begin();
	while( it_rem != m_playHandlesToRemove.end() )
	{
		PlayHandleList::Iterator it = std::find( m_playHandles.begin(), m_playHandles.end(), *it_rem );
		bool removedFromPlayHandles = false;
		if( it != m_playHandles.end() )
		{
			if ((*it)->audioBusHandle()) { (*it)->audioBusHandle()->removePlayHandle(*it); }
			if((*it)->type() == PlayHandle::Type::NotePlayHandle)
			{
				NotePlayHandleManager::release((NotePlayHandle*)*it);
			}
			else { delete *it; }
			it = m_playHandles.erase(it);
			removedFromPlayHandles = true;
		}
		
		if (removedFromPlayHandles)
		{
			it_rem = m_playHandlesToRemove.erase(it_rem); 
		}
		else
		{
			++it_rem;
		}
	}

	swapBuffers();

	// prepare master mix (clear internal buffers etc.)
	Mixer * mixer = Engine::mixer();
	mixer->prepareMasterMix();

	// create play-handles for new notes, samples etc.
	Engine::getSong()->processNextBuffer();

	// add all play-handles that have to be added
	for( LocklessListElement * e = m_newPlayHandles.popList(); e; )
	{
		m_playHandles += e->value;
		LocklessListElement * next = e->next;
		m_newPlayHandles.free( e );
		e = next;
	}
}



void AudioEngine::renderStageInstruments()
{
	AudioEngineProfiler::Probe profilerProbe(m_profiler, AudioEngineProfiler::DetailType::Instruments);

	AudioEngineWorkerThread::fillJobQueue(m_playHandles);
	AudioEngineWorkerThread::startAndWaitForJobs();
}



void AudioEngine::renderStageEffects()
{
	AudioEngineProfiler::Probe profilerProbe(m_profiler, AudioEngineProfiler::DetailType::Effects);

	// STAGE 2: process effects of all instrument- and sampletracks
	AudioEngineWorkerThread::fillJobQueue(m_audioBusHandles);
	AudioEngineWorkerThread::startAndWaitForJobs();

	// removed all play handles which are done
	for( PlayHandleList::Iterator it = m_playHandles.begin();
						it != m_playHandles.end(); )
	{
		if( ( *it )->affinityMatters() &&
			( *it )->affinity() != QThread::currentThread() )
		{
			++it;
			continue;
		}
		if( ( *it )->isFinished() )
		{
			if ((*it)->audioBusHandle()) { (*it)->audioBusHandle()->removePlayHandle(*it); }
			if((*it)->type() == PlayHandle::Type::NotePlayHandle)
			{
				NotePlayHandleManager::release((NotePlayHandle*)*it);
			}
			else delete *it;
			it = m_playHandles.erase(it);
		}
		else
		{
			++it;
		}
	}
}



void AudioEngine::renderStageMix()
{
	AudioEngineProfiler::Probe profilerProbe(m_profiler, AudioEngineProfiler::DetailType::Mixing);

	Mixer *mixer = Engine::mixer();
	mixer->masterMix(m_outputBufferWrite.get());

	MixHelpers::multiply(m_outputBufferWrite.get(), m_masterGain, m_framesPerPeriod);

	emit nextAudioBuffer(m_outputBufferRead.get());

	// and trigger LFOs
	EnvelopeAndLfoParameters::instances()->trigger();
	Controller::triggerFrameCounter();
	AutomatableModel::incrementPeriodCounter();
}



const SampleFrame* AudioEngine::renderNextBuffer()
{
	const auto lock = std::lock_guard{m_changeMutex};

	m_profiler.startPeriod();
	s_renderingThread = true;

	renderStageNoteSetup();     // STAGE 0: clear old play handles and buffers, setup new play handles
	renderStageInstruments();   // STAGE 1: run and render all play handles
	renderStageEffects();       // STAGE 2: process effects of all instrument- and sampletracks
	renderStageMix();           // STAGE 3: do master mix in mixer

	s_renderingThread = false;
	m_profiler.finishPeriod(outputSampleRate(), m_framesPerPeriod);

	return m_outputBufferRead.get();
}




void AudioEngine::swapBuffers()
{
	m_inputBufferWrite = (m_inputBufferWrite + 1) % 2;
	m_inputBufferRead = (m_inputBufferRead + 1) % 2;
	m_inputBuffer[m_inputBufferWrite].clear();

	std::swap(m_outputBufferRead, m_outputBufferWrite);
	zeroSampleFrames(m_outputBufferWrite.get(), m_framesPerPeriod);
}

void AudioEngine::clear()
{
	m_clearSignal = true;
}




void AudioEngine::clearNewPlayHandles()
{
	requestChangeInModel();
	for( LocklessListElement * e = m_newPlayHandles.popList(); e; )
	{
		LocklessListElement * next = e->next;
		m_newPlayHandles.free( e );
		e = next;
	}
	doneChangeInModel();
}



// removes all play-handles. this is necessary, when the song is stopped ->
// all remaining notes etc. would be played until their end
void AudioEngine::clearInternal()
{
	// TODO: m_midiClient->noteOffAll();
	for (auto ph : m_playHandles)
	{
		if (ph->type() != PlayHandle::Type::InstrumentPlayHandle)
		{
			m_playHandlesToRemove.push_back(ph);
		}
	}
}




void AudioEngine::changeQuality(const struct qualitySettings & qs)
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = qs;

	emit sampleRateChanged();
	emit qualitySettingsChanged();

	startProcessing();
}




void AudioEngine::doSetAudioDevice( AudioDevice * _dev )
{
	// TODO: Use shared_ptr here in the future.
	// Currently, this is safe, because this is only called by
	// ProjectRenderer, and after ProjectRenderer calls this function,
	// it does not access the old device anymore.
	if( m_audioDev != m_oldAudioDev ) {delete m_audioDev;}

	if( _dev )
	{
		m_audioDev = _dev;
	}
	else
	{
		printf( "param _dev == NULL in AudioEngine::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
}




void AudioEngine::setAudioDevice(AudioDevice * _dev,
				const struct qualitySettings & _qs,
				bool _needs_fifo,
				bool startNow)
{
	stopProcessing();

	m_qualitySettings = _qs;

	doSetAudioDevice( _dev );

	emit qualitySettingsChanged();
	emit sampleRateChanged();

	if (startNow) {startProcessing( _needs_fifo );}
}




void AudioEngine::storeAudioDevice()
{
	if( !m_oldAudioDev )
	{
		m_oldAudioDev = m_audioDev;
	}
}




void AudioEngine::restoreAudioDevice()
{
	if( m_oldAudioDev && m_audioDev != m_oldAudioDev )
	{
		stopProcessing();
		delete m_audioDev;

		m_audioDev = m_oldAudioDev;
		emit sampleRateChanged();

		startProcessing();
	}
	m_oldAudioDev = nullptr;
}


bool AudioEngine::captureDeviceAvailable() const
{
	return audioDev()->supportsCapture();
}


void AudioEngine::removeAudioBusHandle(AudioBusHandle* busHandle)
{
	requestChangeInModel();

	auto it = std::find(m_audioBusHandles.begin(), m_audioBusHandles.end(), busHandle);
	if (it != m_audioBusHandles.end())
	{
		m_audioBusHandles.erase(it);
	}
	doneChangeInModel();
}


bool AudioEngine::addPlayHandle( PlayHandle* handle )
{
	// Only add play handles if we have the CPU capacity to process them.
	// Instrument play handles are not added during playback, but when the
	// associated instrument is created, so add those unconditionally.
	if (handle->type() == PlayHandle::Type::InstrumentPlayHandle || !criticalXRuns())
	{
		m_newPlayHandles.push( handle );
		if (handle->audioBusHandle()) { handle->audioBusHandle()->addPlayHandle(handle); }
		return true;
	}

	if( handle->type() == PlayHandle::Type::NotePlayHandle )
	{
		NotePlayHandleManager::release( (NotePlayHandle*)handle );
	}
	else delete handle;

	return false;
}


void AudioEngine::removePlayHandle(PlayHandle * ph)
{
	requestChangeInModel();
	// check thread affinity as we must not delete play-handles
	// which were created in a thread different than the audio engine thread
	if (ph->affinityMatters() && ph->affinity() == QThread::currentThread())
	{
		if (ph->audioBusHandle()) { ph->audioBusHandle()->removePlayHandle(ph); }
		bool removedFromList = false;
		// Check m_newPlayHandles first because doing it the other way around
		// creates a race condition
		for( LocklessListElement * e = m_newPlayHandles.first(),
				* ePrev = nullptr; e; ePrev = e, e = e->next )
		{
			if (e->value == ph)
			{
				if( ePrev )
				{
					ePrev->next = e->next;
				}
				else
				{
					m_newPlayHandles.setFirst( e->next );
				}
				m_newPlayHandles.free( e );
				removedFromList = true;
				break;
			}
		}
		// Now check m_playHandles
		PlayHandleList::Iterator it = std::find(m_playHandles.begin(), m_playHandles.end(), ph);
		if (it != m_playHandles.end())
		{
			m_playHandles.erase(it);
			removedFromList = true;
		}
		// Only deleting PlayHandles that were actually found in the list
		// "fixes crash when previewing a preset under high load"
		// (See tobydox's 2008 commit 4583e48)
		if ( removedFromList )
		{
			if (ph->type() == PlayHandle::Type::NotePlayHandle)
			{
				NotePlayHandleManager::release(dynamic_cast<NotePlayHandle*>(ph));
			}
			else { delete ph; }
		}
	}
	else
	{
		m_playHandlesToRemove.push_back(ph);
	}
	doneChangeInModel();
}




void AudioEngine::removePlayHandlesOfTypes(Track * track, PlayHandle::Types types)
{
	requestChangeInModel();
	PlayHandleList::Iterator it = m_playHandles.begin();
	while( it != m_playHandles.end() )
	{
		if ((*it)->isFromTrack(track) && ((*it)->type() & types))
		{
			if ((*it)->audioBusHandle()) { (*it)->audioBusHandle()->removePlayHandle(*it); }
			if((*it)->type() == PlayHandle::Type::NotePlayHandle)
			{
				NotePlayHandleManager::release((NotePlayHandle*)*it);
			}
			else delete *it;
			it = m_playHandles.erase(it);
		}
		else
		{
			++it;
		}
	}
	doneChangeInModel();
}




void AudioEngine::requestChangeInModel()
{
	if (s_renderingThread) { return; }
	m_changeMutex.lock();
}

void AudioEngine::doneChangeInModel()
{
	if (s_renderingThread) { return; }
	m_changeMutex.unlock();
}

bool AudioEngine::isAudioDevNameValid(QString name)
{
#ifdef LMMS_HAVE_SDL
	if (name == AudioSdl::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_ALSA
	if (name == AudioAlsa::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if (name == AudioPulseAudio::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if (name == AudioOss::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if (name == AudioSndio::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if (name == AudioJack::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if (name == AudioPortAudio::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_SOUNDIO
	if (name == AudioSoundIo::name())
	{
		return true;
	}
#endif

	if (name == AudioDummy::name())
	{
		return true;
	}

	return false;
}

bool AudioEngine::isMidiDevNameValid(QString name)
{
#ifdef LMMS_HAVE_ALSA
	if (name == MidiAlsaSeq::name() || name == MidiAlsaRaw::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if (name == MidiJack::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_OSS
	if (name == MidiOss::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if (name == MidiSndio::name())
	{
		return true;
	}
#endif

#ifdef LMMS_BUILD_WIN32
	if (name == MidiWinMM::name())
	{
		return true;
	}
#endif

#ifdef LMMS_BUILD_APPLE
	if (name == MidiApple::name())
	{
		return true;
	}
#endif

	if (name == MidiDummy::name())
	{
		return true;
	}

	return false;
}

AudioDevice * AudioEngine::tryAudioDevices()
{
	bool success_ful = false;
	AudioDevice * dev = nullptr;
	QString dev_name = ConfigManager::inst()->value( "audioengine", "audiodev" );
	if( !isAudioDevNameValid( dev_name ) )
	{
		dev_name = "";
	}

	m_audioDevStartFailed = false;

#ifdef LMMS_HAVE_SDL
	if( dev_name == AudioSdl::name() || dev_name == "" )
	{
		dev = new AudioSdl( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSdl::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_ALSA
	if( dev_name == AudioAlsa::name() || dev_name == "" )
	{
		dev = new AudioAlsa( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioAlsa::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if( dev_name == AudioPulseAudio::name() || dev_name == "" )
	{
		dev = new AudioPulseAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPulseAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if( dev_name == AudioOss::name() || dev_name == "" )
	{
		dev = new AudioOss( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioOss::name();
			return dev;
		}
		delete dev;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( dev_name == AudioSndio::name() || dev_name == "" )
	{
		dev = new AudioSndio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSndio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_JACK
	if( dev_name == AudioJack::name() || dev_name == "" )
	{
		dev = new AudioJack( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioJack::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if( dev_name == AudioPortAudio::name() || dev_name == "" )
	{
		dev = new AudioPortAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPortAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_SOUNDIO
	if( dev_name == AudioSoundIo::name() || dev_name == "" )
	{
		dev = new AudioSoundIo( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSoundIo::name();
			return dev;
		}
		delete dev;
	}
#endif


	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, this );
	//if( sucess_ful )
	//{
	//	return dev;
	//}
	//delete dev

	if( dev_name != AudioDummy::name() )
	{
		printf( "No audio-driver working - falling back to dummy-audio-"
			"driver\nYou can render your songs and listen to the output "
			"files...\n" );

		m_audioDevStartFailed = true;
	}

	m_audioDevName = AudioDummy::name();

	return new AudioDummy( success_ful, this );
}




MidiClient * AudioEngine::tryMidiClients()
{
	QString client_name = ConfigManager::inst()->value( "audioengine", "mididev" );
	if( !isMidiDevNameValid( client_name ) )
	{
		client_name = "";
	}

#ifdef LMMS_HAVE_ALSA
	if( client_name == MidiAlsaSeq::name() || client_name == "" )
	{
		auto malsas = new MidiAlsaSeq;
		if( malsas->isRunning() )
		{
			m_midiClientName = MidiAlsaSeq::name();
			return malsas;
		}
		delete malsas;
	}

	if( client_name == MidiAlsaRaw::name() || client_name == "" )
	{
		auto malsar = new MidiAlsaRaw;
		if( malsar->isRunning() )
		{
			m_midiClientName = MidiAlsaRaw::name();
			return malsar;
		}
		delete malsar;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if( client_name == MidiJack::name() || client_name == "" )
	{
		auto mjack = new MidiJack;
		if( mjack->isRunning() )
		{
			m_midiClientName = MidiJack::name();
			return mjack;
		}
		delete mjack;
	}
#endif

#ifdef LMMS_HAVE_OSS
	if( client_name == MidiOss::name() || client_name == "" )
	{
		auto moss = new MidiOss;
		if( moss->isRunning() )
		{
			m_midiClientName = MidiOss::name();
			return moss;
		}
		delete moss;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( client_name == MidiSndio::name() || client_name == "" )
	{
		MidiSndio * msndio = new MidiSndio;
		if( msndio->isRunning() )
		{
			m_midiClientName = MidiSndio::name();
			return msndio;
		}
		delete msndio;
	}
#endif

#ifdef LMMS_BUILD_WIN32
	if( client_name == MidiWinMM::name() || client_name == "" )
	{
		MidiWinMM * mwmm = new MidiWinMM;
//		if( moss->isRunning() )
		{
			m_midiClientName = MidiWinMM::name();
			return mwmm;
		}
		delete mwmm;
	}
#endif

#ifdef LMMS_BUILD_APPLE
	printf( "trying midi apple...\n" );
	if( client_name == MidiApple::name() || client_name == "" )
	{
		MidiApple * mapple = new MidiApple;
		m_midiClientName = MidiApple::name();
		printf( "Returning midi apple\n" );
		return mapple;
	}
	printf( "midi apple didn't work: client_name=%s\n", client_name.toUtf8().constData());
#endif

	if(client_name != MidiDummy::name())
	{
		if (client_name.isEmpty())
		{
			printf("Unknown MIDI-client. ");
		}
		else
		{
			printf("Couldn't create %s MIDI-client. ", client_name.toUtf8().constData());
		}
		printf("Will use dummy-MIDI-client.\n");
	}

	m_midiClientName = MidiDummy::name();

	return new MidiDummy;
}









AudioEngine::fifoWriter::fifoWriter( AudioEngine* audioEngine, Fifo * fifo ) :
	m_audioEngine( audioEngine ),
	m_fifo( fifo ),
	m_writing( true )
{
	setObjectName("AudioEngine::fifoWriter");
}




void AudioEngine::fifoWriter::finish()
{
	m_writing = false;
}




void AudioEngine::fifoWriter::run()
{
	disable_denormals();

	const fpp_t frames = m_audioEngine->framesPerPeriod();
	while( m_writing )
	{
		auto buffer = new SampleFrame[frames];
		const SampleFrame* b = m_audioEngine->renderNextBuffer();
		memcpy(buffer, b, frames * sizeof(SampleFrame));
		m_fifo->write(buffer);
	}

	// Let audio backend stop processing
	m_fifo->write(nullptr);
	m_fifo->waitUntilRead();
}

} // namespace lmms
