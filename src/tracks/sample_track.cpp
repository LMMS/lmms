#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_track.cpp - implementation of class sampleTrack, a track which
 *                    provides arrangement of samples
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <Qt/QtXml>
#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>

#include "effect_label.h"
#include "sample_track.h"
#include "song_editor.h"
#include "embed.h"
#include "engine.h"
#include "templates.h"
#include "tooltip.h"
#include "audio_port.h"
#include "automation_pattern.h"
#include "sample_play_handle.h"
#include "string_pair_drag.h"
#include "volume.h"
#include "volume_knob.h"


sampleTCO::sampleTCO( track * _track ) :
	trackContentObject( _track ),
	m_sampleBuffer( new sampleBuffer )
{
	saveJournallingState( FALSE );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( engine::getSongEditor(), SIGNAL( tempoChanged( bpm_t ) ), this,
						SLOT( updateLength( bpm_t ) ) );
}




sampleTCO::~sampleTCO()
{
	sharedObject::unref( m_sampleBuffer );
}




void sampleTCO::changeLength( const midiTime & _length )
{
	trackContentObject::changeLength( tMax( static_cast<Sint32>( _length ),
									64 ) );
}




const QString & sampleTCO::sampleFile( void ) const
{
	return( m_sampleBuffer->audioFile() );
}




void sampleTCO::setSampleFile( const QString & _sf )
{
	m_sampleBuffer->setAudioFile( _sf );
	updateLength();
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	toolTip::add( this, ( m_sampleBuffer->audioFile() != "" ) ?
					m_sampleBuffer->audioFile() :
					tr( "double-click to select sample" ) );
}




void sampleTCO::updateLength( bpm_t )
{
	changeLength( getSampleLength() );
}




void sampleTCO::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == FALSE )
	{
		trackContentObject::dragEnterEvent( _dee );
	}
}




void sampleTCO::dropEvent( QDropEvent * _de )
{
	if( stringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		setSampleFile( stringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( stringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_sampleBuffer->loadFromBase64(
					stringPairDrag::decodeValue( _de ) );
		engine::getSongEditor()->setModified();
		updateLength();
		update();
		_de->accept();
	}
	else
	{
		trackContentObject::dropEvent( _de );
	}
}




void sampleTCO::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_sampleBuffer->openAudioFile();
	if( af != "" && af != m_sampleBuffer->audioFile() )
	{
		setSampleFile( af );
		engine::getSongEditor()->setModified();
	}
}




void sampleTCO::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	QLinearGradient grad( 0, 0, 0, height() );
	if( isSelected() )
	{
		grad.setColorAt( 1, QColor( 0, 0, 224 ) );
		grad.setColorAt( 0, QColor( 0, 0, 128 ) );
	}
	else
	{
		grad.setColorAt( 0, QColor( 96, 96, 96 ) );
		grad.setColorAt( 1, QColor( 16, 16, 16 ) );
	}
	p.fillRect( _pe->rect(), grad );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( 0, 0, width()-1, height()-1 );
	if( getTrack()->muted() || muted() )
	{
		p.setPen( QColor( 128, 128, 128 ) );
	}
	else
	{
		p.setPen( QColor( 64, 224, 160 ) );
	}
	QRect r = QRect( 1, 1,
			tMax( static_cast<int>( getSampleLength() *
				pixelsPerTact() / 64 ), 1 ), height() - 4 );
	p.setClipRect( QRect( 1, 1, width() - 2, height() - 2 ) );
	m_sampleBuffer->visualize( p, r, _pe->rect() );
	if( r.width() < width() - 1 )
	{
		p.drawLine( r.x() + r.width(), r.y() + r.height() / 2,
				width() - 2, r.y() + r.height() / 2 );
	}

	p.translate( 0, 0 );
	if( muted() )
	{
		p.drawPixmap( 3, 8, embed::getIconPixmap( "muted", 16, 16 ) );
	}
}




midiTime sampleTCO::getSampleLength( void ) const
{
	return( static_cast<Sint32>( m_sampleBuffer->frames() /
						engine::framesPerTact64th() ) );
}




void FASTCALL sampleTCO::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
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
	_this.setAttribute( "muted", muted() );
	_this.setAttribute( "src", sampleFile() );
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
	}
	// TODO: start- and end-frame
}




void FASTCALL sampleTCO::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	setSampleFile( _this.attribute( "src" ) );
	if( sampleFile() == "" )
	{
		m_sampleBuffer->loadFromBase64( _this.attribute( "data" ) );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	if( _this.attribute( "muted" ).toInt() != muted() )
	{
		toggleMute();
	}
}





