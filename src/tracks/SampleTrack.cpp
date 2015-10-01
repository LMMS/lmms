/*
 * SampleTrack.cpp - implementation of class SampleTrack, a track which
 *                   provides arrangement of samples
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QDomElement>
#include <QDropEvent>
#include <QMenu>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>
#include <QPushButton>

#include "gui_templates.h"
#include "SampleTrack.h"
#include "Song.h"
#include "embed.h"
#include "Engine.h"
#include "ToolTip.h"
#include "AudioPort.h"
#include "SamplePlayHandle.h"
#include "SampleRecordHandle.h"
#include "StringPairDrag.h"
#include "Knob.h"
#include "MainWindow.h"
#include "GuiApplication.h"
#include "EffectRackView.h"
#include "TrackLabelButton.h"
#include "ConfigManager.h"
#include "panning_constants.h"


SampleTCO::SampleTCO( Track * _track ) :
	TrackContentObject( _track ),
	m_sampleBuffer( new SampleBuffer )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( Engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
					this, SLOT( updateLength( bpm_t ) ) );
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
}




SampleTCO::~SampleTCO()
{
	sharedObject::unref( m_sampleBuffer );
}




void SampleTCO::changeLength( const MidiTime & _length )
{
	TrackContentObject::changeLength( qMax( static_cast<int>( _length ), DefaultTicksPerTact ) );
}




const QString & SampleTCO::sampleFile() const
{
	return m_sampleBuffer->audioFile();
}



void SampleTCO::setSampleBuffer( SampleBuffer* sb )
{
	sharedObject::unref( m_sampleBuffer );
	m_sampleBuffer = sb;
	updateLength();

	emit sampleChanged();
}



void SampleTCO::setSampleFile( const QString & _sf )
{
	m_sampleBuffer->setAudioFile( _sf );
	updateLength();

	emit sampleChanged();
}




void SampleTCO::toggleRecord()
{
	m_recordModel.setValue( !m_recordModel.value() );
	emit dataChanged();
}




void SampleTCO::updateLength( bpm_t )
{
	changeLength( sampleLength() );
}




MidiTime SampleTCO::sampleLength() const
{
	return (int)( m_sampleBuffer->frames() / Engine::framesPerTick() );
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
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
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
}




TrackContentObjectView * SampleTCO::createView( TrackView * _tv )
{
	return new SampleTCOView( this, _tv );
}










SampleTCOView::SampleTCOView( SampleTCO * _tco, TrackView * _tv ) :
	TrackContentObjectView( _tco, _tv ),
	m_tco( _tco )
{
	// update UI and tooltip
	updateSample();

	// track future changes of SampleTCO
	connect( m_tco, SIGNAL( sampleChanged() ),
			this, SLOT( updateSample() ) );

	setStyle( QApplication::style() );
}




SampleTCOView::~SampleTCOView()
{
}




void SampleTCOView::updateSample()
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	ToolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
					m_tco->m_sampleBuffer->audioFile() :
					tr( "double-click to select sample" ) );
}




void SampleTCOView::contextMenuEvent( QContextMenuEvent * _cme )
{
	if( _cme->modifiers() )
	{
		return;
	}

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




void SampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( StringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		TrackContentObjectView::dragEnterEvent( _dee );
	}
}




void SampleTCOView::dropEvent( QDropEvent * _de )
{
	if( StringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_tco->setSampleFile( StringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( StringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_tco->m_sampleBuffer->loadFromBase64(
					StringPairDrag::decodeValue( _de ) );
		m_tco->updateLength();
		update();
		_de->accept();
		Engine::getSong()->setModified();
	}
	else
	{
		TrackContentObjectView::dropEvent( _de );
	}
}




void SampleTCOView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
		_me->modifiers() & Qt::ShiftModifier )
	{
		m_tco->toggleRecord();
	}
	else
	{
		TrackContentObjectView::mousePressEvent( _me );
	}
}




void SampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();
	if( af != "" && af != m_tco->m_sampleBuffer->audioFile() )
	{
		m_tco->setSampleFile( af );
		Engine::getSong()->setModified();
	}
}




void SampleTCOView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	const QColor styleColor = p.pen().brush().color();

	QColor c;
	if( !( m_tco->getTrack()->isMuted() || m_tco->isMuted() ) )
	{
		c = styleColor;
	}
	else
	{
		c = QColor( 80, 80, 80 );
	}

	if( isSelected() == true )
	{
		c.setRgb( qMax( c.red() - 128, 0 ), qMax( c.green() - 128, 0 ), 255 );
	}

	QLinearGradient grad( 0, 0, 0, height() );

	grad.setColorAt( 1, c.darker( 300 ) );
	grad.setColorAt( 0, c );

	p.setBrush( grad );
	p.setPen( c.lighter( 160 ) );
	p.drawRect( 1, 1, width()-3, height()-3 );

	p.setBrush( QBrush() );
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, width()-1, height()-1 );


	if( m_tco->getTrack()->isMuted() || m_tco->isMuted() )
	{
		p.setPen( QColor( 128, 128, 128 ) );
	}
	else
	{
		p.setPen( fgColor() );
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
		p.setFont( pointSize<7>( p.font() ) );

		p.setPen( QColor( 0, 0, 0 ) );
		p.drawText( 10, p.fontMetrics().height()+1, "Rec" );
		p.setPen( textColor() );
		p.drawText( 9, p.fontMetrics().height(), "Rec" );

		p.setBrush( QBrush( textColor() ) );
		p.drawEllipse( 4, 5, 4, 4 );
	}
}






SampleTrack::SampleTrack( TrackContainer* tc ) :
	Track( Track::SampleTrack, tc ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 1.0, this,
							tr( "Volume" ) ),
	m_panningModel( DefaultPanning, PanningLeft, PanningRight, 0.1f,
					this, tr( "Panning" ) ),
	m_audioPort( tr( "Sample track" ), true, &m_volumeModel, &m_panningModel, &m_mutedModel )
{
	setName( tr( "Sample track" ) );
	m_panningModel.setCenterValue( DefaultPanning );
}




SampleTrack::~SampleTrack()
{
	Engine::mixer()->removePlayHandles( this );
}




bool SampleTrack::play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _offset, int /*_tco_num*/ )
{
	m_audioPort.effects()->startRunning();
	bool played_a_note = false;	// will be return variable

	for( int i = 0; i < numOfTCOs(); ++i )
	{
		TrackContentObject * tco = getTCO( i );
		if( tco->startPosition() != _start )
		{
			continue;
		}
		SampleTCO * st = dynamic_cast<SampleTCO *>( tco );
		if( !st->isMuted() )
		{
			PlayHandle* handle;
			if( st->isRecord() )
			{
				if( !Engine::getSong()->isRecording() )
				{
					return played_a_note;
				}
				SampleRecordHandle* smpHandle = new SampleRecordHandle( st );
				handle = smpHandle;
			}
			else
			{
				SamplePlayHandle* smpHandle = new SamplePlayHandle( st );
				smpHandle->setVolumeModel( &m_volumeModel );
				handle = smpHandle;
			}
//TODO: check whether this works
//			handle->setBBTrack( _tco_num );
			handle->setOffset( _offset );
			// send it to the mixer
			Engine::mixer()->addPlayHandle( handle );
			played_a_note = true;
		}
	}

	return played_a_note;
}




