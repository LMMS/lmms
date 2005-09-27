/*
 * song_editor.cpp - basic window for editing song
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QSlider>
#include <QStatusBar>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>
#include <QStatusBar>
#include <QAction>
#include <QToolBar>

#else

#include <qapplication.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qslider.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qstatusbar.h>

#endif


#include "song_editor.h"
#include "bb_editor.h"
#include "rename_dialog.h"
#include "embed.h"
#include "templates.h"
#include "export_project_dialog.h"
#include "bb_track.h"
#include "channel_track.h"
#include "mmp.h"
#include "midi_device.h"
#include "timeline.h"
#include "pattern.h"
#include "piano_roll.h"
#include "envelope_and_lfo_widget.h"
#include "visualization_widget.h"
#include "project_notes.h"
#include "config_mgr.h"
#include "midi_file.h"
#include "lcd_spinbox.h"
#include "tooltip.h"

#include "debug.h"


#include "triple_oscillator.h"


extern QString file_to_load;
extern QString file_to_render;


const int SCROLLBAR_SIZE = 16;


songEditor * songEditor::s_instanceOfMe = NULL;


songEditor::songEditor() :
	trackContainer(),
	m_fileName( "" ),
	m_oldFileName( "" ),
	m_exporting( FALSE ),
	m_playing( FALSE ),
	m_paused( FALSE ),
	m_playMode( PLAY_SONG ),
	m_trackToPlay( NULL ),
	m_patternToPlay( NULL ),
	m_loopPattern( FALSE ),
	m_scrollBack( FALSE ),
	m_epd( NULL ),
	m_shiftPressed( FALSE ),
	m_controlPressed( FALSE )
{
	// hack, because code called out of this function uses
	// songEditor::inst(), which assigns s_instanceOfMe after return
	// of this function...
	s_instanceOfMe = this;

	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );
	setGeometry( 10, 10, 640, 300 );
	show();

#ifdef QT4
	setFocusPolicy( Qt::StrongFocus );
#else
	setFocusPolicy( StrongFocus );
#endif
	setFocus();


	QWidget * cw = new QWidget( this );
	setCentralWidget( cw );

	// create time-line
	timeLine * tl = new timeLine( TRACK_OP_WIDTH +
					DEFAULT_SETTINGS_WIDGET_WIDTH, 0,
					pixelsPerTact(), m_playPos[PLAY_SONG],
					m_currentPosition, cw );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				m_playPos[PLAY_SONG].m_timeLine,
			SLOT( updatePosition( const midiTime & ) ) );
	connect( tl, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );

#ifdef QT4
	containerWidget()->setParent( cw );
#else
	containerWidget()->reparent( cw, 0, QPoint( 0, 0 ) );
#endif
	containerWidget()->move( 0, tl->height() );


	QToolBar * song_control = new QToolBar( tr( "Song control" ), this );
#ifdef QT4
	addToolBar( Qt::TopToolBarArea, song_control );
#else
	addDockWindow( song_control, tr( "Song control" ), Qt::DockTop,
			FALSE );
#endif
/*	song_control->setPaletteBackgroundPixmap( embed::getIconPixmap(
							"toolbar_bg" ) );
	song_control->setErasePixmap( embed::getIconPixmap( "toolbar_bg" ) );*/

#ifdef QT4
	QAction * a;

	a = song_control->addAction( embed::getIconPixmap( "play" ),
						tr( "Play song (Space)" ),
						this, SLOT( play() ) );
	a->setToolTip( tr( "Play/pause song (Space)" ) );
	a->setWhatsThis( tr( "Click here, if you want to play your whole song. "
				"Playing will be started at the song-position-"
				"marker (green). You can also move it while "
				"playing." ) );
#else
	m_playButton = new QToolButton( embed::getIconPixmap( "play" ),
					tr( "Play song (Space)" ),
					QString::null, this, SLOT( play() ),
					song_control );
#endif
#ifdef QT4
	a = song_control->addAction( embed::getIconPixmap( "stop" ),
						tr( "Stop song (Space)" ),
						this, SLOT( stop() ) );
	a->setToolTip( tr( "Stop song (Space)" ) );
	a->setWhatsThis( tr( "Click here, if you want to stop playing of your "
				"song. The song-position-marker will be set to "
				"the start of your song." ) );
#else
	m_stopButton = new QToolButton( embed::getIconPixmap( "stop" ),
					tr( "Stop song (Space)" ),
					QString::null, this, SLOT( stop() ),
					song_control );
#endif


	song_control->addSeparator();

	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 10, 1 );


	QLabel * bpm_label = new QLabel( song_control );
	bpm_label->setPixmap( embed::getIconPixmap( "clock" ) );

	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 8, 1 );


	m_bpmSpinBox = new lcdSpinBox( MIN_BPM, MAX_BPM, 3, song_control );
#ifdef QT4
	song_control->addWidget( m_bpmSpinBox );
	song_control->addWidget( bpm_label );
#endif
	m_bpmSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	connect( m_bpmSpinBox, SIGNAL( valueChanged( int ) ), this,
							SLOT( setBPM( int ) ) );
	toolTip::add( m_bpmSpinBox, tr( "tempo of song" ) );

