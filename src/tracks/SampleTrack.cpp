/*
 * SampleTrack.cpp - implementation of class SampleTrack, a track which
 *                   provides arrangement of samples
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>

#include "gui_templates.h"
#include "SampleTrack.h"
#include "song.h"
#include "embed.h"
#include "engine.h"
#include "tooltip.h"
#include "AudioPort.h"
#include "SamplePlayHandle.h"
#include "SampleRecordHandle.h"
#include "string_pair_drag.h"
#include "knob.h"
#include "MainWindow.h"
#include "EffectRackView.h"
#include "track_label_button.h"
#include "config_mgr.h"
#include "rename_dialog.h"


SampleTCO::SampleTCO( track * _track ) :
	trackContentObject( _track ),
	m_sampleBuffer( new SampleBuffer ),
	m_isPlaying( false )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	connect( engine::getSong(), SIGNAL( elapsedFramesChanged() ),
					this, SLOT( updatePlayPosition() ) );
	connect( this, SIGNAL( positionChanged() ),
					this, SLOT( updatePlayPosition() ) );
					
	changeLength( DefaultTicksPerTact );
}




SampleTCO::~SampleTCO()
{
	if( m_isPlaying )
	{
		stopPlayback();
	}
	sharedObject::unref( m_sampleBuffer );
}




void SampleTCO::changeLength( const MidiTime & _length )
{
	trackContentObject::changeLength( _length );
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




void SampleTCO::updateLength()
{
	if( m_sampleBuffer->frames() > 0 )
	{
		changeLength( sampleLength() );
	}
}




MidiTime SampleTCO::sampleLength() const
{
	f_cnt_t start = engine::getSong()->elapsedFramesAt( startPosition() );
	f_cnt_t end = start + m_sampleBuffer->frames();
	MidiTime m = MidiTime( engine::getSong()->miditimeAtFrames( end ) - startPosition() );
	return m;
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




trackContentObjectView * SampleTCO::createView( trackView * _tv )
{
	return new SampleTCOView( this, _tv );
}


/** @brief Starts the playback of this sampletrack tco
 * should only be called by the sampletrack and only when isPlaying() is false
 * @param start Miditime to start playback from (global time, not tco time)
 * @param offset the frame offset that gets passed to the playhandle - if miditime tick is mid-period
 */
bool SampleTCO::startPlayback( const MidiTime & start, f_cnt_t offset )
{
	// if we start at beginning, it's simple - no extra calculation needed
	f_cnt_t startframe = 0;
	// if we start from the middle, use tempo-aware calculation in song to figure out starting frame
	if( start > startPosition() )
	{
		// substract the frameposition at tco start from current frameposition to get frameposition to play from
		startframe = engine::getSong()->elapsedFramesAt( start ) - engine::getSong()->elapsedFramesAt( startPosition() );
	}
	
	bool played_a_note = false;
		if( ! isMuted() )
		{
			// move this to its own startRecord function, perhaps?
			if( isRecord() )
			{
				if( !engine::getSong()->isRecording() )
				{
					return false;
				}
				SampleRecordHandle* smpHandle = new SampleRecordHandle( this );
				m_handle = smpHandle;
			}
			else
			{
				if( m_sampleBuffer->frames() <= startframe )
				{
					return false;
				}
				SamplePlayHandle* smpHandle = new SamplePlayHandle( this, startframe );
				smpHandle->setVolumeModel( static_cast<SampleTrack *>( getTrack() )->volumeModel() );
				m_handle = smpHandle;
				m_isPlaying = true;
			}
			// set offset
			m_handle->setOffset( offset );
			// send it to the mixer
			engine::mixer()->addPlayHandle( m_handle );
			played_a_note = true;
		}
	return played_a_note;
}

/** @brief forces playback of this tco to stop
 * used for stopping playback before sample end, so the user can adjust play time of the tco
 * should only be called when isPlaying() is true, we don't check for it here
 */
void SampleTCO::stopPlayback()
{
	if( m_handle )
	{
		SamplePlayHandle * smpHandle = dynamic_cast<SamplePlayHandle *>( m_handle );
		if( smpHandle )
		{
			// we set the frame position of the playhandle to negative, which causes the playhandle to cease playing
			// and also to set the isPlaying property of this tco to false, so we don't have to do that here
			smpHandle->setFramePosition( -1 );
			// this may not be the prettiest solution, but it is efficient
		}
	}
}


/** @brief slot for updating play position
 * gets triggered whenever the position is changed in ways other than normal song progression
 * eg. loopbacks, manual changes
 */
void SampleTCO::updatePlayPosition()
{
	if( m_isPlaying && m_handle )
	{
		SamplePlayHandle * smpHandle = dynamic_cast<SamplePlayHandle *>( m_handle );
		if( smpHandle )
		{
			f_cnt_t startframe = engine::getSong()->elapsedFrames() - 
				engine::getSong()->elapsedFramesAt( startPosition() );
			smpHandle->setFramePosition( startframe );
		}
	}
}

QPixmap * SampleTCOView::s_pat_rec = NULL;

SampleTCOView::SampleTCOView( SampleTCO * _tco, trackView * _tv ) :
	trackContentObjectView( _tco, _tv ),
	m_tco( _tco )
{
	// update UI and tooltip
	updateSample();

	// track future changes of SampleTCO
	connect( m_tco, SIGNAL( sampleChanged() ),
			this, SLOT( updateSample() ) );

	setStyle( QApplication::style() );
	
	if( s_pat_rec == NULL ) { s_pat_rec = new QPixmap( embed::getIconPixmap(
							"pat_rec" ) ); }
}




SampleTCOView::~SampleTCOView()
{
}




