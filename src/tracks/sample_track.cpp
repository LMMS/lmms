/*
 * sample_track.cpp - implementation of class sampleTrack, a track which
 *                    provides arrangement of samples
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtXml/QDomElement>
#include <QtGui/QDropEvent>
#include <QtGui/QMenu>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>

#include "gui_templates.h"
#include "sample_track.h"
#include "song.h"
#include "embed.h"
#include "engine.h"
#include "tooltip.h"
#include "AudioPort.h"
#include "sample_play_handle.h"
#include "sample_record_handle.h"
#include "string_pair_drag.h"
#include "knob.h"
#include "main_window.h"
#include "effect_rack_view.h"
#include "track_label_button.h"



sampleTCO::sampleTCO( track * _track ) :
	trackContentObject( _track ),
	m_sampleBuffer( new sampleBuffer )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
					this, SLOT( updateLength( bpm_t ) ) );
}




sampleTCO::~sampleTCO()
{
	sharedObject::unref( m_sampleBuffer );
}




void sampleTCO::changeLength( const midiTime & _length )
{
	trackContentObject::changeLength( qMax( static_cast<Sint32>( _length ),
							DefaultTicksPerTact ) );
}




const QString & sampleTCO::sampleFile() const
{
	return m_sampleBuffer->audioFile();
}



void sampleTCO::setSampleBuffer( sampleBuffer * _sb )
{
	sharedObject::unref( m_sampleBuffer );
	m_sampleBuffer = _sb;
	updateLength();

	emit sampleChanged();
}



void sampleTCO::setSampleFile( const QString & _sf )
{
	m_sampleBuffer->setAudioFile( _sf );
	updateLength();

	emit sampleChanged();
}




void sampleTCO::toggleRecord()
{
	m_recordModel.setValue( !m_recordModel.value() );
	emit dataChanged();
}




void sampleTCO::updateLength( bpm_t )
{
	changeLength( sampleLength() );
}




midiTime sampleTCO::sampleLength() const
{
	return (int)( m_sampleBuffer->frames() / engine::framesPerTick() );
}




void sampleTCO::saveSettings( QDomDocument & _doc, QDomElement & _this )
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
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
	}
	// TODO: start- and end-frame
}




void sampleTCO::loadSettings( const QDomElement & _this )
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
}




trackContentObjectView * sampleTCO::createView( trackView * _tv )
{
	return new sampleTCOView( this, _tv );
}










sampleTCOView::sampleTCOView( sampleTCO * _tco, trackView * _tv ) :
	trackContentObjectView( _tco, _tv ),
	m_tco( _tco )
{
	connect( m_tco, SIGNAL( sampleChanged() ),
			this, SLOT( updateSample() ) );
}




sampleTCOView::~sampleTCOView()
{
}




void sampleTCOView::updateSample()
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	toolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
					m_tco->m_sampleBuffer->audioFile() :
					tr( "double-click to select sample" ) );
}




void sampleTCOView::contextMenuEvent( QContextMenuEvent * _cme )
{
	QMenu contextMenu( this );
	if( fixedTCOs() == false )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
						this, SLOT( remove() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), this, SLOT( cut() ) );
	}
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy" ), m_tco, SLOT( copy() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), m_tco, SLOT( paste() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "muted" ),
				tr( "Mute/unmute (<Ctrl> + middle click)" ),
						m_tco, SLOT( toggleMute() ) );
	contextMenu.addAction( embed::getIconPixmap( "record" ),
				tr( "Set/clear record" ),
						m_tco, SLOT( toggleRecord() ) );
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}




void sampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		trackContentObjectView::dragEnterEvent( _dee );
	}
}




void sampleTCOView::dropEvent( QDropEvent * _de )
{
	if( stringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_tco->setSampleFile( stringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( stringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_tco->m_sampleBuffer->loadFromBase64(
					stringPairDrag::decodeValue( _de ) );
		m_tco->updateLength();
		update();
		_de->accept();
		engine::getSong()->setModified();
	}
	else
	{
		trackContentObjectView::dropEvent( _de );
	}
}




void sampleTCOView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
		_me->modifiers() & Qt::ShiftModifier )
	{
		m_tco->toggleRecord();
	}
	else
	{
		trackContentObjectView::mousePressEvent( _me );
	}
}




void sampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();
	if( af != "" && af != m_tco->m_sampleBuffer->audioFile() )
	{
		m_tco->setSampleFile( af );
		engine::getSong()->setModified();
	}
}




void sampleTCOView::paintEvent( QPaintEvent * _pe )
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
	if( m_tco->getTrack()->isMuted() || m_tco->isMuted() )
	{
		p.setPen( QColor( 128, 128, 128 ) );
	}
	else
	{
		p.setPen( QColor( 64, 224, 160 ) );
	}
	QRect r = QRect( 1, 1,
			qMax( static_cast<int>( m_tco->sampleLength() *
				pixelsPerTact() / DefaultTicksPerTact ), 1 ),
								height() - 4 );
	p.setClipRect( QRect( 1, 1, width() - 2, height() - 2 ) );
	m_tco->m_sampleBuffer->visualize( p, r, _pe->rect() );
	if( r.width() < width() - 1 )
	{
		p.drawLine( r.x() + r.width(), r.y() + r.height() / 2,
				width() - 2, r.y() + r.height() / 2 );
	}

	p.translate( 0, 0 );
	if( m_tco->isMuted() )
	{
		p.drawPixmap( 3, 8, embed::getIconPixmap( "muted", 16, 16 ) );
	}
	if( m_tco->isRecord() )
	{
		p.setFont( pointSize<6>( p.font() ) );
		p.setPen( QColor( 224, 0, 0 ) );
		p.drawText( 9, p.fontMetrics().height() - 1, "Rec" );
		p.setBrush( QBrush( QColor( 224, 0, 0 ) ) );
		p.drawEllipse( 4, 5, 4, 4 );
	}
}






sampleTrack::sampleTrack( trackContainer * _tc ) :
	track( SampleTrack, _tc ),
	m_audioPort( tr( "Sample track" ) ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 1.0, this,
							tr( "Volume" ) )
{
	setName( tr( "Sample track" ) );
}




sampleTrack::~sampleTrack()
{
	engine::getMixer()->removePlayHandles( this );
}




bool sampleTrack::play( const midiTime & _start, const fpp_t _frames,
						const f_cnt_t _offset,
							Sint16 /*_tco_num*/ )
{
	m_audioPort.getEffects()->startRunning();
	bool played_a_note = false;	// will be return variable

	for( int i = 0; i < numOfTCOs(); ++i )
	{
		trackContentObject * tco = getTCO( i );
		if( tco->startPosition() != _start )
		{
			continue;
		}
		sampleTCO * st = dynamic_cast<sampleTCO *>( tco );
		if( !st->isMuted() )
		{
			playHandle * handle;
			if( st->isRecord() )
			{
				if( !engine::getSong()->isRecording() )
				{
					return played_a_note;
				}
				sampleRecordHandle * smpHandle = new sampleRecordHandle( st );
				handle = smpHandle;
			}
			else
			{
				samplePlayHandle * smpHandle = new samplePlayHandle( st );
				smpHandle->setVolumeModel( &m_volumeModel );
				handle = smpHandle;
			}
//TODO: check whether this works
//			handle->setBBTrack( _tco_num );
			handle->setOffset( _offset );
			// send it to the mixer
			engine::getMixer()->addPlayHandle( handle );
			played_a_note = true;
		}
	}

	return played_a_note;
}




