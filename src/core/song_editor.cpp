#ifndef SINGLE_SOURCE_COMPILE

/*
 * song_editor.cpp - basic window for song-editing
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "song_editor.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>


#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QAction>
#include <QtGui/QButtonGroup>
#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QMessageBox>
#include <QtGui/QScrollBar>
#include <QtGui/QStatusBar>


#include "automatable_slider.h"
#include "bb_editor.h"
#include "bb_track.h"
#include "combobox.h"
#include "config_mgr.h"
#include "cpuload_widget.h"
#include "embed.h"
#include "envelope_and_lfo_parameters.h"
#include "export_project_dialog.h"
#include "import_filter.h"
#include "instrument_track.h"
#include "lcd_spinbox.h"
#include "main_window.h"
#include "midi_client.h"
#include "mmp.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "piano_roll.h"
#include "project_journal.h"
#include "project_notes.h"
#include "rename_dialog.h"
#include "templates.h"
#include "text_float.h"
#include "timeline.h"
#include "tool_button.h"
#include "tooltip.h"
#include "visualization_widget.h"




songEditor::songEditor( song * _song ) :
	trackContainerView( _song ),
	m_s( _song ),
	m_scrollBack( FALSE )
{
	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	// create time-line
	timeLine * tl = new timeLine( TRACK_OP_WIDTH +
					DEFAULT_SETTINGS_WIDGET_WIDTH, 32,
					pixelsPerTact(),
					m_s->m_playPos[song::Mode_PlaySong],
					m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
				m_s->m_playPos[song::Mode_PlaySong].m_timeLine,
			SLOT( updatePosition( const midiTime & ) ) );
	connect( tl, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );


	// add some essential widgets to global tool-bar 
	QWidget * tb = engine::getMainWindow()->toolBar();

	engine::getMainWindow()->addSpacingToToolBar( 10 );

	m_tempoSpinBox = new lcdSpinBox( 3, tb, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &m_s->m_tempoModel );
	m_tempoSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	toolTip::add( m_tempoSpinBox, tr( "tempo of song" ) );

	m_tempoSpinBox->setWhatsThis(
		tr( "The tempo of a song is specified in beats per minute "
			"(BPM). If you want to change the tempo of your "
			"song, change this value. Every tact has four beats, "
			"so the tempo in BPM specifies, how many tacts / 4 "
			"should be played within a minute (or how many tacts "
			"should be played within four minutes)." ) );

	int col = engine::getMainWindow()->addWidgetToToolBar( m_tempoSpinBox,
									0 );


	toolButton * hq_btn = new toolButton( embed::getIconPixmap( "hq_mode" ),
						tr( "High quality mode" ),
						NULL, NULL, tb );
	hq_btn->setCheckable( TRUE );
	connect( hq_btn, SIGNAL( toggled( bool ) ), engine::getMixer(),
					SLOT( setHighQuality( bool ) ) );
	hq_btn->setFixedWidth( 42 );
	engine::getMainWindow()->addWidgetToToolBar( hq_btn, 1, col );


	toolButton * cp_btn = new toolButton(
					embed::getIconPixmap( "auto_limit" ),
					      tr( "Auto limiter" ),
					      NULL, NULL, tb );
	cp_btn->setCheckable( TRUE );
	connect( cp_btn, SIGNAL( toggled( bool ) ), engine::getMixer(),
		 SLOT( setClipScaling( bool ) ) );
	cp_btn->setFixedWidth( 30 );
	engine::getMainWindow()->addWidgetToToolBar( cp_btn, 1, col + 1 );


	engine::getMainWindow()->addSpacingToToolBar( 10 );

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ), this,
					SLOT( updateFramesPerTact64th() ) );



	QLabel * master_vol_lbl = new QLabel( tb );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_masterVolumeSlider = new automatableSlider( tb,
							tr( "Master volume" ) );
	m_masterVolumeSlider->setModel( &m_s->m_masterVolumeModel );
	m_masterVolumeSlider->setOrientation( Qt::Vertical );
	m_masterVolumeSlider->setPageStep( 1 );
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
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

	engine::getMainWindow()->addWidgetToToolBar( master_vol_lbl );
	engine::getMainWindow()->addWidgetToToolBar( m_masterVolumeSlider );


	engine::getMainWindow()->addSpacingToToolBar( 10 );


	QLabel * master_pitch_lbl = new QLabel( tb );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );
	master_pitch_lbl->setFixedHeight( 64 );

	m_masterPitchSlider = new automatableSlider( tb, tr( "Master pitch" ) );
	m_masterPitchSlider->setModel( &m_s->m_masterPitchModel );
	m_masterPitchSlider->setOrientation( Qt::Vertical );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
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

	engine::getMainWindow()->addWidgetToToolBar( master_pitch_lbl );
	engine::getMainWindow()->addWidgetToToolBar( m_masterPitchSlider );

	engine::getMainWindow()->addSpacingToToolBar( 10 );

	// create widget for visualization- and cpu-load-widget
	QWidget * vc_w = new QWidget( tb );
	QVBoxLayout * vcw_layout = new QVBoxLayout( vc_w );
	vcw_layout->setMargin( 0 );
	vcw_layout->setSpacing( 0 );

	//vcw_layout->addStretch();
	vcw_layout->addWidget( new visualizationWidget(
			embed::getIconPixmap( "output_graph" ), vc_w ) );

	vcw_layout->addWidget( new cpuloadWidget( vc_w ) );
	vcw_layout->addStretch();

	engine::getMainWindow()->addWidgetToToolBar( vc_w );


	// create own toolbar
	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(), 
				embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );

	static_cast<QVBoxLayout *>( layout() )->insertWidget( 0, m_toolBar );
	static_cast<QVBoxLayout *>( layout() )->insertWidget( 1, tl );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );
	tb_layout->setSpacing( 0 );


	// fill own tool-bar
	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
					tr( "Play song (Space)" ),
					m_s, SLOT( play() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
					tr( "Stop song (Space)" ),
					m_s, SLOT( stop() ), m_toolBar );

	m_addBBTrackButton = new toolButton( embed::getIconPixmap(
						"add_bb_track" ),
						tr( "Add beat/bassline" ),
						m_s, SLOT( addBBTrack() ),
						m_toolBar );

	m_addSampleTrackButton = new toolButton( embed::getIconPixmap(
					"add_sample_track" ),
					tr( "Add sample-track" ),
					m_s, SLOT( addSampleTrack() ),
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
					this, SLOT( zoomingChanged( void ) ) );


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


	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setMinimum( 0 );
	m_leftRightScroll->setMaximum( 0 );
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( 20 );
	static_cast<QVBoxLayout *>( layout() )->addWidget( m_leftRightScroll );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ),
					this, SLOT( scrolled( int ) ) );


	if( engine::getMainWindow()->workspace() != NULL )
	{
		engine::getMainWindow()->workspace()->addSubWindow( this );
		parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
	}

	QWidget * w = ( parentWidget() != NULL ) ? parentWidget() : this;
	if( engine::getMainWindow()->workspace() != NULL )
	{
		w->resize( 680, 300 );
		w->move( 10, 10 );
	}
	else
	{
		resize( 580, 300 );
		w->move( 210, 10 );
	}

	w->show();

}




songEditor::~songEditor()
{
}




void songEditor::paintEvent( QPaintEvent * _pe )
{
	m_leftRightScroll->setMaximum( m_s->lengthInTacts() );
	trackContainerView::paintEvent( _pe );
}




void songEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( /*_ke->modifiers() & Qt::ShiftModifier*/
		engine::getMainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Insert )
	{
		m_s->insertBar();
	}
	else if(/* _ke->modifiers() & Qt::ShiftModifier &&*/
			engine::getMainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Delete )
	{
		m_s->removeBar();
	}
	else if( _ke->key() == Qt::Key_Left )
	{
		tact interesting_tact = m_s->currentTact();
		if( interesting_tact > 0 )
		{
			m_s->setPlayPos( --interesting_tact,
						m_s->currentTact64th(),
						song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Right )
	{
		tact interesting_tact = m_s->currentTact();
		if( interesting_tact < MAX_SONG_LENGTH )
		{
			m_s->setPlayPos( ++interesting_tact,
						m_s->currentTact64th(),
						song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Space )
	{
		if( m_s->playing() )
		{
			m_s->stop();
		}
		else
		{
			m_s->play();
		}
	}
	else if( _ke->key() == Qt::Key_Home )
	{
		m_s->setPlayPos( 0, 0, song::Mode_PlaySong );
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
	if( engine::getMainWindow()->isCtrlPressed() == TRUE )
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
	else if( engine::getMainWindow()->isShiftPressed() == TRUE )
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
	if( m_mvsStatus->isVisible() == FALSE && m_s->m_loadingProject == FALSE
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->reparent( m_masterVolumeSlider );
		m_mvsStatus->move( m_masterVolumeSlider->mapTo(
					m_masterVolumeSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	engine::getMixer()->setMasterGain( _new_val / 100.0f );
}




void songEditor::masterVolumePressed( void )
{
	m_mvsStatus->reparent( m_masterVolumeSlider );
	m_mvsStatus->move( m_masterVolumeSlider->mapTo(
					m_masterVolumeSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	masterVolumeMoved( m_s->m_masterVolumeModel.value() );
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
	if( m_mpsStatus->isVisible() == FALSE && m_s->m_loadingProject == FALSE
					&& m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->reparent( m_masterPitchSlider );
		m_mpsStatus->move( m_masterPitchSlider->mapTo(
					m_masterPitchSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
}




void songEditor::masterPitchPressed( void )
{
	m_mpsStatus->reparent( m_masterPitchSlider );
	m_mpsStatus->move( m_masterPitchSlider->mapTo(
					m_masterPitchSlider->topLevelWidget(),
							QPoint( 0, 0 ) ) +
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	masterPitchMoved( m_s->m_masterPitchModel.value() );
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
	if( ( m_s->m_playing && m_s->m_playMode == song::Mode_PlaySong ) ||
							m_scrollBack == TRUE )
	{
		const int w = width() - DEFAULT_SETTINGS_WIDGET_WIDTH
							- TRACK_OP_WIDTH;
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




void songEditor::zoomingChanged( void )
{
	const QString & zfac = m_zoomingComboBox->model()->currentText();
	setPixelsPerTact( zfac.left( zfac.length() - 1 ).toInt() *
					DEFAULT_PIXELS_PER_TACT / 100 );
	m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
	realignTracks();
}




void songEditor::updateTimeLinePosition( void )
{
	if( m_s->m_playPos[m_s->m_playMode].m_timeLine != NULL &&
		m_s->m_playPos[m_s->m_playMode].m_timeLineUpdate == TRUE )
	{
/*		QTimer::singleShot( 1, m_playPos[m_playMode].m_timeLine,
						SLOT( updatePosition() ) );*/
		//m_playPos[m_playMode].m_timeLine->updatePosition();
	}
}





bool songEditor::allowRubberband( void ) const
{
	return( m_editModeButton->isChecked() );
}




#include "song_editor.moc"


#endif