/*

sampleTCOSettingsDialog::sampleTCOSettingsDialog( sampleTCO * _stco ) :
	QDialog(),
	m_sampleTCO( _stco )
{
	resize( 400, 300 );

	QVBoxWidget * vb0 = new QVBoxWidget( this );
	vb0->resize( 400, 300 );
	QHBoxWidget * hb00 = new QHBoxWidget( vb0 );
	m_fileLbl = new QLabel( _stco->sampleFile(), hb00 );
	QPushButton * open_file_btn = new QPushButton(
				embed::getIconPixmap( "fileopen" ), "", hb00 );
	connect( open_file_btn, SIGNAL( clicked() ), this,
						SLOT( openSampleFile() ) );

	QHBoxWidget * hb01 = new QHBoxWidget( vb0 );

	QPushButton * ok_btn = new QPushButton( tr( "OK" ), hb01 );
	ok_btn->setGeometry( 10, 0, 100, 32 );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( accept() ) );

	QPushButton * cancel_btn = new QPushButton( tr( "Cancel" ), hb01 );
	cancel_btn->setGeometry( 120, 0, 100, 32 );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( reject() ) );
	
}




sampleTCOSettingsDialog::~sampleTCOSettingsDialog()
{
}




void sampleTCOSettingsDialog::openSampleFile( void )
{
	QString af = m_sampleTCO->m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		setSampleFile( af );
	}
}




void sampleTCOSettingsDialog::setSampleFile( const QString & _f )
{
	m_fileLbl->setText( _f );
	m_sampleTCO->setSampleFile( _f );
	engine::getSongEditor()->setModified();
}
*/





sampleTrack::sampleTrack( trackContainer * _tc ) :
	track( _tc ),
	m_audioPort( new audioPort( tr( "Sample track" ) ) )
{
	getTrackWidget()->setFixedHeight( 32 );

	m_trackLabel = new effectLabel( tr( "Sample track" ),
					      getTrackSettingsWidget(), this );
#if 0
	m_trackLabel = new nameLabel( tr( "Sample track" ),
						getTrackSettingsWidget() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "sample_track" ) );
#endif
	m_trackLabel->setGeometry( 26, 1, DEFAULT_SETTINGS_WIDGET_WIDTH-2, 29 );
	m_trackLabel->show();

	m_volumeKnob = new volumeKnob( knobSmall_17, getTrackSettingsWidget(),
					    tr( "Channel volume" ), this );
	m_volumeKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
	m_volumeKnob->setInitValue( DEFAULT_VOLUME );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->move( 4, 4 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();
	m_volumeKnob->setWhatsThis(
		tr( "With this knob you can set "
			"the volume of the opened "
			"channel." ) ); 
}




sampleTrack::~sampleTrack()
{
	// disconnect sampleTCOs
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		disconnect( engine::getSongEditor(), 0, getTCO( i ), 0 );
	}

	engine::getMixer()->removePlayHandles( this );
	delete m_audioPort;
}




track::trackTypes sampleTrack::type( void ) const
{
	return( SAMPLE_TRACK );
}




bool FASTCALL sampleTrack::play( const midiTime & _start,
						const fpp_t _frames,
						const f_cnt_t _offset,
							Sint16 /*_tco_num*/ )
{
	sendMidiTime( _start );

	m_audioPort->getEffects()->startRunning();
	bool played_a_note = FALSE;	// will be return variable

	for( int i = 0; i < numOfTCOs(); ++i )
	{
		trackContentObject * tco = getTCO( i );
		if( tco->startPosition() != _start )
		{
			continue;
		}
		sampleTCO * st = dynamic_cast<sampleTCO *>( tco );
		if( !st->muted() )
		{
			samplePlayHandle * handle = new samplePlayHandle( st );
			connect( m_volumeKnob, SIGNAL( valueChanged( float ) ),
					handle, SLOT( setVolume( float ) ) );
			handle->setVolume( m_volumeKnob->value() );
//TODO: do we need sample tracks in BB editor?
//			handle->setBBTrack( bb_track );
			handle->setOffset( _offset );
			// send it to the mixer
			engine::getMixer()->addPlayHandle( handle );
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}




trackContentObject * sampleTrack::createTCO( const midiTime & )
{
	return( new sampleTCO( this ) );
}





void sampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "name", m_trackLabel->text() );
	m_trackLabel->saveState( _doc, _this );
#if 0
	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
#endif
	m_volumeKnob->saveSettings( _doc, _this, "vol" );
}




void sampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	m_trackLabel->setText( _this.attribute( "name" ) );
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_trackLabel->nodeName() == node.nodeName() )
			{
				m_trackLabel->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
#if 0
	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}
#endif
	m_volumeKnob->loadSettings( _this, "vol" );
}




#include "sample_track.moc"


#endif