#ifdef QT4
	m_bpmSpinBox->setWhatsThis(
#else
	QWhatsThis::add( m_bpmSpinBox,
#endif
		tr( "The tempo of a song is specified in beats per minute "
			"(BPM). If you want to change the tempo of your "
			"song, change this value. Every tact has four beats, "
			"so the tempo in BPM specifies, how many tacts / 4 "
			"should be played within a minute (or how many tacts "
			"should be played within four minutes)." ) );

	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 10, 1 );


	song_control->addSeparator();


	QLabel * master_vol_lbl = new QLabel( song_control );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

#ifdef QT4
	m_masterVolumeSlider = new QSlider( Qt::Vertical, song_control );
	m_masterVolumeSlider->setRange( 0, 200 );
	m_masterVolumeSlider->setPageStep( 10 );
	m_masterVolumeSlider->setValue( 100 );
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
	song_control->addWidget( master_vol_lbl );
	song_control->addWidget( m_masterVolumeSlider );
#else
	m_masterVolumeSlider = new QSlider( 0, 200, 10, 100, Qt::Vertical,
								song_control );
	m_masterVolumeSlider->setTickPosition( QSlider::Left );
#endif
	m_masterVolumeSlider->setFixedSize( 26, 48 );
	m_masterVolumeSlider->setTickInterval( 50 );
	toolTip::add( m_masterVolumeSlider, tr( "master output volume" ) );

	connect( m_masterVolumeSlider, SIGNAL( valueChanged( int ) ), this,
			SLOT( masterVolumeChanged( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterVolumePressed() ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderMoved( int ) ), this,
			SLOT( masterVolumeMoved( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterVolumeReleased() ) );


	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 10, 1 );

	QLabel * master_pitch_lbl = new QLabel( song_control );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );

#ifdef QT4
	m_masterPitchSlider = new QSlider( Qt::Vertical, song_control );
	m_masterPitchSlider->setRange( -12, 12 );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setValue( 0 );
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
	song_control->addWidget( master_pitch_lbl );
	song_control->addWidget( m_masterPitchSlider );
#else
	m_masterPitchSlider = new QSlider( -12, 12, 1, 0, Qt::Vertical,
						song_control );
	m_masterPitchSlider->setTickPosition( QSlider::Left );
#endif
	m_masterPitchSlider->setFixedSize( 26, 48 );
	m_masterPitchSlider->setTickInterval( 12 );
	toolTip::add( m_masterPitchSlider, tr( "master pitch" ) );
	connect( m_masterPitchSlider, SIGNAL( valueChanged( int ) ), this,
			SLOT( masterPitchChanged( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterPitchPressed() ) );
	connect( m_masterPitchSlider, SIGNAL (sliderMoved( int) ), this,
			SLOT( masterPitchMoved( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterPitchReleased() ) );

	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 5, 1 );

	song_control->addSeparator();

	// spacer-item
	( new QWidget( song_control ) )->setFixedSize( 5, 1 );

	m_masterOutputGraph = new visualizationWidget( embed::getIconPixmap(
					"output_graph" ), song_control ); 
#ifdef QT4
	song_control->addWidget( m_masterOutputGraph );
#endif
	// live high-quality mode switching is somewhat experimental so we don't
	// offer it...
/*	QToolButton * hq = new QToolButton(
					embed::getIconPixmap( "presetfile" ),
					tr( "High quality mode" ),
					QString::null, NULL, NULL,
					song_control );
	hq->setToggleButton( TRUE );
	connect( hq, SIGNAL( toggled( bool ) ), mixer::inst(),
					SLOT( setHighQuality( bool ) ) );*/


	m_leftRightScroll = new QScrollBar( Qt::Horizontal, cw );
	m_leftRightScroll->setMinimum( 0 );
	m_leftRightScroll->setMaximum( 0 );
#ifdef QT4
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( 20 );
#else
	m_leftRightScroll->setSteps( 1, 20 );
#endif
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ), this,
			SLOT( scrolled( int ) ) );


	QToolBar * edit_tb = new QToolBar( tr( "Edit" ), this );
#ifdef QT4
	addToolBar( Qt::TopToolBarArea, edit_tb );
#else
	addDockWindow( edit_tb, tr( "Edit" ), Qt::DockTop, FALSE );
#endif
/*	edit_tb->setPaletteBackgroundPixmap( embed::getIconPixmap(
							"toolbar_bg" ) );
	edit_tb->setErasePixmap( embed::getIconPixmap( "toolbar_bg" ) );*/
#ifdef QT4
	a = edit_tb->addAction( embed::getIconPixmap( "add_channel_track" ), "",
					this, SLOT( addChannelTrack() ) );
	a->setToolTip( tr( "Add channel-track" ) );
#else
	m_addChannelTrackButton = new QToolButton( embed::getIconPixmap(
					"add_channel_track" ), "", "",
					this, SLOT( addChannelTrack() ),
					edit_tb );
#endif
#ifdef QT4
	a = edit_tb->addAction( embed::getIconPixmap( "add_bb_track" ), "",
						this, SLOT( addBBTrack() ) );
	a->setToolTip( tr( "Add beat/bassline" ) );
