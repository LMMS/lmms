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
	audioPort()->effects()->startRunning();
	bool played_a_note = false;	// will be return variable

	tcoVector tcos;
	if( _tco_num >= 0 )
	{
		if (_start > getTCO(_tco_num)->length())
		{
			m_playing = false;
		}
		if( _start != 0 )
		{
			return false;
		}
		tcos.push_back( getTCO( _tco_num ) );
		if (trackContainer() == (TrackContainer*)Engine::getBBTrackContainer())
		{
			m_playing = true;
		}
	}
	else
	{
		bool nowPlaying = false;
		for( int i = 0; i < numOfTCOs(); ++i )
		{
			TrackContentObject * tco = getTCO( i );
			VocalPattern * sTco = dynamic_cast<VocalPattern*>( tco );

			if( _start >= sTco->startPosition() && _start < sTco->endPosition() )
			{
				if( sTco->isPlaying() == false && _start >= (sTco->startPosition() + sTco->startTimeOffset()) )
				{
					auto bufferFramesPerTick = Engine::framesPerTick (sTco->getBouncedBuffer().sampleRate ());
					f_cnt_t sampleStart = bufferFramesPerTick * ( _start - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t tcoFrameLength = bufferFramesPerTick * ( sTco->endPosition() - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t sampleBufferLength = sTco->getBouncedBuffer().frames();
					//if the Tco smaller than the sample length we play only until Tco end
					//else we play the sample to the end but nothing more
					f_cnt_t samplePlayLength = tcoFrameLength > sampleBufferLength ? sampleBufferLength : tcoFrameLength;
					//we only play within the sampleBuffer limits
					if( sampleStart < sampleBufferLength )
					{
						sTco->getBouncedBuffer().setStartFrame(sampleStart);
						sTco->getBouncedBuffer().setEndFrame(samplePlayLength);
						tcos.push_back( sTco );
						sTco->setIsPlaying( true );
						nowPlaying = true;
					}
				}
			}
			else
			{
				sTco->setIsPlaying( false );
			}
			nowPlaying = nowPlaying || sTco->isPlaying();
		}
		m_playing = nowPlaying;
	}

	for( tcoVector::Iterator it = tcos.begin(); it != tcos.end(); ++it ) {
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
