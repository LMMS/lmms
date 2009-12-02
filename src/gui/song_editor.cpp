/*
 * song_editor.cpp - basic window for song-editing
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QAction>
#include <QtGui/QButtonGroup>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QToolBar>
#include <QtGui/QSpacerItem>

#include <math.h>

#include "AudioOutputContext.h"
#include "song_editor.h"
#include "combobox.h"
#include "embed.h"
#include "MainWindow.h"
#include "mmp.h"
#include "timeline.h"
#include "tool_button.h"
#include "tooltip.h"
#include "AudioBackend.h"
#include "piano_roll.h"



positionLine::positionLine( QWidget * _parent ) :
	QWidget( _parent )
{
	setFixedWidth( 1 );
	setAttribute( Qt::WA_NoSystemBackground, true );
}




void positionLine::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.fillRect( rect(), QColor( 255, 255, 255, 153 ) );
}




songEditor::songEditor( song * _song, songEditor * & _engine_ptr ) :
	trackContainerView( _song ),
	m_s( _song ),
	m_scrollBack( false )
{
	_engine_ptr = this;

	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	// create time-line
	m_timeLine = new timeLine( TRACK_OP_WIDTH +
					DEFAULT_SETTINGS_WIDGET_WIDTH, 32,
					pixelsPerTact(),
					m_s->m_playPos[song::Mode_PlaySong],
					m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				m_s->m_playPos[song::Mode_PlaySong].m_timeLine,
			SLOT( updatePosition( const midiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );

	m_positionLine = new positionLine( this );


	// create own toolbar
	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(), 
				embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );

	static_cast<QVBoxLayout *>( layout() )->insertWidget( 0, m_toolBar );
	static_cast<QVBoxLayout *>( layout() )->insertWidget( 1, m_timeLine );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );
	tb_layout->setSpacing( 0 );


	// fill own tool-bar
	m_playButton = new toolButton( embed::getIconPixmap( "play", 24, 24 ),
					tr( "Play song (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_recordButton = new toolButton( embed::getIconPixmap( "record" ),
			tr( "Record samples from Audio-device" ),
					this, SLOT( record() ), m_toolBar );
	m_recordAccompanyButton = new toolButton( 
			embed::getIconPixmap( "record_accompany" ),
			tr( "Record samples from Audio-device while playing "
							"song or BB track" ),
				this, SLOT( recordAccompany() ), m_toolBar );

	// FIXME: disable record button while it is not implemented
	m_recordButton->setDisabled( true );
	
	// disable record buttons if capturing is not supported
	if( !engine::mixer()->audioOutputContext()->audioBackend()->supportsCapture() )
	{
		m_recordButton->setDisabled( true );
		m_recordAccompanyButton->setDisabled( true );
	}

	m_stopButton = new toolButton( embed::getIconPixmap( "stop", 24, 24 ),
					tr( "Stop song (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_addBBTrackButton = new toolButton( embed::getIconPixmap(
						"add_bb_track" ),
						tr( "Add beat/bassline" ),
						m_s, SLOT( addBBTrack() ),
						m_toolBar );

	m_addSampleTrackButton = new toolButton( embed::getIconPixmap(
					"add_sample_track", 24, 24 ),
					tr( "Add sample-track" ),
					m_s, SLOT( addSampleTrack() ),
					m_toolBar );

	m_addAutomationTrackButton = new toolButton( embed::getIconPixmap(
					"add_automation", 24, 24 ),
					tr( "Add automation-track" ),
					m_s, SLOT( addAutomationTrack() ),
					m_toolBar );

	m_drawModeButton = new toolButton( embed::getIconPixmap(
								"edit_draw" ),
							tr( "Draw mode" ),
							NULL, NULL, m_toolBar );
	m_drawModeButton->setCheckable( true );
	m_drawModeButton->setChecked( true );

	m_editModeButton = new toolButton( embed::getIconPixmap(
								"edit_select" ),
					tr( "Edit mode (select and move)" ),
							NULL, NULL, m_toolBar );
	m_editModeButton->setCheckable( true );

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawModeButton );
	tool_button_group->addButton( m_editModeButton );
	tool_button_group->setExclusive( true );

#if 0
#warning TODO
	QWhatsThis::add( m_playButton, tr( "Click here, if you want to play "
						"your whole song. Playing will "
						"be started at the "
						"song-position-marker (green). "
						"You can also move it while "
						"playing." ) );
	QWhatsThis::add( m_stopButton, tr ( "Click here, if you want to stop "
						"playing of your song. The "
						"song-position-marker will be "
						"set to the start of your song."
			) );
/*	QWhatsThis::add( m_insertBarButton, tr( "If you click here, a "
							"bar will "
							"be inserted at the "
							"current bar." ) );
	QWhatsThis::add( m_removeBarButton, tr( "If you click here, the "
							"current bar will be "
							"removed." ) );*/