#else
	m_addBBTrackButton = new QToolButton( embed::getIconPixmap(
						"add_bb_track" ), "", "",
						this, SLOT( addBBTrack() ),
						edit_tb );
#endif
#ifdef QT4
	a = edit_tb->addAction( embed::getIconPixmap( "add_sample_track" ), "",
				this, SLOT( addSampleTrack() ) );
	a->setToolTip( tr( "Add sample-track" ) );
#else
	m_addSampleTrackButton = new QToolButton( embed::getIconPixmap(
					"add_sample_track" ), "", "",
					this, SLOT( addSampleTrack() ),
					edit_tb );

#endif

	edit_tb->addSeparator();

#ifdef QT4
	a = edit_tb->addAction( embed::getIconPixmap( "se_insert_tact" ), "",
						this, SLOT( insertTact() ) );
	a->setToolTip( tr( "Insert bar at current tact (Shift+Insert)" ) );
	a->setWhatsThis( tr( "If you click here, a tact will be inserted at "
				"the current tact." ) );
#else
	m_insertTactButton = new QToolButton( embed::getIconPixmap(
						"se_insert_tact" ), "", "",
						this, SLOT( insertTact() ),
						edit_tb );
#endif
#ifdef QT4
	a = edit_tb->addAction( embed::getIconPixmap( "se_remove_tact" ), "",
						this, SLOT( removeTact() ) );
	a->setToolTip( tr( "Remove bar at current tact (Shift+Delete)" ) );
	a->setWhatsThis( tr( "If you click here, the tact at the current tact "
							"will be removed." ) );
#else
	m_removeTactButton = new QToolButton( embed::getIconPixmap(
						"se_remove_tact" ), "", "",
						this, SLOT( removeTact() ),
						edit_tb );
#endif

	// add tooltips and whats-this-texts to all buttons

	toolTip::add( m_playButton, tr( "Play/pause song (Space)" ) );
	toolTip::add( m_stopButton, tr( "Stop playing song (Space)" ) );
	toolTip::add( m_addChannelTrackButton, tr( "Add channel-track" ) );
	toolTip::add( m_addBBTrackButton, tr( "Add beat/bassline" ) );
	toolTip::add( m_addSampleTrackButton, tr( "Add sample-track" ) );
	toolTip::add( m_insertTactButton, tr( "Insert tact at current tact "
						"(Shift+Insert)" ) );
	toolTip::add( m_removeTactButton, tr( "Remove tact at current tact "
						"(Shift+Delete)" ) );
#ifdef QT4
#else
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
	QWhatsThis::add( m_insertTactButton, tr( "If you click here, a "
							"tact will "
							"be inserted at the "
							"current tact." ) );
	QWhatsThis::add( m_removeTactButton, tr( "If you click here, the "
							"tact at the "
							"current tact will be "
							"removed." ) );
#endif

	m_projectNotes = new projectNotes();
	m_projectNotes->resize( 300, 200 );
	m_projectNotes->move( 640, 10 );
	m_projectNotes->show();


	// we try to load given file
	if( file_to_load != "" )
	{
		loadProject( file_to_load );
	}
	else
	{
		createNewProject();
	}
}




songEditor::~songEditor()
{
}




void songEditor::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	hide();
}




void songEditor::paintEvent( QPaintEvent * _pe )
{
	m_leftRightScroll->setMaximum( lengthInTacts() );
	trackContainer::paintEvent( _pe );
}




// responsible for moving scrollbars after resizing
void songEditor::resizeEvent( QResizeEvent * _re )
{
	m_leftRightScroll->setGeometry( 0,
					centralWidget()->height() - 2 -
								SCROLLBAR_SIZE,
					centralWidget()->width() -
								SCROLLBAR_SIZE,
					SCROLLBAR_SIZE );

	m_playPos[PLAY_SONG].m_timeLine->setFixedWidth(
						centralWidget()->width() );
	trackContainer::resizeEvent( _re );
}




void songEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Shift )
	{
		m_shiftPressed = TRUE;
	}
	else
	{
		m_shiftPressed = FALSE;
	}
	if( _ke->key() == Qt::Key_Control )
	{
		m_controlPressed = TRUE;
	}
	else
	{
		m_controlPressed = FALSE;
	}

	if( _ke->modifiers() & Qt::ShiftModifier &&
		_ke->key() == Qt::Key_Insert )
	{
		insertTact();
	}
	else if( _ke->modifiers() & Qt::ShiftModifier &&
		_ke->key() == Qt::Key_Delete )
	{
		removeTact();
	}
	else if( _ke->key() == Qt::Key_Left )
	{
		tact interesting_tact = currentTact();
		if( interesting_tact > 0 )
		{
			setPlayPos( --interesting_tact, currentTact64th(),
								PLAY_SONG );
		}

	}
	else if( _ke->key() == Qt::Key_Right )
	{
		tact interesting_tact = currentTact();
		if( interesting_tact < MAX_SONG_LENGTH )
		{
			setPlayPos( ++interesting_tact, currentTact64th(),
								PLAY_SONG );
		}

	}
	else if( _ke->key() == Qt::Key_Space )
	{
		if( playing() )
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
		setPlayPos( 0, 0, PLAY_SONG );
	}
	else
	{
		_ke->ignore();
	}
}





