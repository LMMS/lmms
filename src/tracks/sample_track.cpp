#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_track.cpp - implementation of class sampleTrack, a track which
 *                    provides arrangement of samples
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>

#else

#include <qpushbutton.h>
#include <qpainter.h>
#include <qdom.h>
#include <qwhatsthis.h>

#endif


#include "sample_track.h"
#include "song_editor.h"
#include "name_label.h"
#include "embed.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "tooltip.h"
#include "audio_port.h"
#include "string_pair_drag.h"
#include "knob.h"
#include "volume.h"


sampleTCO::sampleTCO( track * _track ) :
	trackContentObject( _track ),
	m_sampleBuffer( eng() )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	saveJournallingState( FALSE );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( eng()->getSongEditor(), SIGNAL( tempoChanged( bpm_t ) ), this,
						SLOT( updateLength( bpm_t ) ) );
}




sampleTCO::~sampleTCO()
{
}




void sampleTCO::changeLength( const midiTime & _length )
{
	trackContentObject::changeLength( tMax( static_cast<Sint32>( _length ),
									64 ) );
}




void FASTCALL sampleTCO::play( sampleFrame * _ab, f_cnt_t _start_frame,
							const fpab_t _frames )
{
	_start_frame = static_cast<Uint32>( tMax( 0.0f, _start_frame -
							startPosition() *
			eng()->getSongEditor()->framesPerTact() / 64 ) );
	m_sampleBuffer.play( _ab, _start_frame, _frames );
}




const QString & sampleTCO::sampleFile( void ) const
{
	return( m_sampleBuffer.audioFile() );
}




void sampleTCO::setSampleFile( const QString & _sf )
{
	m_sampleBuffer.setAudioFile( _sf );
	updateLength();
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	toolTip::add( this, ( m_sampleBuffer.audioFile() != "" ) ?
					m_sampleBuffer.audioFile() :
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
		m_sampleBuffer.loadFromBase64(
					stringPairDrag::decodeValue( _de ) );
		eng()->getSongEditor()->setModified();
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
	QString af = m_sampleBuffer.openAudioFile();
	if( af != "" && af != m_sampleBuffer.audioFile() )
	{
		setSampleFile( af );
		eng()->getSongEditor()->setModified();
	}
}




void sampleTCO::paintEvent( QPaintEvent * _pe )
{
#ifdef QT4
	QPainter p( this );
#warning TODO: set according brush for gradient!
	p.fillRect( _pe->rect(), QColor( 64, 64, 64 ) );
#else
	// create pixmap for our widget
	QPixmap pm( _pe->rect().size() );
	// and a painter for it
	QPainter p( &pm );
	p.translate( -_pe->rect().x(), -_pe->rect().y() );

	for( int y = 1; y < height() - 1; ++y )
	{
		const int gray = 96 - y * 96 / height();
		if( isSelected() == TRUE )
		{
			p.setPen( QColor( 0, 0, 128 + gray ) );
		}
		else
		{
			p.setPen( QColor( gray, gray, gray ) );
		}
		p.drawLine( 1, y, width() - 1, y );
	}
#endif

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( 0, 0, width(), height() );
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
	m_sampleBuffer.visualize( p, r, _pe->rect() );
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

#ifndef QT4
	bitBlt( this, _pe->rect().topLeft(), &pm );
#endif
}




midiTime sampleTCO::getSampleLength( void ) const
{
	return( static_cast<Sint32>( m_sampleBuffer.frames() /
					eng()->getSongEditor()->framesPerTact() *
									64 ) );
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
		_this.setAttribute( "data", m_sampleBuffer.toBase64( s ) );
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
		m_sampleBuffer.loadFromBase64( _this.attribute( "data" ) );
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
	QString af = m_sampleTCO->m_sampleBuffer.openAudioFile();
	if( af != "" )
	{
		setSampleFile( af );
	}
}




void sampleTCOSettingsDialog::setSampleFile( const QString & _f )
{
	m_fileLbl->setText( _f );
	m_sampleTCO->setSampleFile( _f );
	eng()->getSongEditor()->setModified();
}
*/





sampleTrack::sampleTrack( trackContainer * _tc ) :
	track( _tc ),
	m_audioPort( new audioPort( tr( "Sample track" ), eng() ) ),
	m_volume( 1.0f )
{
	getTrackWidget()->setFixedHeight( 32 );

	m_trackLabel = new nameLabel( tr( "Sample track" ),
					getTrackSettingsWidget(), eng() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "sample_track" ) );
	m_trackLabel->setGeometry( 26, 1, DEFAULT_SETTINGS_WIDGET_WIDTH-2, 29 );
	m_trackLabel->show();

	m_volumeKnob = new volumeKnob( knobSmall_17, getTrackSettingsWidget(),
				    tr( "Channel volume" ), eng(), this );
	m_volumeKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
	m_volumeKnob->setInitValue( DEFAULT_VOLUME );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->move( 4, 4 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();
	connect( m_volumeKnob, SIGNAL( valueChanged( float ) ), 
		 this, SLOT( setVolume( float ) ) );
#ifdef QT4
	m_volumeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_volumeKnob,
#endif
		tr( "With this knob you can set "
			"the volume of the opened "
			"channel." ) ); 
	
	_tc->updateAfterTrackAdd();
}




