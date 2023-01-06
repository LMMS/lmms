/*
 * SampleClip.cpp
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
 
#include "SampleClip.h"

#include <QDomElement>

#include "SampleBuffer.h"
#include "SampleClipView.h"
#include "SampleTrack.h"
#include "TimeLineWidget.h"


namespace lmms
{


SampleClip::SampleClip( Track * _track ) :
	Clip( _track ),
	m_sampleBuffer( new SampleBuffer ),
	m_isPlaying( false )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this Clip
	connect( Engine::getSong(), SIGNAL(tempoChanged(lmms::bpm_t)),
					this, SLOT(updateLength()), Qt::DirectConnection );
	connect( Engine::getSong(), SIGNAL(timeSignatureChanged(int,int)),
					this, SLOT(updateLength()));

	//care about positionmarker
	gui::TimeLineWidget* timeLine = Engine::getSong()->getPlayPos( Engine::getSong()->Mode_PlaySong ).m_timeLine;
	if( timeLine )
	{
		connect( timeLine, SIGNAL(positionMarkerMoved()), this, SLOT(playbackPositionChanged()));
	}
	//playbutton clicked or space key / on Export Song set isPlaying to false
	connect( Engine::getSong(), SIGNAL(playbackStateChanged()),
			this, SLOT(playbackPositionChanged()), Qt::DirectConnection );
	//care about loops
	connect( Engine::getSong(), SIGNAL(updateSampleTracks()),
			this, SLOT(playbackPositionChanged()), Qt::DirectConnection );
	//care about mute Clips
	connect( this, SIGNAL(dataChanged()), this, SLOT(playbackPositionChanged()));
	//care about mute track
	connect( getTrack()->getMutedModel(), SIGNAL(dataChanged()),
			this, SLOT(playbackPositionChanged()), Qt::DirectConnection );
	//care about Clip position
	connect( this, SIGNAL(positionChanged()), this, SLOT(updateTrackClips()));

	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::PatternContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
	updateTrackClips();
}

SampleClip::SampleClip(const SampleClip& orig) :
	SampleClip(orig.getTrack())
{
	// TODO: This creates a new SampleBuffer for the new Clip, eating up memory
	// & eventually causing performance issues. Letting tracks share buffers
	// when they're identical would fix this, but isn't possible right now.
	*m_sampleBuffer = *orig.m_sampleBuffer;
	m_isPlaying = orig.m_isPlaying;
}




SampleClip::~SampleClip()
{
	auto sampletrack = dynamic_cast<SampleTrack*>(getTrack());
	if ( sampletrack )
	{
		sampletrack->updateClips();
	}
	Engine::audioEngine()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::audioEngine()->doneChangeInModel();
}




void SampleClip::changeLength( const TimePos & _length )
{
	Clip::changeLength( qMax( static_cast<int>( _length ), 1 ) );
}




const QString & SampleClip::sampleFile() const
{
	return m_sampleBuffer->audioFile();
}



void SampleClip::setSampleBuffer( SampleBuffer* sb )
{
	Engine::audioEngine()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::audioEngine()->doneChangeInModel();
	m_sampleBuffer = sb;
	updateLength();

	emit sampleChanged();
}



void SampleClip::setSampleFile( const QString & _sf )
{
	int length;
	if ( _sf.isEmpty() )
	{	//When creating an empty sample clip make it a bar long
		float nom = Engine::getSong()->getTimeSigModel().getNumerator();
		float den = Engine::getSong()->getTimeSigModel().getDenominator();
		length = DefaultTicksPerBar * ( nom / den );
	}
	else
	{	//Otherwise set it to the sample's length
		m_sampleBuffer->setAudioFile( _sf );
		length = sampleLength();
	}
	changeLength(length);

	setStartTimeOffset( 0 );

	emit sampleChanged();
	emit playbackPositionChanged();
}




void SampleClip::toggleRecord()
{
	m_recordModel.setValue( !m_recordModel.value() );
	emit dataChanged();
}




void SampleClip::playbackPositionChanged()
{
	Engine::audioEngine()->removePlayHandlesOfTypes( getTrack(), PlayHandle::TypeSamplePlayHandle );
	auto st = dynamic_cast<SampleTrack*>(getTrack());
	st->setPlayingClips( false );
}




void SampleClip::updateTrackClips()
{
	auto sampletrack = dynamic_cast<SampleTrack*>(getTrack());
	if( sampletrack)
	{
		sampletrack->updateClips();
	}
}




bool SampleClip::isPlaying() const
{
	return m_isPlaying;
}




void SampleClip::setIsPlaying(bool isPlaying)
{
	m_isPlaying = isPlaying;
}




void SampleClip::updateLength()
{
	emit sampleChanged();
}




TimePos SampleClip::sampleLength() const
{
	return (int)( m_sampleBuffer->frames() / Engine::framesPerTick() );
}




void SampleClip::setSampleStartFrame(f_cnt_t startFrame)
{
	m_sampleBuffer->setStartFrame( startFrame );
}




void SampleClip::setSamplePlayLength(f_cnt_t length)
{
	m_sampleBuffer->setEndFrame( length );
}




void SampleClip::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( _this.parentNode().nodeName() == "clipboard" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "len", length() );
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "src", sampleFile() );
	_this.setAttribute( "off", startTimeOffset() );
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
	}

	_this.setAttribute( "sample_rate", m_sampleBuffer->sampleRate());
	if( usesCustomClipColor() )
	{
		_this.setAttribute( "color", color().name() );
	}
	if (m_sampleBuffer->reversed())
	{
		_this.setAttribute("reversed", "true");
	}
	// TODO: start- and end-frame
}




void SampleClip::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	setSampleFile( _this.attribute( "src" ) );
	if( sampleFile().isEmpty() && _this.hasAttribute( "data" ) )
	{
		m_sampleBuffer->loadFromBase64( _this.attribute( "data" ) );
		if (_this.hasAttribute("sample_rate"))
		{
			m_sampleBuffer->setSampleRate(_this.attribute("sample_rate").toInt());
		}
	}
	changeLength( _this.attribute( "len" ).toInt() );
	setMuted( _this.attribute( "muted" ).toInt() );
	setStartTimeOffset( _this.attribute( "off" ).toInt() );

	if( _this.hasAttribute( "color" ) )
	{
		useCustomClipColor( true );
		setColor( _this.attribute( "color" ) );
	}
	else
	{
		useCustomClipColor(false);
	}

	if(_this.hasAttribute("reversed"))
	{
		m_sampleBuffer->setReversed(true);
		emit wasReversed(); // tell SampleClipView to update the view
	}
}




gui::ClipView * SampleClip::createView( gui::TrackView * _tv )
{
	return new gui::SampleClipView( this, _tv );
}


} // namespace lmms