void songEditor::scrolled( int _new_pos )
{
	update();
	emit positionChanged( m_currentPosition = midiTime( _new_pos, 0 ) );
}




void songEditor::wheelEvent( QWheelEvent * _we )
{
	if( m_controlPressed )
	{
		if( _we->delta() > 0 )
		{
			setPixelsPerTact( (int) tMin( pixelsPerTact() * 2,
								256.0f ) );
		}
		else if( pixelsPerTact() >= 8 )
		{
			setPixelsPerTact( (int) pixelsPerTact() / 2 );
		}
		m_playPos[PLAY_SONG].m_timeLine->setPixelsPerTact(
							pixelsPerTact() );
		realignTracks( TRUE );
	} 
	else if( m_shiftPressed )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							_we->delta() / 30 );
	}
}




void songEditor::masterVolumeChanged( int _new_val )
{
	mixer::inst()->setMasterOutput( 2.0f - _new_val / 100.0f );
	setModified();
}




void songEditor::masterVolumePressed( void )
{
	masterVolumeMoved( m_masterVolumeSlider->value() );
}




void songEditor::masterVolumeMoved( int _new_val )
{
	lmmsMainWin::inst()->statusBar()->showMessage( tr(
						"Master output volume:" ) +
				" " + QString::number( 200 - _new_val ) + "%" );
}




void songEditor::masterVolumeReleased( void )
{
	lmmsMainWin::inst()->statusBar()->clearMessage();
}




void songEditor::masterPitchChanged( int _new_val )
{
	setModified();
}




void songEditor::masterPitchPressed( void )
{
	masterPitchMoved( m_masterPitchSlider->value() );
}




void songEditor::masterPitchMoved( int _new_val )
{
	lmmsMainWin::inst()->statusBar()->showMessage( tr(
						"Master output pitch:" ) +
		" " + QString::number( -_new_val ) + " " + tr( "semitones" ) );

}




void songEditor::masterPitchReleased( void )
{
	lmmsMainWin::inst()->statusBar()->clearMessage();
}




void songEditor::toggleHQMode( void )
{
	//mixer::inst()->setHighQuality (hq_btn->isChecked());
}




void songEditor::updatePosition( const midiTime & _t )
{
	if( ( m_playing && m_playMode == PLAY_SONG ) || m_scrollBack == TRUE )
	{
		const int w = centralWidget()->width() -
				DEFAULT_SETTINGS_WIDGET_WIDTH - TRACK_OP_WIDTH;
		if( _t > m_currentPosition + w * 64 / pixelsPerTact() )
		{
			m_leftRightScroll->setValue( _t.getTact() );
		}
		else if( _t < m_currentPosition )
		{
			midiTime t = tMax( (int)( _t - w * 64 * 64 /
							pixelsPerTact() ),
									0 );
			m_leftRightScroll->setValue( t.getTact() );
		}
		m_scrollBack = FALSE;
	}
}




void songEditor::setBPM( int _new_bpm )
{
	m_bpmSpinBox->setValue( tLimit( _new_bpm, MIN_BPM, MAX_BPM ) );
	setModified();
}




int songEditor::masterPitch( void ) const
{
	return( -m_masterPitchSlider->value() );
}




void songEditor::doActions( void )
{
	while( !m_actions.empty() )
	{
		switch( m_actions.front() )
		{
			case ACT_STOP_PLAY:
			{
				timeLine * tl =
					m_playPos[m_playMode].m_timeLine;
				m_playing = FALSE;
				if( tl != NULL )
				{

		switch( tl->behaviourAtStop() )
		{
			case timeLine::BACK_TO_ZERO:
				m_playPos[m_playMode].setTact( 0 );
				m_playPos[m_playMode].setTact64th( 0 );
				break;

			case timeLine::BACK_TO_START:
				if( tl->savedPos() >= 0 )
				{
					m_playPos[m_playMode].setTact(
						tl->savedPos().getTact() );
					m_playPos[m_playMode].setTact64th(
						tl->savedPos().getTact64th() );
					tl->savePos( -1 );
				}
				break;

			case timeLine::KEEP_STOP_POSITION:
			default:
				break;
		}

				}
				else
				{
					m_playPos[m_playMode].setTact( 0 );
					m_playPos[m_playMode].setTact64th( 0 );
				}

				m_playPos[m_playMode].setCurrentFrame( 0 );
				updateTimeLinePosition();

				// remove all note-play-handles that are active
				mixer::inst()->clear();

				break;
			}

			case ACT_PLAY_SONG:
				m_playMode = PLAY_SONG;
				m_playing = TRUE;
				break;

			case ACT_PLAY_TRACK:
				m_playMode = PLAY_TRACK;
				m_playing = TRUE;
				break;

			case ACT_PLAY_BB:
				m_playMode = PLAY_BB;
				m_playing = TRUE;
				break;

			case ACT_PLAY_PATTERN:
				m_playMode = PLAY_PATTERN;
				m_playing = TRUE;
				break;

			case ACT_PAUSE:
				m_playing = FALSE;// just set the play-flag
				m_paused = TRUE;
				break;

			case ACT_RESUME_FROM_PAUSE:
				m_playing = TRUE;// just set the play-flag
				m_paused = FALSE;
				break;
		}

		// a second switch for saving pos when starting to play
		// anything
		switch( m_actions.front() )
		{
			case ACT_PLAY_SONG:
			case ACT_PLAY_TRACK:
			case ACT_PLAY_BB:
			case ACT_PLAY_PATTERN:
			{
				timeLine * tl =
					m_playPos[m_playMode].m_timeLine;
				if( tl != NULL )
				{
					tl->savePos( m_playPos[m_playMode] );
				}
				break;
			}

			// keep GCC happy...
			default:
				break;
		}

		m_actions.erase( m_actions.begin() );

	}

}




