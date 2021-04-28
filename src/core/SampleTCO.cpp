/*
 * SampleTCO.cpp
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
 
#include "SampleTCO.h"

#include <QDomElement>

#include "SampleTCOView.h"
#include "TimeLineWidget.h"

SampleTCO::SampleTCO( Track * _track ) :
	TrackContentObject( _track ),
	m_sampleBuffer( new SampleBuffer ),
	m_isPlaying( false )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( Engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
					this, SLOT( updateLength() ), Qt::DirectConnection );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int,int ) ),
					this, SLOT( updateLength() ) );

	//care about positionmarker
	TimeLineWidget * timeLine = Engine::getSong()->getPlayPos( Engine::getSong()->Mode_PlaySong ).m_timeLine;
	if( timeLine )
	{
		connect( timeLine, SIGNAL( positionMarkerMoved() ), this, SLOT( playbackPositionChanged() ) );
	}
	//playbutton clicked or space key / on Export Song set isPlaying to false
	connect( Engine::getSong(), SIGNAL( playbackStateChanged() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about loops
	connect( Engine::getSong(), SIGNAL( updateSampleTracks() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about mute TCOs
	connect( this, SIGNAL( dataChanged() ), this, SLOT( playbackPositionChanged() ) );
	//care about mute track
	connect( getTrack()->getMutedModel(), SIGNAL( dataChanged() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about TCO position
	connect( this, SIGNAL( positionChanged() ), this, SLOT( updateTrackTcos() ) );

	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::BBContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
	updateTrackTcos();
}

SampleTCO::SampleTCO(const SampleTCO& orig) :
	SampleTCO(orig.getTrack())
{
	// TODO: This creates a new SampleBuffer for the new TCO, eating up memory
	// & eventually causing performance issues. Letting tracks share buffers
	// when they're identical would fix this, but isn't possible right now.
	*m_sampleBuffer = *orig.m_sampleBuffer;
	m_isPlaying = orig.m_isPlaying;
}




SampleTCO::~SampleTCO()
{
	SampleTrack * sampletrack = dynamic_cast<SampleTrack*>( getTrack() );
	if ( sampletrack )
	{
		sampletrack->updateTcos();
	}
	Engine::mixer()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::mixer()->doneChangeInModel();
}




void SampleTCO::changeLength( const TimePos & _length )
{
	TrackContentObject::changeLength( qMax( static_cast<int>( _length ), 1 ) );
}




const QString & SampleTCO::sampleFile() const
{
	return m_sampleBuffer->audioFile();
}



void SampleTCO::setSampleBuffer( SampleBuffer* sb )
{
	Engine::mixer()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::mixer()->doneChangeInModel();
	m_sampleBuffer = sb;
	updateLength();

	emit sampleChanged();
}



void SampleTCO::setSampleFile( const QString & _sf )
{
	int length;
	if ( _sf.isEmpty() )
	{	//When creating an empty sample pattern make it a bar long
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




void SampleTCO::toggleRecord()
{
	m_recordModel.setValue( !m_recordModel.value() );
	emit dataChanged();
}




void SampleTCO::playbackPositionChanged()
{
	Engine::mixer()->removePlayHandlesOfTypes( getTrack(), PlayHandle::TypeSamplePlayHandle );
	SampleTrack * st = dynamic_cast<SampleTrack*>( getTrack() );
	st->setPlayingTcos( false );
}




void SampleTCO::updateTrackTcos()
{
	SampleTrack * sampletrack = dynamic_cast<SampleTrack*>( getTrack() );
	if( sampletrack)
	{
		sampletrack->updateTcos();
	}
}




bool SampleTCO::isPlaying() const
{
	return m_isPlaying;
}




void SampleTCO::setIsPlaying(bool isPlaying)
{
	m_isPlaying = isPlaying;
}




void SampleTCO::updateLength()
{
	emit sampleChanged();
}




TimePos SampleTCO::sampleLength() const
{
	return (int)( m_sampleBuffer->frames() / Engine::framesPerTick() );
}




void SampleTCO::setSampleStartFrame(f_cnt_t startFrame)
{
	m_sampleBuffer->setStartFrame( startFrame );
}




void SampleTCO::setSamplePlayLength(f_cnt_t length)
{
	m_sampleBuffer->setEndFrame( length );
}




void SampleTCO::saveSettings( QDomDocument & _doc, QDomElement & _this )
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




void SampleTCO::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	setSampleFile( _this.attribute( "src" ) );
	if( sampleFile().isEmpty() && _this.hasAttribute( "data" ) )
	{
		m_sampleBuffer->loadFromBase64( _this.attribute( "data" ) );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	setMuted( _this.attribute( "muted" ).toInt() );
	setStartTimeOffset( _this.attribute( "off" ).toInt() );

	if ( _this.hasAttribute( "sample_rate" ) ) {
		m_sampleBuffer->setSampleRate( _this.attribute( "sample_rate" ).toInt() );
	}

	if( _this.hasAttribute( "color" ) )
	{
		useCustomClipColor( true );
		setColor( _this.attribute( "color" ) );
	}

	if(_this.hasAttribute("reversed"))
	{
		m_sampleBuffer->setReversed(true);
		emit wasReversed(); // tell SampleTCOView to update the view
	}
}




TrackContentObjectView * SampleTCO::createView( TrackView * _tv )
{
	return new SampleTCOView( this, _tv );
}