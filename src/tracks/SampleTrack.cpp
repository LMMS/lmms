/*
 * SampleTrack.cpp - implementation of class SampleTrack, a track which
 *                   provides arrangement of samples
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "SampleTrack.h"

#include <QDomElement>

#include "EffectChain.h"
#include "Mixer.h"
#include "panning.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "SampleClip.h"
#include "SamplePlayHandle.h"
#include "SampleRecordHandle.h"
#include "SampleTrackView.h"
#include "Song.h"
#include "volume.h"


namespace lmms
{


SampleTrack::SampleTrack(TrackContainer* tc) :
	Track(Track::Type::Sample, tc),
	m_volumeModel(DefaultVolume, MinVolume, MaxVolume, 0.1f, this, tr("Volume")),
	m_panningModel(DefaultPanning, PanningLeft, PanningRight, 0.1f, this, tr("Panning")),
	m_mixerChannelModel(0, 0, 0, this, tr("Mixer channel")),
	m_audioPort(tr("Sample track"), true, &m_volumeModel, &m_panningModel, &m_mutedModel),
	m_isPlaying(false)
{
	setName(tr("Sample track"));
	m_panningModel.setCenterValue(DefaultPanning);
	m_mixerChannelModel.setRange(0, Engine::mixer()->numChannels()-1, 1);

	connect(&m_mixerChannelModel, SIGNAL(dataChanged()), this, SLOT(updateMixerChannel()));
}




SampleTrack::~SampleTrack()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( this, PlayHandle::Type::SamplePlayHandle );
}




bool SampleTrack::play( const TimePos & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _clip_num )
{
	m_audioPort.effects()->startRunning();
	bool played_a_note = false; // will be return variable


	clipVector clips;
	class PatternTrack * pattern_track = nullptr;
	if( _clip_num >= 0 )
	{
		if (_start > getClip(_clip_num)->length())
		{
			setPlaying(false);
		}
		if( _start != 0 )
		{
			return false;
		}
		clips.push_back( getClip( _clip_num ) );
		if (trackContainer() == Engine::patternStore())
		{
			pattern_track = PatternTrack::findPatternTrack(_clip_num);
			setPlaying(true);
		}
	}
	else
	{
		bool nowPlaying = false;
		for( int i = 0; i < numOfClips(); ++i )
		{
			Clip * clip = getClip( i );
			auto sClip = dynamic_cast<SampleClip*>(clip);

			if( _start >= sClip->startPosition() && _start < sClip->endPosition() )
			{
				if( sClip->isPlaying() == false && _start >= (sClip->startPosition() + sClip->startTimeOffset()) )
				{
					auto bufferFramesPerTick = Engine::framesPerTick(sClip->sample().sampleRate());
					f_cnt_t sampleStart = bufferFramesPerTick * ( _start - sClip->startPosition() - sClip->startTimeOffset() );
					f_cnt_t clipFrameLength = bufferFramesPerTick * ( sClip->endPosition() - sClip->startPosition() - sClip->startTimeOffset() );
					f_cnt_t sampleBufferLength = sClip->sample().sampleSize();
					//if the Clip smaller than the sample length we play only until Clip end
					//else we play the sample to the end but nothing more
					f_cnt_t samplePlayLength = clipFrameLength > sampleBufferLength ? sampleBufferLength : clipFrameLength;
					//we only play within the sampleBuffer limits
					if( sampleStart < sampleBufferLength )
					{
						sClip->setSampleStartFrame( sampleStart );
						sClip->setSamplePlayLength( samplePlayLength );
						clips.push_back( sClip );
						sClip->setIsPlaying( true );
						nowPlaying = true;
					}
				}
			}
			else
			{
				sClip->setIsPlaying( false );
			}
			nowPlaying = nowPlaying || sClip->isPlaying();
		}
		setPlaying(nowPlaying);
	}

	for (const auto& clip : clips)
	{
		auto st = dynamic_cast<SampleClip*>(clip);
		if( !st->isMuted() )
		{
			PlayHandle* handle;
			if( st->isRecord() )
			{
				if( !Engine::getSong()->isRecording() )
				{
					return played_a_note;
				}
				auto smpHandle = new SampleRecordHandle(st);
				handle = smpHandle;
			}
			else
			{
				auto smpHandle = new SamplePlayHandle(st);
				smpHandle->setVolumeModel( &m_volumeModel );
				smpHandle->setPatternTrack(pattern_track);
				handle = smpHandle;
			}
			handle->setOffset( _offset );
			// send it to the audio engine
			Engine::audioEngine()->addPlayHandle( handle );
			played_a_note = true;
		}
	}

	return played_a_note;
}




gui::TrackView * SampleTrack::createView( gui::TrackContainerView* tcv )
{
	return new gui::SampleTrackView( this, tcv );
}




Clip * SampleTrack::createClip(const TimePos & pos)
{
	auto sClip = new SampleClip(this);
	sClip->movePosition(pos);
	return sClip;
}




void SampleTrack::saveTrackSpecificSettings(QDomDocument& _doc, QDomElement& _this, bool presetMode)
{
	m_audioPort.effects()->saveState( _doc, _this );
	m_volumeModel.saveSettings( _doc, _this, "vol" );
	m_panningModel.saveSettings( _doc, _this, "pan" );
	m_mixerChannelModel.saveSettings( _doc, _this, "mixch" );
}




void SampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	m_audioPort.effects()->clear();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_audioPort.effects()->nodeName() == node.nodeName() )
			{
				m_audioPort.effects()->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
	m_volumeModel.loadSettings( _this, "vol" );
	m_panningModel.loadSettings( _this, "pan" );
	m_mixerChannelModel.setRange( 0, Engine::mixer()->numChannels() - 1 );
	m_mixerChannelModel.loadSettings( _this, "mixch" );
}




void SampleTrack::updateClips()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( this, PlayHandle::Type::SamplePlayHandle );
	setPlayingClips( false );
}




void SampleTrack::setPlayingClips( bool isPlaying )
{
	for( int i = 0; i < numOfClips(); ++i )
	{
		Clip * clip = getClip( i );
		auto sClip = dynamic_cast<SampleClip*>(clip);
		sClip->setIsPlaying( isPlaying );
	}
}




void SampleTrack::updateMixerChannel()
{
	m_audioPort.setNextMixerChannel( m_mixerChannelModel.value() );
}


} // namespace lmms