sampleTrack::~sampleTrack()
{
	delete m_audioPort;
}



void sampleTrack::setVolume( float _new_volume )
{
	if( _new_volume <= MAX_VOLUME )
	{
		m_volume = _new_volume / 100.0f;
	}
}




track::trackTypes sampleTrack::type( void ) const
{
	return( SAMPLE_TRACK );
}




bool FASTCALL sampleTrack::play( const midiTime & _start,
						const f_cnt_t _start_frame,
						const fpab_t _frames,
						const f_cnt_t _frame_base,
							Sint16 /*_tco_num*/ )
{
	sendMidiTime( _start );

	vlist<trackContentObject *> tcos;
	getTCOsInRange( tcos, _start, _start+static_cast<Sint32>( _frames * 64 /
				eng()->getSongEditor()->framesPerTact() ) );

	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( _frames );

	volumeVector v = { m_volume, m_volume
#ifndef DISABLE_SURROUND
					, m_volume, m_volume
#endif
			} ;
	float fpt = eng()->getSongEditor()->framesPerTact();

	for( vlist<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
	{
		sampleTCO * st = dynamic_cast<sampleTCO *>( *it );
		if( st != NULL && !st->muted() )
		{
			st->play( buf, _start_frame +
					static_cast<Uint32>( _start.getTact() *
									fpt ),
					_frames );
			eng()->getMixer()->bufferToPort( buf, _frames,
							_frame_base +
							static_cast<Uint32>(
					st->startPosition().getTact64th() *
							fpt / 64.0f ), v,
							m_audioPort );
		}
	}

	bufferAllocator::free( buf );

	return( TRUE );
}




trackContentObject * sampleTrack::createTCO( const midiTime & )
{
	return( new sampleTCO( this ) );
}





void sampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "name", m_trackLabel->text() );
	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
	m_volumeKnob->saveSettings( _doc, _this, "vol" );
}




void sampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	m_trackLabel->setText( _this.attribute( "name" ) );
	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}
	if( _this.attribute( "vol" ) != "" )
	{
		m_volume = _this.attribute( "vol" ).toFloat();
		m_volumeKnob->setInitValue( m_volume * 100.0f );
	}
	else
	{
		QDomNode node = _this.namedItem(
					automationPattern::classNodeName() );
		if( node.isElement() && node.namedItem( "vol" ).isElement() )
		{
			m_volumeKnob->loadSettings( _this, "vol" );
			m_volume = m_volumeKnob->value() / 100.0f;
		}
		else
		{
			m_volume = 1.0;
			m_volumeKnob->setInitValue( 100.0f );
		}
	}
}




#include "sample_track.moc"


#endif
