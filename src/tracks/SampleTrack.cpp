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

#include "BBTrack.h"
#include "SamplePlayHandle.h"
#include "SampleRecordHandle.h"
#include "Song.h"



SampleTrack::SampleTrack(TrackContainer* tc) :
	Track(Track::SampleTrack, tc),
	m_volumeModel(DefaultVolume, MinVolume, MaxVolume, 0.1f, this, tr("Volume")),
	m_panningModel(DefaultPanning, PanningLeft, PanningRight, 0.1f, this, tr("Panning")),
	m_effectChannelModel(0, 0, 0, this, tr("FX channel")),
	m_audioPort(tr("Sample track"), true, &m_volumeModel, &m_panningModel, &m_mutedModel),
	m_isPlaying(false)
{
	setName(tr("Sample track"));
	m_panningModel.setCenterValue(DefaultPanning);
	m_effectChannelModel.setRange(0, Engine::fxMixer()->numChannels()-1, 1);

	connect(&m_effectChannelModel, SIGNAL(dataChanged()), this, SLOT(updateEffectChannel()));
}




SampleTrack::~SampleTrack()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( this, PlayHandle::TypeSamplePlayHandle );
}




bool SampleTrack::play( const TimePos & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _tco_num )
{
	m_audioPort.effects()->startRunning();
	bool played_a_note = false;	// will be return variable

	tcoVector tcos;
	::BBTrack * bb_track = nullptr;
	if( _tco_num >= 0 )
	{
		if (_start > getTCO(_tco_num)->length())
		{
			setPlaying(false);
		}
		if( _start != 0 )
		{
			return false;
		}
		tcos.push_back( getTCO( _tco_num ) );
		if (trackContainer() == (TrackContainer*)Engine::getBBTrackContainer())
		{
			bb_track = BBTrack::findBBTrack( _tco_num );
			setPlaying(true);
		}
	}
	else
	{
		bool nowPlaying = false;
		for( int i = 0; i < numOfTCOs(); ++i )
		{
			TrackContentObject * tco = getTCO( i );
			SampleTCO * sTco = dynamic_cast<SampleTCO*>( tco );

			if( _start >= sTco->startPosition() && _start < sTco->endPosition() )
			{
				if(!sTco->isPlaying() && (_start >= (sTco->startPosition() + sTco->startTimeOffset())
										  || sTco->isRecord()))
				{
					auto bufferFramesPerTick = Engine::framesPerTick (sTco->sampleBuffer ()->sampleRate ());
					f_cnt_t sampleStart = bufferFramesPerTick * ( _start - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t tcoFrameLength = bufferFramesPerTick * ( sTco->endPosition() - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t sampleBufferLength = sTco->sampleBuffer()->frames();
					//if the Tco smaller than the sample length we play only until Tco end
					//else we play the sample to the end but nothing more
					f_cnt_t samplePlayLength = tcoFrameLength > sampleBufferLength ? sampleBufferLength : tcoFrameLength;

					// In case we are recoding, "play" the whole TCO.
					if(sTco->isRecord()) {
                        samplePlayLength = tcoFrameLength;
                    }

                    //we only play within the sampleBuffer limits
                    //Ignore that in case of recoding.
					if(sampleStart < sampleBufferLength || sTco->isRecord ())
					{
						sTco->setSampleStartFrame(sampleStart);
						sTco->setSamplePlayLength(samplePlayLength);
						tcos.push_back(sTco);
						sTco->setIsPlaying(true);
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
		setPlaying(nowPlaying);
	}

	for( tcoVector::Iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		SampleTCO * st = dynamic_cast<SampleTCO *>( *it );
		if( !st->isMuted() )
		{
			PlayHandle* handle;
			if(st->isRecord())
			{
				if( !Engine::getSong()->isRecording() )
				{
					return played_a_note;
				}
				handle = new SampleRecordHandle(st, _start - st->startPosition());
			}
			else
			{
				SamplePlayHandle* smpHandle = new SamplePlayHandle( st );
				smpHandle->setVolumeModel( &m_volumeModel );
				smpHandle->setBBTrack( bb_track );
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




TrackView * SampleTrack::createView( TrackContainerView* tcv )
{
	return new SampleTrackView( this, tcv );
}




TrackContentObject * SampleTrack::createTCO(const TimePos & pos)
{
	SampleTCO * sTco = new SampleTCO(this);
	sTco->movePosition(pos);
	return sTco;
}




void SampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_audioPort.effects()->saveState( _doc, _this );
#if 0
	_this.setAttribute( "icon", tlb->pixmapFile() );
#endif
	m_volumeModel.saveSettings( _doc, _this, "vol" );
	m_panningModel.saveSettings( _doc, _this, "pan" );
	m_effectChannelModel.saveSettings( _doc, _this, "fxch" );
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
	m_effectChannelModel.setRange( 0, Engine::fxMixer()->numChannels() - 1 );
	m_effectChannelModel.loadSettings( _this, "fxch" );
}




void SampleTrack::updateTcos()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( this, PlayHandle::TypeSamplePlayHandle );
	setPlayingTcos( false );
}




void SampleTrack::setPlayingTcos( bool isPlaying )
{
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		TrackContentObject * tco = getTCO( i );
		SampleTCO * sTco = dynamic_cast<SampleTCO*>( tco );
		sTco->setIsPlaying( isPlaying );
	}
}




void SampleTrack::updateEffectChannel()
{
	m_audioPort.setNextFxChannel( m_effectChannelModel.value() );
}