void SampleTCOView::updateSample()
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	toolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
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
	contextMenu.addAction( embed::getIconPixmap( "edit_rename" ),
					tr( "Change name" ),
						this, SLOT( changeName() ) );
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


void SampleTCOView::changeName()
{
	QString s = m_tco->name();
	renameDialog rename_dlg( s );
	rename_dlg.exec();
	m_tco->setName( s );
	update();
}



void SampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		trackContentObjectView::dragEnterEvent( _dee );
	}
}




void SampleTCOView::dropEvent( QDropEvent * _de )
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
		trackContentObjectView::mousePressEvent( _me );
	}
}




void SampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();
	if( af != "" && af != m_tco->m_sampleBuffer->audioFile() )
	{
		m_tco->setSampleFile( af );
		engine::getSong()->setModified();
	}
}




void SampleTCOView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	const QColor styleColor = p.pen().brush().color();

	QColor c;
	if( !( m_tco->getTrack()->isMuted() || m_tco->isMuted() ) )
		c = isSelected() ? QColor( 0, 0, 224 )
						 : styleColor;
	else c = QColor( 80, 80, 80 );

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
	
	// waveform drawing code
	
	// start by calculating how many ticks we have per pixel, do it in float because otherwise it'll just be 0 
	const float ticksPerPixel = static_cast<float>( DefaultTicksPerTact ) / pixelsPerTact();
	int xstep = 2; // go in steps of 2 pixels min. - this is to make the drawing faster
	// if the step size doesn't increment a whole tick (very high zoom levels), increase it
	while( xstep * ticksPerPixel < 1.0f )
	{
		xstep++;
	}
	// get the total tick increment per iteration
	const float ticksPerStep = ticksPerPixel * xstep;
	// get frame values in a vector
	// in the future, we might optimize this by checking if there is any tempo automation overlapping the tco,
	// and use a simpler paint algorithm if there isn't
	QVector<f_cnt_t> framePos = engine::getSong()->elapsedFramesAt( m_tco->startPosition(), m_tco->length() + 1 ); // add 1 just in case
	p.setClipRect( QRect( 1, 1, width() - 2, height() - 2 ) );
	float i = 0; 
	int x = 0;
	while( i < m_tco->length() ) 
	{
		// get rectangle for this step
		QRect r = QRect( x + 1, 1, 
			xstep, height() - 4 );
	
		// get frame window for this step
		f_cnt_t frameStart = framePos[ static_cast<int>( i ) ] - framePos[0];
		f_cnt_t frameEnd = framePos[ static_cast<int>( i + ticksPerStep  ) ] - framePos[0];
		
		// if we're still within the bounds of the samplebuffer's content, visualize it
		if( frameStart < m_tco->m_sampleBuffer->frames() )
		{
			m_tco->m_sampleBuffer->visualize( p, r, _pe->rect(), frameStart, frameEnd );
		}
		// if not, just draw a flatline
		else
		{
			p.drawLine( r.x(), r.y() + r.height() / 2, r.x() + r.width(), r.y() + r.height() / 2 );
		}
		
		i += ticksPerStep;
		x += xstep;
	}
	
	p.translate( 0, 0 );
	
	// pattern name
	p.setFont( pointSize<8>( p.font() ) );

	QColor text_color = ( m_tco->isMuted() || m_tco->getTrack()->isMuted() )
		? QColor( 30, 30, 30 )
		: textColor();

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( 4, p.fontMetrics().height()+1, m_tco->name() );
	p.setPen( text_color );
	p.drawText( 3, p.fontMetrics().height(), m_tco->name() );
	
	if( m_tco->isMuted() )
	{
		p.drawPixmap( 3, 8, embed::getIconPixmap( "muted", 16, 16 ) );
	}
	if( m_tco->isRecord() )
	{
		p.drawPixmap( 4, 14, *s_pat_rec );
	}
	
	
}






SampleTrack::SampleTrack( TrackContainer* tc ) :
	track( track::SampleTrack, tc ),
	m_audioPort( tr( "Sample track" ) ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 1.0, this,
							tr( "Volume" ) )
{
	setName( tr( "Sample track" ) );
}




SampleTrack::~SampleTrack()
{
	engine::mixer()->removePlayHandles( this );
}




bool SampleTrack::play( const MidiTime & start, const fpp_t frames,
						const f_cnt_t offset, int /*_tco_num*/ )
{
	m_audioPort.effects()->startRunning();
	bool played_a_note = false;	// will be return variable

	for( int i = 0; i < numOfTCOs(); ++i )
	{
		trackContentObject * tco = getTCO( i );
		
		if( start <= tco->endPosition()  )
		{
			if( start >= tco->startPosition() )
			{
				SampleTCO * st = dynamic_cast<SampleTCO *>( tco );
				if( !st->isPlaying() )
				{
					played_a_note = st->startPlayback( start, offset );
				}
			}
		}
		else
		{
			// force stop at end of tco, even if the sample is longer
			SampleTCO * st = dynamic_cast<SampleTCO *>( tco );
			if( st->isPlaying() )
			{
				st->stopPlayback();
			}
		}
	}

	return played_a_note;
}




trackView * SampleTrack::createView( TrackContainerView* tcv )
{
	return new SampleTrackView( this, tcv );
}




trackContentObject * SampleTrack::createTCO( const MidiTime & )
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
}






SampleTrackView::SampleTrackView( SampleTrack * _t, TrackContainerView* tcv ) :
	trackView( _t, tcv )
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
	if( configManager::inst()->value( "ui",
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

	m_effectRack = new EffectRackView( _t->audioPort()->effects() );
	m_effectRack->setFixedSize( 240, 242 );

	m_effWindow = engine::mainWindow()->workspace()->addSubWindow( m_effectRack );
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

	trackView::modelChanged();
}



#include "moc_SampleTrack.cxx"

