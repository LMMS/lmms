#ifndef SINGLE_SOURCE_COMPILE

/*
 * song_editor.cpp - basic window for editing song
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QStatusBar>

#else

#include <qapplication.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qlabel.h>
#include <qstatusbar.h>
#include <qlayout.h>
#include <qbuttongroup.h>

#define addButton insert
#define setCheckable setToggleButton

#endif


#include "song_editor.h"
#include "automatable_slider.h"
#include "bb_editor.h"
#include "rename_dialog.h"
#include "embed.h"
#include "templates.h"
#include "export_project_dialog.h"
#include "bb_track.h"
#include "instrument_track.h"
#include "mmp.h"
#include "midi_client.h"
#include "timeline.h"
#include "pattern.h"
#include "piano_roll.h"
#include "envelope_and_lfo_widget.h"
#include "visualization_widget.h"
#include "project_notes.h"
#include "config_mgr.h"
#include "lcd_spinbox.h"
#include "tooltip.h"
#include "tool_button.h"
#include "cpuload_widget.h"
#include "text_float.h"
#include "combobox.h"
#include "main_window.h"
#include "import_filter.h"
#include "project_journal.h"

#include "debug.h"




songEditor::songEditor( engine * _engine ) :
	trackContainer( _engine ),
	m_fileName( "" ),
	m_oldFileName( "" ),
	m_exporting( FALSE ),
	m_playing( FALSE ),
	m_paused( FALSE ),
	m_loadingProject( FALSE ),
	m_playMode( PLAY_SONG ),
	m_trackToPlay( NULL ),
	m_patternToPlay( NULL ),
	m_loopPattern( FALSE ),
	m_scrollBack( FALSE )
{
	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );

	QWidget * w = ( parentWidget() != NULL ) ? parentWidget() : this;
	if( eng()->getMainWindow()->workspace() != NULL )
	{
		resize( 680, 300 );
		w->move( 10, 10 );
	}
	else
	{
		resize( 580, 300 );
		w->move( 210, 10 );
	}

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
					DEFAULT_SETTINGS_WIDGET_WIDTH, 32,
					pixelsPerTact(), m_playPos[PLAY_SONG],
					m_currentPosition, cw, eng() );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				m_playPos[PLAY_SONG].m_timeLine,
			SLOT( updatePosition( const midiTime & ) ) );
	connect( tl, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );

	m_automation_track = track::create( track::AUTOMATION_TRACK, this );

	// add some essential widgets to global tool-bar 
	QWidget * tb = eng()->getMainWindow()->toolBar();

	eng()->getMainWindow()->addSpacingToToolBar( 10 );

	m_bpmSpinBox = new lcdSpinBox( MIN_BPM, MAX_BPM, 3, tb, tr( "Tempo" ),
						eng(), m_automation_track );
	m_bpmSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	connect( m_bpmSpinBox, SIGNAL( valueChanged( int ) ), this,
						SLOT( setTempo( int ) ) );
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

	int col = eng()->getMainWindow()->addWidgetToToolBar( m_bpmSpinBox, 0 );


	toolButton * hq_btn = new toolButton( embed::getIconPixmap( "hq_mode" ),
						tr( "High quality mode" ),
						NULL, NULL, tb );
	hq_btn->setCheckable( TRUE );
	connect( hq_btn, SIGNAL( toggled( bool ) ), eng()->getMixer(),
					SLOT( setHighQuality( bool ) ) );
	hq_btn->setFixedWidth( 42 );
	eng()->getMainWindow()->addWidgetToToolBar( hq_btn, 1, col );


	toolButton * cp_btn = new toolButton( embed::getIconPixmap( "auto_limit" ),
					      tr( "Auto limiter" ),
					      NULL, NULL, tb );
	cp_btn->setCheckable( TRUE );
	connect( cp_btn, SIGNAL( toggled( bool ) ), eng()->getMixer(),
		 SLOT( setClipScaling( bool ) ) );
	cp_btn->setFixedWidth( 30 );
	eng()->getMainWindow()->addWidgetToToolBar( cp_btn, 1, col + 1 );


	eng()->getMainWindow()->addSpacingToToolBar( 10 );



	QLabel * master_vol_lbl = new QLabel( tb );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_masterVolumeSlider = new automatableSlider( tb, tr( "Master volume" ),
						eng(), m_automation_track );
	m_masterVolumeSlider->setOrientation( Vertical );
	m_masterVolumeSlider->setRange( 0, 200 );
	m_masterVolumeSlider->setPageStep( 1 );
	m_masterVolumeSlider->setInitValue( 100 );
#ifdef QT4
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
#else
	m_masterVolumeSlider->setTickPosition( QSlider::Left );
#endif
	m_masterVolumeSlider->setFixedSize( 26, 60 );
	m_masterVolumeSlider->setTickInterval( 50 );
	toolTip::add( m_masterVolumeSlider, tr( "master volume" ) );

	connect( m_masterVolumeSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( masterVolumeChanged( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterVolumePressed() ) );
	connect( m_masterVolumeSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( masterVolumeMoved( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterVolumeReleased() ) );

	m_mvsStatus = new textFloat( m_masterVolumeSlider );
	m_mvsStatus->setTitle( tr( "Master volume" ) );
	m_mvsStatus->setPixmap( embed::getIconPixmap( "master_volume" ) );

	eng()->getMainWindow()->addWidgetToToolBar( master_vol_lbl );
	eng()->getMainWindow()->addWidgetToToolBar( m_masterVolumeSlider );


	eng()->getMainWindow()->addSpacingToToolBar( 10 );

	QLabel * master_pitch_lbl = new QLabel( tb );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );
	master_pitch_lbl->setFixedHeight( 64 );

	m_masterPitchSlider = new automatableSlider( tb, tr( "Master pitch" ),
						eng(), m_automation_track );
	m_masterPitchSlider->setOrientation( Vertical );
	m_masterPitchSlider->setRange( -12, 12 );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setInitValue( 0 );
#ifdef QT4
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
#else
	m_masterPitchSlider->setTickPosition( QSlider::Left );
#endif
	m_masterPitchSlider->setFixedSize( 26, 60 );
	m_masterPitchSlider->setTickInterval( 12 );
	toolTip::add( m_masterPitchSlider, tr( "master pitch" ) );
	connect( m_masterPitchSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( masterPitchChanged( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderPressed() ), this,
			SLOT( masterPitchPressed() ) );
	connect( m_masterPitchSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( masterPitchMoved( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderReleased() ), this,
			SLOT( masterPitchReleased() ) );

	m_mpsStatus = new textFloat( m_masterPitchSlider );
	m_mpsStatus->setTitle( tr( "Master pitch" ) );
	m_mpsStatus->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	eng()->getMainWindow()->addWidgetToToolBar( master_pitch_lbl );
	eng()->getMainWindow()->addWidgetToToolBar( m_masterPitchSlider );

	eng()->getMainWindow()->addSpacingToToolBar( 10 );

	// create widget for visualization- and cpu-load-widget
	QWidget * vc_w = new QWidget( tb );
	QVBoxLayout * vcw_layout = new QVBoxLayout( vc_w );
	vcw_layout->setMargin( 0 );
	vcw_layout->setSpacing( 0 );

	//vcw_layout->addStretch();
	vcw_layout->addWidget( new visualizationWidget(
			embed::getIconPixmap( "output_graph" ), vc_w, eng() ) );

	vcw_layout->addWidget( new cpuloadWidget( vc_w, eng() ) );
	vcw_layout->addStretch();

	eng()->getMainWindow()->addWidgetToToolBar( vc_w );


	// create own toolbar
	m_toolBar = new QWidget( cw );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
#ifdef QT4
	m_toolBar->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(), 
				embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );
#else
	m_toolBar->setPaletteBackgroundPixmap( embed::getIconPixmap(
							"toolbar_bg" ) );
#endif

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );
	tb_layout->setSpacing( 0 );

#ifdef QT4
	containerWidget()->setParent( cw );
#else
	containerWidget()->reparent( cw, 0, QPoint( 0, 0 ) );
#endif
	containerWidget()->move( 0, m_toolBar->height() + tl->height() );



	// fill own tool-bar
	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
					tr( "Play song (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
					tr( "Stop song (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_addBBTrackButton = new toolButton( embed::getIconPixmap(
						"add_bb_track" ),
						tr( "Add beat/bassline" ),
						this, SLOT( addBBTrack() ),
						m_toolBar );

	m_addSampleTrackButton = new toolButton( embed::getIconPixmap(
					"add_sample_track" ),
					tr( "Add sample-track" ),
					this, SLOT( addSampleTrack() ),
					m_toolBar );

	m_drawModeButton = new toolButton( embed::getIconPixmap(
								"edit_draw" ),
							tr( "Draw mode" ),
							NULL, NULL, m_toolBar );
	m_drawModeButton->setCheckable( TRUE );
	m_drawModeButton->setChecked( TRUE );

	m_editModeButton = new toolButton( embed::getIconPixmap(
								"edit_arrow" ),
					tr( "Edit mode (select and move)" ),
							NULL, NULL, m_toolBar );
	m_editModeButton->setCheckable( TRUE );

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawModeButton );
	tool_button_group->addButton( m_editModeButton );
	tool_button_group->setExclusive( TRUE );
#ifndef QT4
	tool_button_group->hide();
#endif


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
	m_zoomingComboBox = new comboBox( m_toolBar, eng() );
	m_zoomingComboBox->setFixedSize( 80, 22 );
	m_zoomingComboBox->move( 580, 4 );
	for( int i = 0; i < 7; ++i )
	{
		m_zoomingComboBox->addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingComboBox->setInitValue( m_zoomingComboBox->findText(
								"100%" ) );
	connect( m_zoomingComboBox, SIGNAL( activated( const QString & ) ),
			this, SLOT( zoomingChanged( const QString & ) ) );


	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_addBBTrackButton );
	tb_layout->addWidget( m_addSampleTrackButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_drawModeButton );
	tb_layout->addWidget( m_editModeButton );
	tb_layout->addSpacing( 10 );
	tl->addToolButtons( m_toolBar );
	tb_layout->addSpacing( 15 );
	tb_layout->addWidget( zoom_lbl );
	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_zoomingComboBox );
	tb_layout->addStretch();


	m_leftRightScroll = new QScrollBar( Qt::Horizontal, cw );
	m_leftRightScroll->setMinimum( 0 );
	m_leftRightScroll->setMaximum( 0 );
#ifndef QT3
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( 20 );
#else
	m_leftRightScroll->setSteps( 1, 20 );
#endif
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ),
					this, SLOT( scrolled( int ) ) );


	show();

}




songEditor::~songEditor()
{
	delete m_automation_track;
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




QRect songEditor::scrollAreaRect( void ) const
{
	if( centralWidget() == NULL )
	{
		return( rect() );
	}
	return( QRect( 0, 0, centralWidget()->width(),
			centralWidget()->height() - m_toolBar->height() -
			m_playPos[PLAY_SONG].m_timeLine->height() -
			DEFAULT_SCROLLBAR_SIZE ) );
}




// responsible for moving scrollbars after resizing
void songEditor::resizeEvent( QResizeEvent * _re )
{
	if( centralWidget() != NULL )
	{
		m_leftRightScroll->setGeometry( 0,
					centralWidget()->height() -
							DEFAULT_SCROLLBAR_SIZE,
					centralWidget()->width(),
					DEFAULT_SCROLLBAR_SIZE );

		m_playPos[PLAY_SONG].m_timeLine->setFixedWidth(
						centralWidget()->width() );
		m_toolBar->setFixedWidth( centralWidget()->width() );
	}
	trackContainer::resizeEvent( _re );
}




void songEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( /*_ke->modifiers() & Qt::ShiftModifier*/
		eng()->getMainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Insert )
	{
		insertBar();
	}
	else if(/* _ke->modifiers() & Qt::ShiftModifier &&*/
			eng()->getMainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Delete )
	{
		removeBar();
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
		QWidget::keyPressEvent( _ke );
	}
}