TrackView * SampleTrack::createView( TrackContainerView* tcv )
{
	return new SampleTrackView( this, tcv );
}




TrackContentObject * SampleTrack::createTCO( const MidiTime & )
{
	return new SampleTCO( this );
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
}






SampleTrackView::SampleTrackView( SampleTrack * _t, TrackContainerView* tcv ) :
	TrackView( _t, tcv )
{
	setFixedHeight( 32 );

	TrackLabelButton * tlb = new TrackLabelButton( this,
						getTrackSettingsWidget() );
	connect( tlb, SIGNAL( clicked( bool ) ),
			this, SLOT( showEffects() ) );
	tlb->setIcon( embed::getIconPixmap( "sample_track" ) );
	tlb->move( 3, 1 );
	tlb->show();

	m_volumeKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
						    tr( "Track volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_t->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Channel volume:" ), "%" );
	if( ConfigManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		m_volumeKnob->move( DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT-2*24, 2 );
	}
	else
	{
		m_volumeKnob->move( DEFAULT_SETTINGS_WIDGET_WIDTH-2*24, 2 );
	}
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();

	m_panningKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Panning" ) );
	m_panningKnob->setModel( &_t->m_panningModel );
	m_panningKnob->setHintText( tr( "Panning:" ), "%" );
	m_panningKnob->move( DEFAULT_SETTINGS_WIDGET_WIDTH-24, 2 );
	m_panningKnob->setLabel( tr( "PAN" ) );
	m_panningKnob->show();

	m_effectRack = new EffectRackView( _t->audioPort()->effects() );
	m_effectRack->setFixedSize( 240, 242 );

	m_effWindow = gui->mainWindow()->addWindowedWidget( m_effectRack );
	m_effWindow->setAttribute( Qt::WA_DeleteOnClose, false );
	m_effWindow->layout()->setSizeConstraint( QLayout::SetFixedSize );
 	m_effWindow->setWindowTitle( _t->name() );
	m_effWindow->hide();

	setModel( _t );
}




SampleTrackView::~SampleTrackView()
{
	m_effWindow->deleteLater();
}




void SampleTrackView::showEffects()
{
	if( m_effWindow->isHidden() )
	{
		m_effectRack->show();
		m_effWindow->show();
		m_effWindow->raise();
	}
	else
	{
		m_effWindow->hide();
	}
}



void SampleTrackView::modelChanged()
{
	SampleTrack * st = castModel<SampleTrack>();
	m_volumeKnob->setModel( &st->m_volumeModel );

	TrackView::modelChanged();
}