trackView * sampleTrack::createView( trackContainerView * _tcv )
{
	return new sampleTrackView( this, _tcv );
}




trackContentObject * sampleTrack::createTCO( const midiTime & )
{
	return new sampleTCO( this );
}




void sampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_audioPort.getEffects()->saveState( _doc, _this );
#if 0
	_this.setAttribute( "icon", tlb->pixmapFile() );
#endif
	m_volumeModel.saveSettings( _doc, _this, "vol" );
}




void sampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	m_audioPort.getEffects()->clear();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_audioPort.getEffects()->nodeName() ==
							node.nodeName() )
			{
				m_audioPort.getEffects()->restoreState(
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}
	m_volumeModel.loadSettings( _this, "vol" );
}






sampleTrackView::sampleTrackView( sampleTrack * _t, trackContainerView * _tcv ) :
	trackView( _t, _tcv )
{
	setFixedHeight( 32 );

	trackLabelButton * tlb = new trackLabelButton( this,
						getTrackSettingsWidget() );
	connect( tlb, SIGNAL( clicked( bool ) ),
			this, SLOT( showEffects() ) );
	tlb->setIcon( embed::getIconPixmap( "sample_track" ) );
	tlb->move( 3, 1 );
	tlb->show();

	m_volumeKnob = new knob( knobSmall_17, getTrackSettingsWidget(),
						    tr( "Track volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_t->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->move( DEFAULT_SETTINGS_WIDGET_WIDTH-2*24, 4 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();

	m_effectRack = new effectRackView( _t->audioPort()->getEffects() );
	m_effectRack->setFixedSize( 240, 242 );

	engine::getMainWindow()->workspace()->addSubWindow( m_effectRack );
	m_effWindow = m_effectRack->parentWidget();
	m_effWindow->setAttribute( Qt::WA_DeleteOnClose, false );
	m_effWindow->layout()->setSizeConstraint( QLayout::SetFixedSize );
 	m_effWindow->setWindowTitle( _t->name() );
	m_effWindow->hide();
}




sampleTrackView::~sampleTrackView()
{
	m_effWindow->deleteLater();
}




void sampleTrackView::showEffects()
{
	if( m_effWindow->isHidden() )
	{
		m_effWindow->show();
		m_effWindow->raise();
	}
	else
	{
		m_effWindow->hide();
	}
}




#include "moc_sample_track.cxx"