void songEditor::processNextBuffer( void )
{
	doActions();

	if( m_playing == FALSE )
	{
		return;
	}

	trackVector tv;
	Sint16 tco_num = -1;

	switch( m_playMode )
	{
		case PLAY_SONG:
			tv = tracks();
			// at song-start we have to reset the LFOs
			if( m_playPos[PLAY_SONG] == 0 )
			{
				envelopeAndLFOWidget::resetLFO();
			}
			break;

		case PLAY_TRACK:
			tv.push_back( m_trackToPlay );
			break;

		case PLAY_BB:
			if( bbEditor::inst()->numOfBBs() > 0 )
			{
				tco_num = bbEditor::inst()->currentBB();
				tv.push_back( bbTrack::findBBTrack( tco_num ) );
			}
			break;

		case PLAY_PATTERN:
			tco_num = m_patternToPlay->getTrack()->getTCONum(
							m_patternToPlay );
			tv.push_back( m_patternToPlay->getTrack() );
			break;

		default:
			return;

	}

	if( tv.empty() == TRUE )
	{
		return;
	}

	// check for looping-mode and act if neccessary
	timeLine * tl = m_playPos[m_playMode].m_timeLine;
	if( tl != NULL && m_exporting == FALSE && tl->loopPointsEnabled() )
	{
		if( m_playPos[m_playMode] < tl->loopBegin() ||
					m_playPos[m_playMode] >= tl->loopEnd() )
		{
			m_playPos[m_playMode].setTact(
						tl->loopBegin().getTact() );
			m_playPos[m_playMode].setTact64th(
						tl->loopBegin().getTact64th() );
			// force reset of current-frame-var afterwards
			m_playPos[m_playMode].setCurrentFrame( 0 );
		}
	}

	Uint32 total_frames_played = 0;
	Uint32 frames_per_tact = static_cast<Uint32>( framesPerTact() );
	if( m_playPos[m_playMode].currentFrame() == 0 &&
		m_playPos[m_playMode].getTact64th() > 0 )
	{
		m_playPos[m_playMode].setCurrentFrame(
					m_playPos[m_playMode].getTact64th() *
							frames_per_tact / 64 );
	}

	while( total_frames_played < mixer::inst()->framesPerAudioBuffer() )
	{
		Uint32 played_frames = mixer::inst()->framesPerAudioBuffer() -
					total_frames_played;

		// did we play a whole tact?
		if( m_playPos[m_playMode].currentFrame() >= frames_per_tact )
		{
			// per default we just continue playing even if
			// there's no more stuff to play (song-play-mode)
			int max_tact = m_playPos[m_playMode].getTact() + 2;

			// then decide whether to go over to next tact or to 
			// loop back to first tact
			if( m_playMode == PLAY_BB )
			{
				max_tact =
					bbEditor::inst()->lengthOfCurrentBB();
			}
			else if( m_playMode == PLAY_PATTERN &&
					m_loopPattern == TRUE &&
					tl != NULL &&
					tl->loopPointsEnabled() == FALSE )
			{
				max_tact = m_patternToPlay->length().getTact();
			}
			if( m_playPos[m_playMode].getTact() + 1 < max_tact )
			{
				// next tact
				m_playPos[m_playMode].setTact(
					m_playPos[m_playMode].getTact() + 1 );
			}
			else
			{
				// first tact
				m_playPos[m_playMode].setTact( 0 );
			}
			m_playPos[m_playMode].setCurrentFrame( 0 );
		}
		// or do we have some samples left in this tact but this are 
		// less then samples we have to play?
		else if( frames_per_tact - m_playPos[m_playMode].currentFrame()
				< mixer::inst()->framesPerAudioBuffer() )
		{
			// then set played_samples to remaining samples, the 
			// rest will be played in next loop
			played_frames = frames_per_tact -
					m_playPos[m_playMode].currentFrame();
		}

		// loop through all tracks and play them if they're not muted
		for( trackVector::iterator it = tv.begin(); it != tv.end();
									++it )
		{
			if( ( *it )->muted() == FALSE )
			{
				( *it )->play( m_playPos[m_playMode],
					m_playPos[m_playMode].currentFrame(),
					played_frames, total_frames_played,
					tco_num );
			}
		}

		// update frame-counters
		total_frames_played += played_frames;
		m_playPos[m_playMode].setCurrentFrame(
					m_playPos[m_playMode].currentFrame() +
					played_frames );
		m_playPos[m_playMode].setTact64th(
					( m_playPos[m_playMode].currentFrame() *
						64 / frames_per_tact) % 64 );
	}

	if( m_playPos[m_playMode].m_timeLine != NULL &&
		m_playPos[m_playMode].m_timeLineUpdate == TRUE &&
		m_exporting == FALSE )
	{
		m_playPos[m_playMode].m_timeLine->updatePosition(); 
	}

	if( m_exporting == TRUE )
	{
		tact tacts = lengthInTacts() + 1;
		if( m_playPos[PLAY_SONG].getTact() >= tacts )
		{
			// now pause the mixer - method
			// exportProjectDialog::redrawProgressBar() which is
			// called every 100 ms will find out that export
			// is done and will act according to this
			mixer::inst()->pause();
		}
		else
		{
			m_epd->updateProgressBar(
					( m_playPos[PLAY_SONG].getTact() * 64 +
					m_playPos[PLAY_SONG].getTact64th() ) * 
						100 / ( tacts * 64 ) );
		}
	}

	if( m_playMode == PLAY_PATTERN && m_loopPattern == FALSE &&
		m_patternToPlay->isFreezing() == TRUE &&
		m_playPos[PLAY_PATTERN] > m_patternToPlay->length() )
	{
		m_patternToPlay->finishFreeze();
	}
}









