//
// Created by seledreams on 02/03/2021.
//
#include "VocalInstrumentTrack.h"
#include "VocalPattern.h"
#include "Engine.h"
#include "Song.h"
#include "SamplePlayHandle.h"
#include "VocalInstrument.h"
#include "EffectChain.h"
#include "BBTrack.h"
#include <qdebug.h>

VocalInstrumentTrack::VocalInstrumentTrack(TrackContainer *tc) : InstrumentTrack(tc)
{
}

TrackContentObject *VocalInstrumentTrack::createTCO(const TimePos &pos)
{
	VocalPattern *vp = new VocalPattern(this);
	vp->movePosition(pos);
	connect(vp, &VocalPattern::dataChanged, this, &VocalInstrumentTrack::bounceVocalPattern, Qt::QueuedConnection);
	return vp;
}


bool VocalInstrumentTrack::play(const TimePos & _start, const fpp_t _frames,
								const f_cnt_t _offset, int _tco_num )
{
	m_start = _start;
	m_tco_num = _tco_num;
	audioPort()->effects()->startRunning();
	if( m_tco_num >= 0 )
	{
		if (!getTCOsToPlay()){
			return false;
		}
	}
	else
	{
		bool nowPlaying = false;
		for( int i = 0; i < numOfTCOs(); ++i )
		{
			TrackContentObject * tco = getTCO( i );
			auto * pVocalPattern = dynamic_cast<VocalPattern*>( tco );

			if( _start >= pVocalPattern->startPosition() && _start < pVocalPattern->endPosition() )
			{
				nowPlaying = setupTCO(pVocalPattern);
			}
			else
			{
				pVocalPattern->setIsPlaying(false );
			}
			nowPlaying = nowPlaying || pVocalPattern->isPlaying();
		}
		m_playing = nowPlaying;
	}

	return addPlayHandles(_offset);
}

void VocalInstrumentTrack::bounceVocalPattern()
{
	// Gets the pattern who send the event
	auto thesender = sender();
	if (!thesender)
		return;
	// Only render vocal patterns
	auto *vocalPattern = dynamic_cast<VocalPattern*>(thesender);
	if (!vocalPattern){
		return;
	}
	auto *vocalInstrument = dynamic_cast<VocalInstrument*>(vocalPattern->instrumentTrack()->instrument());
	if (vocalInstrument){
		vocalInstrument->bounceVocalPattern(vocalPattern);

	}
}

bool VocalInstrumentTrack::getTCOsToPlay()
{
	if (m_start > getTCO(m_tco_num)->length())
	{
		m_playing = false;
	}
	if( m_start != 0 )
	{
		return false;
	}
	m_tcos.push_back( getTCO( m_tco_num ) );
	if (trackContainer() == (TrackContainer*)Engine::getBBTrackContainer())
	{
		m_playing = true;
	}
	return true;
}

bool VocalInstrumentTrack::setupTCO(VocalPattern *_vocal_pattern)
{
	if( !_vocal_pattern->isPlaying() && m_start >= (_vocal_pattern->startPosition() + _vocal_pattern->startTimeOffset()) )
	{
		auto bufferFramesPerTick = Engine::framesPerTick (_vocal_pattern->getBouncedBuffer().sampleRate ());
		f_cnt_t sampleStart = bufferFramesPerTick * ( m_start - _vocal_pattern->startPosition() - _vocal_pattern->startTimeOffset() );
		f_cnt_t tcoFrameLength = bufferFramesPerTick * ( _vocal_pattern->endPosition() - _vocal_pattern->startPosition() - _vocal_pattern->startTimeOffset() );
		f_cnt_t sampleBufferLength = _vocal_pattern->getBouncedBuffer().frames();
		//if the Tco smaller than the sample length we play only until Tco end
		//else we play the sample to the end but nothing more
		f_cnt_t samplePlayLength = tcoFrameLength > sampleBufferLength ? sampleBufferLength : tcoFrameLength;
		//we only play within the sampleBuffer limits
		if( sampleStart < sampleBufferLength )
		{
			_vocal_pattern->getBouncedBuffer().setStartFrame(sampleStart);
			_vocal_pattern->getBouncedBuffer().setEndFrame(samplePlayLength);
			m_tcos.push_back( _vocal_pattern );
			_vocal_pattern->setIsPlaying( true );
			return true;
		}
	}
	return false;
}

bool VocalInstrumentTrack::addPlayHandles(const TimePos & _offset)
{
	bool played_a_note;
	for( tcoVector::Iterator it = m_tcos.begin(); it != m_tcos.end(); ++it ) {
		auto *st = dynamic_cast<VocalPattern *>( *it );
		if (!st->isMuted()) {
			PlayHandle *handle;
			auto *smpHandle = new SamplePlayHandle(&st->getBouncedBuffer(),false);
			smpHandle->setAudioPort(audioPort());
			handle = smpHandle;
			handle->setOffset(_offset);

			// send it to the mixer
			Engine::mixer()->addPlayHandle(handle);
			played_a_note = true;
		}
	}
	return played_a_note;
}