#endif


	QLabel * zoom_lbl = new QLabel( m_toolBar );
	zoom_lbl->setPixmap( embed::getIconPixmap( "zoom" ) );

	// setup zooming-stuff
	m_zoomingComboBox = new comboBox( m_toolBar );
	m_zoomingComboBox->setFixedSize( 80, 22 );
	m_zoomingComboBox->move( 580, 4 );
	for( int i = 0; i < 7; ++i )
	{
		m_zoomingComboBox->model()->addItem(
					QString::number( 25 << i ) + "%" );
	}
	m_zoomingComboBox->model()->setInitValue(
			m_zoomingComboBox->model()->findText( "100%" ) );
	connect( m_zoomingComboBox->model(), SIGNAL( dataChanged() ),
					this, SLOT( zoomingChanged() ) );


	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_recordButton );
	tb_layout->addWidget( m_recordAccompanyButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_addBBTrackButton );
	tb_layout->addWidget( m_addSampleTrackButton );
	tb_layout->addWidget( m_addAutomationTrackButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_drawModeButton );
	tb_layout->addWidget( m_editModeButton );
	tb_layout->addSpacing( 10 );
	m_timeLine->addToolButtons( m_toolBar );
	tb_layout->addSpacing( 15 );
	tb_layout->addWidget( zoom_lbl );
	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_zoomingComboBox );
	tb_layout->addStretch();


	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setMinimum( 0 );
	m_leftRightScroll->setMaximum( 0 );
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( 20 );
	static_cast<QVBoxLayout *>( layout() )->addWidget( m_leftRightScroll );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ),
					this, SLOT( scrolled( int ) ) );
	connect( m_s, SIGNAL( lengthChanged( int ) ),
			this, SLOT( updateScrollBar( int ) ) );


	engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->resize( 600, 300 );
	parentWidget()->move( 5, 5 );
	parentWidget()->show();
}




songEditor::~songEditor()
{
}



void songEditor::scrolled( int _new_pos )
{
	update();
	emit positionChanged( m_currentPosition = midiTime( _new_pos, 0 ) );
}




void songEditor::play()
{
	engine::mainWindow()->setPlaybackMode( PPM_Song );
	
	m_s->play();
	engine::getPianoRoll()->stopRecording();
	if( m_s->playMode() == song::Mode_PlaySong )
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	}
}




void songEditor::record()
{
	engine::mainWindow()->setPlaybackMode( PPM_Song );
	
	m_s->record();
}




void songEditor::recordAccompany()
{
	engine::mainWindow()->setPlaybackMode( PPM_Song );
	
	m_s->playAndRecord();
}




void songEditor::stop()
{
	m_s->stop();
	engine::getPianoRoll()->stopRecording();
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
}




void songEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->modifiers() & Qt::ShiftModifier &&
						_ke->key() == Qt::Key_Insert )
	{
		m_s->insertBar();
	}
	else if( _ke->modifiers() & Qt::ShiftModifier &&
						_ke->key() == Qt::Key_Delete )
	{
		m_s->removeBar();
	}
	else if( _ke->key() == Qt::Key_Left )
	{
		tick_t t = m_s->currentTick() - midiTime::ticksPerTact();
		if( t >= 0 )
		{
			m_s->setPlayPos( t, song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Right )
	{
		tick_t t = m_s->currentTick() + midiTime::ticksPerTact();
		if( t < MaxSongLength )
		{
			m_s->setPlayPos( t, song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Space )
	{
		if( m_s->isPlaying() )
		{
			stop();
		}
		else
		{
			play();
		}
	}
	else if( _ke->key() == Qt::Key_Home )
	{
		m_s->setPlayPos( 0, song::Mode_PlaySong );
	}
	else
	{
		QWidget::keyPressEvent( _ke );
	}
}




void songEditor::wheelEvent( QWheelEvent * _we )
{
	if( _we->modifiers() & Qt::ControlModifier )
	{
		if( _we->delta() > 0 )
		{
			setPixelsPerTact( (int) qMin( pixelsPerTact() * 2,
								256.0f ) );
		}
		else if( pixelsPerTact() >= 8 )
		{
			setPixelsPerTact( (int) pixelsPerTact() / 2 );
		}
		// update combobox with zooming-factor
		m_zoomingComboBox->model()->setValue(
			m_zoomingComboBox->model()->findText(
				QString::number(
					static_cast<int>( pixelsPerTact() *
					100 / DEFAULT_PIXELS_PER_TACT ) ) +
									"%" ) );
		// update timeline
		m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
		// and make sure, all TCO's are resized and relocated
		realignTracks();
	} 
	else if( _we->modifiers() & Qt::ShiftModifier )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							_we->delta() / 30 );
	}
	else
	{
		_we->ignore();
		return;
	}
	_we->accept();
}




void songEditor::updateScrollBar( int _len )
{
	m_leftRightScroll->setMaximum( _len );
}




void songEditor::updatePosition( const midiTime & _t )
{
	if( ( m_s->isPlaying() && m_s->m_playMode == song::Mode_PlaySong
		  && m_timeLine->autoScroll() == timeLine::AutoScrollEnabled) ||
							m_scrollBack == true )
	{
		const int w = width() - DEFAULT_SETTINGS_WIDGET_WIDTH
							- TRACK_OP_WIDTH;
		if( _t > m_currentPosition + w * midiTime::ticksPerTact() /
							pixelsPerTact() )
		{
			m_leftRightScroll->setValue( _t.getTact() );
		}
		else if( _t < m_currentPosition )
		{
			midiTime t = qMax(
				(int)( _t - w * midiTime::ticksPerTact() /
							pixelsPerTact() ),
									0 );
			m_leftRightScroll->setValue( t.getTact() );
		}
		m_scrollBack = false;
	}

	const int x = m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
							markerX( _t ) + 8;
	if( x >= TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH-1 )
	{
		m_positionLine->show();
		m_positionLine->move( x, 50 );
	}
	else
	{
		m_positionLine->hide();
	}

	m_positionLine->setFixedHeight( height() );
}




void songEditor::zoomingChanged()
{
	const QString & zfac = m_zoomingComboBox->model()->currentText();
	setPixelsPerTact( zfac.left( zfac.length() - 1 ).toInt() *
					DEFAULT_PIXELS_PER_TACT / 100 );
	m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
	realignTracks();
}




bool songEditor::allowRubberband() const
{
	return( m_editModeButton->isChecked() );
}




#include "moc_song_editor.cxx"


/* vim: set tw=0 noexpandtab: */