void songEditor::scrolled( int _new_pos )
{
	update();
	emit positionChanged( m_currentPosition = midiTime( _new_pos, 0 ) );
}




void songEditor::wheelEvent( QWheelEvent * _we )
{
	if( eng()->getMainWindow()->isCtrlPressed() == TRUE )
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
		// update combobox with zooming-factor
		m_zoomingComboBox->setValue(
				m_zoomingComboBox->findText( QString::number(
					static_cast<int>( pixelsPerTact() *
				100 / DEFAULT_PIXELS_PER_TACT ) ) + "%" ) );
		// update timeline
		m_playPos[PLAY_SONG].m_timeLine->setPixelsPerTact(
							pixelsPerTact() );
		// and make sure, all TCO's are resized and relocated
		realignTracks( TRUE );
	} 
	else if( eng()->getMainWindow()->isShiftPressed() == TRUE )
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




void songEditor::masterVolumeChanged( int _new_val )
{
	masterVolumeMoved( _new_val );
	if( m_mvsStatus->isVisible() == FALSE && m_loadingProject == FALSE
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->reparent( m_masterVolumeSlider );
		m_mvsStatus->move( m_masterVolumeSlider->mapTo(
					m_masterVolumeSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	eng()->getMixer()->setMasterGain( _new_val / 100.0f );
	setModified();
}




void songEditor::masterVolumePressed( void )
{
	m_mvsStatus->reparent( m_masterVolumeSlider );
	m_mvsStatus->move( m_masterVolumeSlider->mapTo(
					m_masterVolumeSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	masterVolumeMoved( m_masterVolumeSlider->logicValue() );
}




void songEditor::masterVolumeMoved( int _new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( _new_val ) );
}




void songEditor::masterVolumeReleased( void )
{
	m_mvsStatus->hide();
}




void songEditor::masterPitchChanged( int _new_val )
{
	masterPitchMoved( _new_val );
	if( m_mpsStatus->isVisible() == FALSE && m_loadingProject == FALSE
					&& m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->reparent( m_masterPitchSlider );
		m_mpsStatus->move( m_masterPitchSlider->mapTo(
					m_masterPitchSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
	setModified();
}




void songEditor::masterPitchPressed( void )
{
	m_mpsStatus->reparent( m_masterPitchSlider );
	m_mpsStatus->move( m_masterPitchSlider->mapTo(
					m_masterPitchSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	masterPitchMoved( m_masterPitchSlider->logicValue() );
}




void songEditor::masterPitchMoved( int _new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones").arg( _new_val ) );

}




void songEditor::masterPitchReleased( void )
{
	m_mpsStatus->hide();
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




void songEditor::zoomingChanged( const QString & _zfac )
{
	setPixelsPerTact( _zfac.left( _zfac.length() - 1 ).toInt() *
					DEFAULT_PIXELS_PER_TACT / 100 );
	m_playPos[PLAY_SONG].m_timeLine->setPixelsPerTact( pixelsPerTact() );
	realignTracks( TRUE );
}




void songEditor::setTempo( int _new_bpm )
{
	m_bpmSpinBox->setInitValue(
				tLimit<bpm_t>( _new_bpm, MIN_BPM, MAX_BPM ) );
	setModified();
	emit tempoChanged( _new_bpm );
}




void songEditor::setMasterVolume( volume _vol )
{
	m_masterVolumeSlider->setInitValue( _vol );
}




void songEditor::setMasterPitch( int _master_pitch )
{
	m_masterPitchSlider->setInitValue( _master_pitch );
}




int songEditor::masterPitch( void ) const
{
	return( m_masterPitchSlider->logicValue() );
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
				eng()->getMixer()->clear();

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
		// anything (need pos for restoring it later in certain
		// timeline-modes)
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
				envelopeAndLFOWidget::resetLFO( eng() );
			}
			break;

		case PLAY_TRACK:
			tv.push_back( m_trackToPlay );
			break;

		case PLAY_BB:
			if( eng()->getBBEditor()->numOfBBs() > 0 )
			{
				tco_num = eng()->getBBEditor()->currentBB();
				tv.push_back( bbTrack::findBBTrack( tco_num,
								eng() ) );
			}
			break;

		case PLAY_PATTERN:
			if( m_patternToPlay != NULL )
			{
				tco_num = m_patternToPlay->getTrack()->getTCONum(
							m_patternToPlay );
				tv.push_back( m_patternToPlay->getTrack() );
			}
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
	if( tl != NULL && m_exporting == FALSE && tl->loopPointsEnabled() &&
		!( m_playMode == PLAY_PATTERN &&
		  		m_patternToPlay->freezing() == TRUE ) )
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

	f_cnt_t total_frames_played = 0;
	f_cnt_t frames_per_tact = static_cast<f_cnt_t>( framesPerTact() );
	if( m_playPos[m_playMode].currentFrame() == 0 &&
		m_playPos[m_playMode].getTact64th() > 0 )
	{
		m_playPos[m_playMode].setCurrentFrame(
					m_playPos[m_playMode].getTact64th() *
							frames_per_tact / 64 );
	}

	while( total_frames_played < eng()->getMixer()->framesPerAudioBuffer() )
	{
		f_cnt_t played_frames = eng()->getMixer()->framesPerAudioBuffer() -
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
					eng()->getBBEditor()->lengthOfCurrentBB();
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
				< eng()->getMixer()->framesPerAudioBuffer() )
		{
			// then set played_samples to remaining samples, the 
			// rest will be played in next loop
			played_frames = frames_per_tact -
					m_playPos[m_playMode].currentFrame();
		}

		m_automation_track->play( m_playPos[m_playMode],
					m_playPos[m_playMode].currentFrame(),
					played_frames, total_frames_played,
					tco_num );

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
		m_playPos[m_playMode].setCurrentFrame( played_frames +
					m_playPos[m_playMode].currentFrame() );
		m_playPos[m_playMode].setTact64th(
					( m_playPos[m_playMode].currentFrame() *
						64 / frames_per_tact) % 64 );
	}

	if( m_exporting == FALSE )
	{
		updateTimeLinePosition();
	}
}




bool songEditor::realTimeTask( void ) const
{
	return( !( m_exporting == TRUE || ( m_playMode == PLAY_PATTERN &&
		  	m_patternToPlay != NULL &&
			m_patternToPlay->freezing() == TRUE ) ) );
}




void songEditor::play( void )
{
	if( m_playing == TRUE )
	{
		if( m_playMode != PLAY_SONG )
		{
			// make sure, bb-editor updates/resets it play-button
			eng()->getBBEditor()->stop();
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
	if( m_patternToPlay != NULL )
	{
		m_actions.push_back( ACT_PLAY_PATTERN );
	}
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
	m_playPos[_play_mode].setCurrentFrame( static_cast<f_cnt_t>(
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
/*		QTimer::singleShot( 1, m_playPos[m_playMode].m_timeLine,
						SLOT( updatePosition() ) );*/
		//m_playPos[m_playMode].m_timeLine->updatePosition();
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
}






void songEditor::insertBar( void )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->getTrackContentWidget()->insertTact(
							m_playPos[PLAY_SONG] );
	}
}




void songEditor::removeBar( void )
{
	trackVector tv = tracks();
	for( trackVector::iterator it = tv.begin(); it != tv.end(); ++it )
	{
		( *it )->getTrackContentWidget()->removeTact(
							m_playPos[PLAY_SONG] );
	}
}




void songEditor::addBBTrack( void )
{
	track * t = track::create( track::BB_TRACK, this );
	if( dynamic_cast<bbTrack *>( t ) != NULL )
	{
		dynamic_cast<bbTrack *>( t )->clickedTrackLabel();
	}
}




void songEditor::addSampleTrack( void )
{
	(void) track::create( track::SAMPLE_TRACK, this );
}




float songEditor::framesPerTact( void ) const
{
	// when fooling around with tempo while playing, we sometimes get
	// 0 here which leads to FP-exception, so handle it separately
	const int bpm = tMax( 1, m_bpmSpinBox->value() );
	return( eng()->getMixer()->sampleRate() * 60.0f * BEATS_PER_TACT /
								bpm );
}




bpm_t songEditor::getTempo( void )
{
	return( m_bpmSpinBox->value() );
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
				( eng()->getMainWindow(),
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
				eng()->getMainWindow() );
	int answer = mb.exec();

	if( answer == QMessageBox::Yes )
	{
		return( eng()->getMainWindow()->saveProject() );
	}
	else if( answer == QMessageBox::No )
	{
		return( TRUE );
	}

	return( FALSE );
}




void songEditor::clearProject( void )
{
	eng()->getProjectJournal()->setJournalling( FALSE );

	if( m_playing )
	{
		// stop play, because it's dangerous that play-routines try to
		// access non existing data (as you can see in the next lines,
		// all data is cleared!)
		stop();
	}

	// make sure all running notes are cleared, otherwise the whole
	// thing will end up in a SIGSEGV...
	eng()->getMixer()->clear( TRUE );
	while( eng()->getMixer()->haveNoRunningNotes() == FALSE )
	{
#ifdef QT4
		QApplication::processEvents( QEventLoop::AllEvents );
#else
		qApp->processEvents();
#endif
	}

	clearAllTracks();

	eng()->getAutomationEditor()->setCurrentPattern( NULL );
	m_bpmSpinBox->getAutomationPattern()->clearValues();
	m_masterVolumeSlider->clearAutomationValues();
	m_masterPitchSlider->clearAutomationValues();

	eng()->getBBEditor()->clearAllTracks();

	eng()->getProjectNotes()->clear();

	eng()->getProjectJournal()->clearInvalidJournallingObjects();
	eng()->getProjectJournal()->clearJournal();

	eng()->getProjectJournal()->setJournalling( TRUE );
}





// create new file
void songEditor::createNewProject( void )
{
	clearProject();

	eng()->getProjectJournal()->setJournalling( FALSE );

	track * t;
	t = track::create( track::CHANNEL_TRACK, this );
	dynamic_cast< instrumentTrack * >( t )->loadInstrument(
					"tripleoscillator" );
	track::create( track::SAMPLE_TRACK, this );
	t = track::create( track::CHANNEL_TRACK, eng()->getBBEditor() );
	dynamic_cast< instrumentTrack * >( t )->loadInstrument(
						"tripleoscillator" );
	track::create( track::BB_TRACK, this );

	m_loadingProject = TRUE;
	setTempo( DEFAULT_BPM );
	m_masterVolumeSlider->setInitValue( 100 );
	m_masterPitchSlider->setInitValue( 0 );
	m_loadingProject = FALSE;

	m_fileName = m_oldFileName = "";

	m_modified = FALSE;

	eng()->getMainWindow()->resetWindowTitle( "" );

	eng()->getProjectJournal()->setJournalling( TRUE );

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

	eng()->getProjectJournal()->setJournalling( FALSE );

	m_fileName = _file_name;
	m_oldFileName = _file_name;

	m_loadingProject = TRUE;

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
				setTempo( node.toElement().attribute( "value"
								).toInt() );
			}
			else if( node.nodeName() == "mastervol" )
			{
				if( node.toElement().attribute( "value"
								).toInt() > 0 )
				{
					m_masterVolumeSlider->setInitValue(
						node.toElement().attribute(
							"value" ).toInt() );
				}
				else
				{
					m_masterVolumeSlider->setInitValue(
							DEFAULT_VOLUME );
				}
			}
			else if( node.nodeName() == "masterpitch" )
			{
				m_masterPitchSlider->setInitValue(
					-node.toElement().attribute( "value"
								).toInt() );
			}
			else if( node.nodeName()
					== automationPattern::classNodeName() )
			{
				m_bpmSpinBox->loadSettings( mmp.head(), "bpm" );
				m_masterVolumeSlider->loadSettings( mmp.head(),
								"mastervol" );
				m_masterPitchSlider->loadSettings( mmp.head(),
								"masterpitch" );
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
				( (journallingObject *)( this ) )->
					restoreState( node.toElement() );
			}
			else if( node.nodeName() ==
					eng()->getPianoRoll()->nodeName() )
			{
				eng()->getPianoRoll()->restoreState(
							node.toElement() );
			}
			else if( node.nodeName() ==
				eng()->getAutomationEditor()->nodeName() )
			{
				eng()->getAutomationEditor()->restoreState(
							node.toElement() );
			}
			else if( node.nodeName() ==
					eng()->getProjectNotes()->nodeName() )
			{
				( (journallingObject *)( eng()->
							getProjectNotes() ) )->
					restoreState( node.toElement() );
			}
			else if( node.nodeName() ==
				m_playPos[PLAY_SONG].m_timeLine->nodeName() )
			{
				m_playPos[PLAY_SONG].m_timeLine->restoreState(
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}

	m_modified = FALSE;
	m_leftRightScroll->setValue( 0 );

	m_loadingProject = FALSE;

	eng()->getMainWindow()->resetWindowTitle( "" );

	eng()->getProjectJournal()->setJournalling( TRUE );
}




// save current song
bool songEditor::saveProject( void )
{
	multimediaProject mmp( multimediaProject::SONG_PROJECT );

	m_bpmSpinBox->saveSettings( mmp, mmp.head(), "bpm" );
	m_masterVolumeSlider->saveSettings( mmp, mmp.head(), "mastervol" );
	m_masterPitchSlider->saveSettings( mmp, mmp.head(), "masterpitch" );


	( (journallingObject *)( this ) )->saveState( mmp, mmp.content() );

	eng()->getPianoRoll()->saveState( mmp, mmp.content() );
	eng()->getAutomationEditor()->saveState( mmp, mmp.content() );
	( (journallingObject *)( eng()->getProjectNotes() ) )->saveState( mmp,
								mmp.content() );
	m_playPos[PLAY_SONG].m_timeLine->saveState( mmp, mmp.content() );

	if( mmp.writeFile( m_fileName, m_oldFileName == "" ||
					m_fileName != m_oldFileName ) == TRUE )
	{
		m_modified = FALSE;

		textFloat::displayMessage( tr( "Project saved" ),
					tr( "The project %1 is now saved."
							).arg( m_fileName ),
				embed::getIconPixmap( "project_save", 24, 24 ),
									2000 );
		eng()->getMainWindow()->resetWindowTitle( "" );
	}
	else
	{
		textFloat::displayMessage( tr( "Project NOT saved." ),
				tr( "The project %1 was not saved!" ).arg(
							m_fileName ),
				embed::getIconPixmap( "error" ), 4000 );
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
	QFileDialog ofd( this, tr( "Import file" ), ""/*,
					tr( "MIDI-files (*.mid)" )*/ );
#else
	QFileDialog ofd( QString::null,/* tr( "MIDI-files (*.mid)" )*/ QString::null,
							this, "", TRUE );
	ofd.setWindowTitle( tr( "Import file" ) );
#endif
	ofd.setDirectory( configManager::inst()->userProjectsDir() );
	ofd.setFileMode( QFileDialog::ExistingFiles );
	if( ofd.exec () == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		importFilter::import( ofd.selectedFiles()[0], this );
	}
}




void songEditor::exportProject( void )
{
	QString base_filename;

	if( m_fileName != "" )
	{
		base_filename = baseName( m_fileName );
	}
	else
	{
		base_filename = tr( "untitled" );
	}
 	base_filename += fileEncodeDevices[0].m_extension;

	QFileDialog efd( eng()->getMainWindow() );
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
			QMessageBox::warning( eng()->getMainWindow(),
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
		exportProjectDialog epd( export_file_name,
						eng()->getMainWindow(), eng() );
		epd.exec();
	}
}



#include "song_editor.moc"


#ifdef QT3
#undef addButton
#undef setCheckable
#endif


#endif