void songEditor::play( void )
{
	if( m_playing == TRUE )
	{
		if( m_playMode != PLAY_SONG )
		{
			// make sure, bb-editor updates/resets it play-button
			bbEditor::inst()->stop();
			//pianoRoll::inst()->stop();
		}
		else
		{
			pause();
			return;
		}
	}
#ifdef QT4
	m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
#else
	m_playButton->setPixmap( embed::getIconPixmap( "pause" ) );
#endif
	m_actions.push_back( ACT_PLAY_SONG );
}




void songEditor::playTrack( track * _trackToPlay )
{
	if( m_playing == TRUE )
	{
		stop();
	}
	m_trackToPlay = _trackToPlay;

	m_actions.push_back( ACT_PLAY_TRACK );
}




void songEditor::playBB( void )
{
	if( m_playing == TRUE )
	{
		stop();
	}
	m_actions.push_back( ACT_PLAY_BB );
}




void songEditor::playPattern( pattern * _patternToPlay, bool _loop )
{
	if( m_playing == TRUE )
	{
		stop();
	}
	m_patternToPlay = _patternToPlay;
	m_loopPattern = _loop;
	m_actions.push_back( ACT_PLAY_PATTERN );
}




tact songEditor::lengthInTacts( void ) const
{
	tact len = 0;
	constTrackVector ctv = tracks();
	for( constTrackVector::const_iterator it = ctv.begin(); it != ctv.end();
									++it )
	{
		len = tMax( ( *it )->length(), len );
	}
	return( len );
}




void songEditor::setPlayPos( tact _tact_num, tact64th _t_64th, playModes
								_play_mode )
{
	m_playPos[_play_mode].setTact( _tact_num );
	m_playPos[_play_mode].setTact64th( _t_64th );
	m_playPos[_play_mode].setCurrentFrame( static_cast<Uint32>(
					_t_64th * framesPerTact() / 64.0f ) );
	if( _play_mode == m_playMode )
	{
		updateTimeLinePosition();
	}
}




void songEditor::updateTimeLinePosition( void )
{
	if( m_playPos[m_playMode].m_timeLine != NULL &&
		m_playPos[m_playMode].m_timeLineUpdate == TRUE )
	{
		m_playPos[m_playMode].m_timeLine->updatePosition();
	}
}




void songEditor::stop( void )
{
	m_actions.push_back( ACT_STOP_PLAY );
#ifdef QT4
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
#else
	m_playButton->setPixmap( embed::getIconPixmap( "play" ) );
#endif
	m_scrollBack = TRUE;
}






void songEditor::pause( void )
{
	m_actions.push_back( ACT_PAUSE );
#ifdef QT4
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
#else
	m_playButton->setPixmap( embed::getIconPixmap( "play" ) );
#endif
}




void songEditor::resumeFromPause( void )
{
	m_actions.push_back( ACT_RESUME_FROM_PAUSE );
#ifdef QT4
	m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
#else
	m_playButton->setPixmap( embed::getIconPixmap( "pause" ) );
#endif
}




void songEditor::startExport( void )
{
	stop();
	doActions();

	play();
	doActions();

	m_exporting = TRUE;
}




void songEditor::stopExport( void )
{
	stop();
	m_exporting = FALSE;

	// if we rendered file from cmd-line quit after export
	if( file_to_render != "" )
	{
		qApp->quit();
	}
}






void songEditor::insertTact( void )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->getTrackContentWidget()->insertTact(
							m_playPos[PLAY_SONG] );
	}
}




void songEditor::removeTact( void )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->getTrackContentWidget()->removeTact(
							m_playPos[PLAY_SONG] );
	}
}




void songEditor::addChannelTrack( void )
{
	channelTrack * t = dynamic_cast< channelTrack * >(
			track::createTrack( track::CHANNEL_TRACK, this ) );
#ifdef LMMS_DEBUG
	assert( t != NULL );
#endif
	t->loadPluginSettings( tripleOscillator::defaultSettings() );
	t->toggledChannelButton( TRUE );
	t->show();
}




void songEditor::addBBTrack( void )
{
	track * t = track::createTrack( track::BB_TRACK, this );
	if( dynamic_cast<bbTrack *>( t ) != NULL )
	{
		dynamic_cast<bbTrack *>( t )->clickedTrackLabel();
	}
}




void songEditor::addSampleTrack( void )
{
	(void) track::createTrack( track::SAMPLE_TRACK, this );
}




float songEditor::framesPerTact( void ) const
{
	return( mixer::inst()->sampleRate() * 60.0f * MAIN_BEATS_PER_TACT /
			m_bpmSpinBox->value() );
}





bool songEditor::mayChangeProject( void )
{
	if( m_modified == FALSE )
	{
		return( TRUE );
	}

/*	int answer = QMessageBox::
#if QT_VERSION >= 0x030200
		question
#else
		information
#endif
				( lmmsMainWin::inst(),
						tr( "Project not saved" ),
						tr( "The current project was "
							"modified since last "
							"saving. Do you want "
							"to save it now?" ),
						QMessageBox::Yes,
						QMessageBox::No,
						QMessageBox::Cancel );*/
	QMessageBox mb ( tr( "Project not saved" ),
				tr( "The current project was modified since "
					"last saving. Do you want to save it "
								"now?" ),
#if QT_VERSION >= 0x030200
				QMessageBox::Question,
#else
				QMessageBox::Information,
#endif
				QMessageBox::Yes,
				QMessageBox::No,
				QMessageBox::Cancel,
				lmmsMainWin::inst() );
	int answer = mb.exec();

	if( answer == QMessageBox::Yes )
	{
		return( lmmsMainWin::inst()->saveProject() );
	}
	else if( answer == QMessageBox::No )
	{
		return( TRUE );
	}

	return( FALSE );
}




void songEditor::clearProject( void )
{
	if( m_playing )
	{
		// stop play, because it's dangerous that play-routines try to
		// access non existing data (as you can see in the next lines,
		// all data is cleared!)
		stop();
		doActions();
	}

	// make sure all running notes are cleared, otherwise the whole
	// thing will end up in a SIGSEGV...
	//mixer::inst()->clear();
	while( mixer::inst()->haveNoRunningNotes() == FALSE )
	{
	}

	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		removeTrack( *it );
	}
	tv = bbEditor::inst()->tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		bbEditor::inst()->removeTrack( *it );
	}
	m_projectNotes->clear();
}





// create new file
void songEditor::createNewProject( void )
{
	clearProject();

	track * t;
	t = track::createTrack( track::CHANNEL_TRACK, this );
	dynamic_cast< channelTrack * >( t )->loadPluginSettings(
					tripleOscillator::defaultSettings() );
	track::createTrack( track::SAMPLE_TRACK, this );
	t = track::createTrack( track::CHANNEL_TRACK, bbEditor::inst() );
	dynamic_cast< channelTrack * >( t )->loadPluginSettings(
					tripleOscillator::defaultSettings() );
	track::createTrack( track::BB_TRACK, this );

	setBPM( DEFAULT_BPM );
	m_masterVolumeSlider->setValue( 100 );
	m_masterPitchSlider->setValue( 0 );

	m_fileName = m_oldFileName = "";

	m_modified = FALSE;

	lmmsMainWin::inst()->resetWindowTitle( "" );
}




void FASTCALL songEditor::createNewProjectFromTemplate( const QString &
								_template )
{
	loadProject( _template );
	// clear file-name so that user doesn't overwrite template when
	// saving...
	m_fileName = m_oldFileName = "";
}




// load given song
void FASTCALL songEditor::loadProject( const QString & _file_name )
{

	clearProject();
	m_fileName = _file_name;
	m_oldFileName = _file_name;

	multimediaProject mmp( m_fileName );
	// if file could not be opened, head-node is null and we create
	// new project
	if( mmp.head().isNull() )
	{
		createNewProject();
		return;
	}

	// get the header information from the DOM
	QDomNode node = mmp.head().firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "bpm" &&
		node.toElement().attribute( "value" ).toInt() > 0 )
			{
				setBPM( node.toElement().attribute( "value"
								).toInt() );
			}
			else if( node.nodeName() == "mastervol" )
			{
				if( node.toElement().attribute( "value"
								).toInt() > 0 )
				{
					m_masterVolumeSlider->setValue( 200 -
				node.toElement().attribute( "value" ).toInt() );
				}
				else
				{
					m_masterVolumeSlider->setValue(
						DEFAULT_VOLUME );
				}
			}
			else if( node.nodeName() == "masterpitch" )
			{
				m_masterPitchSlider->setValue(
					node.toElement().attribute( "value"
								).toInt() );
			}
		}
		node = node.nextSibling();
	}

	node = mmp.content().firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "trackcontainer" )
			{
				loadSettings( node.toElement() );
			}
			else if( node.nodeName() == m_projectNotes->nodeName() )
			{
				m_projectNotes->loadSettings(
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}

	m_modified = FALSE;
	m_leftRightScroll->setValue( 0 );

	lmmsMainWin::inst()->resetWindowTitle( "" );
}




// save current song
bool songEditor::saveProject( void )
{
	multimediaProject mmp( multimediaProject::SONG_PROJECT );

	QDomElement bpm = mmp.createElement( "bpm" );
	bpm.setAttribute( "value", QString::number( m_bpmSpinBox->value() ) );
	mmp.head().appendChild( bpm );

	QDomElement mv = mmp.createElement( "mastervol" );
	mv.setAttribute( "value", QString::number( 200 -
					m_masterVolumeSlider->value() ) );
	mmp.head().appendChild( mv );

	QDomElement mp = mmp.createElement( "masterpitch" );
	mp.setAttribute( "value", QString::number(
					m_masterPitchSlider->value() ) );
	mmp.head().appendChild( mp );


	saveSettings( mmp, mmp.content() );
	m_projectNotes->saveSettings( mmp, mmp.content() );

	if( mmp.writeFile( m_fileName, m_oldFileName == "" ||
					m_fileName != m_oldFileName ) == TRUE )
	{
		m_modified = FALSE;

		lmmsMainWin::inst()->statusBar()->showMessage(
					tr( "%1 saved." ).arg( m_fileName ),
									3000 );
		lmmsMainWin::inst()->resetWindowTitle( "" );
	}
	else
	{
		lmmsMainWin::inst()->statusBar()->showMessage(
					tr( "Project NOT saved." ), 3000 );
		return( FALSE );
	}
	return( TRUE );
}




// save current song in given filename
bool FASTCALL songEditor::saveProjectAs( const QString & _file_name )
{
	QString o = m_oldFileName;
	m_oldFileName = m_fileName;
	m_fileName = _file_name;
	if( saveProject() == FALSE )
	{
		m_fileName = m_oldFileName;
		m_oldFileName = o;
		return( FALSE );
	}
	m_oldFileName = m_fileName;
	return( TRUE );
}



void songEditor::importProject( void )
{
#ifdef QT4
		QFileDialog ofd( this, tr( "Import file" ), "",
					tr( "MIDI-files (*.mid)" ) );
#else
		QFileDialog ofd( QString::null,
					tr( "MIDI-files (*.mid)" ),
							this, "", TRUE );
		ofd.setWindowTitle( tr( "Import file" ) );
#endif
		ofd.setDirectory( configManager::inst()->projectsDir() );
		ofd.setFileMode( QFileDialog::ExistingFiles );
		if( ofd.exec () == QDialog::Accepted &&
						!ofd.selectedFiles().isEmpty() )
		{
			midiFile mf( ofd.selectedFiles()[0] );
			mf.importToTrackContainer( this );
		}
}




void songEditor::exportProject( void )
{
	QString base_filename;

	if( m_fileName != "" )
	{
#ifdef QT4
		base_filename = QFileInfo( m_fileName ).absolutePath() + "/" +
					QFileInfo( m_fileName
							).completeBaseName();
#else
		base_filename = QFileInfo( m_fileName ).dirPath() + "/" +
					QFileInfo( m_fileName ).baseName(
									TRUE );
#endif
	}
	else
	{
		base_filename = tr( "untitled" );
	}
 	base_filename += fileEncodeDevices[0].m_extension;

	QFileDialog efd( lmmsMainWin::inst() );
	efd.setFileMode( QFileDialog::AnyFile );

	int idx = 0;
#ifdef QT4
	QStringList types;
	while( fileEncodeDevices[idx].m_fileType != NULL_FILE )
	{
		types << tr( fileEncodeDevices[idx].m_description );
		++idx;
	}
	efd.setFilters( types );
	efd.selectFile( base_filename );
#else
	while( fileEncodeDevices[idx].m_fileType != NULL_FILE )
	{
		efd.addFilter( tr( fileEncodeDevices[idx].m_description ) );
		++idx;
	}
	efd.setSelectedFilter( tr( fileEncodeDevices[0].m_description ) );
	efd.setSelection( base_filename );
#endif
	efd.setWindowTitle( tr( "Select file for project-export..." ) );

	if( efd.exec() == QDialog::Accepted &&
#ifdef QT4
		!efd.selectedFiles().isEmpty() && efd.selectedFiles()[0] != ""
#else
		efd.selectedFile() != ""
#endif
		)
	{
#ifdef QT4
		const QString export_file_name = efd.selectedFiles()[0];
#else
		const QString export_file_name = efd.selectedFile();
#endif
		if( QFileInfo( export_file_name ).exists() == TRUE &&
			QMessageBox::warning( lmmsMainWin::inst(),
						tr( "File already exists" ),
						tr( "The file \"%1\" already "
							"exists. Do you want "
							"to overwrite it?"
						).arg( QFileInfo(
						export_file_name ).fileName() ),
						QMessageBox::Yes,
							QMessageBox::No |
							QMessageBox::Escape |
							QMessageBox::Default )
			== QMessageBox::No )
		{
			return;
		}

		m_epd = new exportProjectDialog( export_file_name,
							lmmsMainWin::inst() );
		m_epd->exec();
		delete m_epd;
		m_epd = NULL;
	}
}







void songEditor::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	trackContainer::saveSettings( _doc, _parent );
}




void songEditor::loadSettings( const QDomElement & _this )
{
	trackContainer::loadSettings( _this );
}



#include "song_editor.moc"

