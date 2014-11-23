/*
 * SongEditor.cpp - basic window for song-editing
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QTimeLine>
#include <QtGui/QAction>
#include <QtGui/QButtonGroup>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>

#include <math.h>

#include "SongEditor.h"
#include "automatable_slider.h"
#include "combobox.h"
#include "config_mgr.h"
#include "cpuload_widget.h"
#include "embed.h"
#include "LcdSpinBox.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "text_float.h"
#include "timeline.h"
#include "tool_button.h"
#include "tooltip.h"
#include "visualization_widget.h"
#include "TimeDisplayWidget.h"
#include "AudioDevice.h"
#include "PianoRoll.h"
#include "config_mgr.h"



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




SongEditor::SongEditor( song * _song ) :
	TrackContainerView( _song ),
	m_s( _song ),
	m_scrollBack( false ),
	m_smoothScroll( configManager::inst()->value( "ui", "smoothscroll" ).toInt() )
{
	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	// create time-line
	int widgetTotal = configManager::inst()->value( "ui",
							"compacttrackbuttons" ).toInt()==1 ?
		DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT :
		DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;
	m_timeLine = new timeLine( widgetTotal, 32,
					pixelsPerTact(),
					m_s->m_playPos[song::Mode_PlaySong],
					m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
				m_s->m_playPos[song::Mode_PlaySong].m_timeLine,
			SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( updatePosition( const MidiTime & ) ) );

	m_positionLine = new positionLine( this );


	// let's get notified when loading a project
	connect( m_s, SIGNAL( projectLoaded() ),
				this, SLOT( adjustUiAfterProjectLoad() ) );


	// add some essential widgets to global tool-bar
	QWidget * tb = engine::mainWindow()->toolBar();

	engine::mainWindow()->addSpacingToToolBar( 10 );

	m_tempoSpinBox = new LcdSpinBox( 3, tb, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &m_s->m_tempoModel );
	m_tempoSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	toolTip::add( m_tempoSpinBox, tr( "tempo of song" ) );

	m_tempoSpinBox->setWhatsThis(
		tr( "The tempo of a song is specified in beats per minute "
			"(BPM). If you want to change the tempo of your "
			"song, change this value. Every measure has four beats, "
			"so the tempo in BPM specifies, how many measures / 4 "
			"should be played within a minute (or how many measures "
			"should be played within four minutes)." ) );

	int tempoSpinBoxCol = engine::mainWindow()->addWidgetToToolBar( m_tempoSpinBox, 0 );

#if 0
	toolButton * hq_btn = new toolButton( embed::getIconPixmap( "hq_mode" ),
						tr( "High quality mode" ),
						NULL, NULL, tb );
	hq_btn->setCheckable( TRUE );
	connect( hq_btn, SIGNAL( toggled( bool ) ),
			this, SLOT( setHighQuality( bool ) ) );
	hq_btn->setFixedWidth( 42 );
	engine::mainWindow()->addWidgetToToolBar( hq_btn, 1, col );
#endif

	engine::mainWindow()->addWidgetToToolBar( new TimeDisplayWidget, 1, tempoSpinBoxCol );

	engine::mainWindow()->addSpacingToToolBar( 10 );

	m_timeSigDisplay = new MeterDialog( this, TRUE );
	m_timeSigDisplay->setModel( &m_s->m_timeSigModel );
	engine::mainWindow()->addWidgetToToolBar( m_timeSigDisplay );

	engine::mainWindow()->addSpacingToToolBar( 10 );


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

	m_mvsStatus = new textFloat;
	m_mvsStatus->setTitle( tr( "Master volume" ) );
	m_mvsStatus->setPixmap( embed::getIconPixmap( "master_volume" ) );

	engine::mainWindow()->addWidgetToToolBar( master_vol_lbl );
	engine::mainWindow()->addWidgetToToolBar( m_masterVolumeSlider );


	engine::mainWindow()->addSpacingToToolBar( 10 );


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

	m_mpsStatus = new textFloat;
	m_mpsStatus->setTitle( tr( "Master pitch" ) );
	m_mpsStatus->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	engine::mainWindow()->addWidgetToToolBar( master_pitch_lbl );
	engine::mainWindow()->addWidgetToToolBar( m_masterPitchSlider );

	engine::mainWindow()->addSpacingToToolBar( 10 );

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

	engine::mainWindow()->addWidgetToToolBar( vc_w );


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
	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
					tr( "Play song (Space)" ),
					this, SLOT( play() ), m_toolBar );
	m_playButton->setObjectName( "playButton" );

	m_recordButton = new toolButton( embed::getIconPixmap( "record" ),
			tr( "Record samples from Audio-device" ),
					this, SLOT( record() ), m_toolBar );
	m_recordButton->setObjectName( "recordButton" );

	m_recordAccompanyButton = new toolButton(
			embed::getIconPixmap( "record_accompany" ),
			tr( "Record samples from Audio-device while playing "
							"song or BB track" ),
				this, SLOT( recordAccompany() ), m_toolBar );
	m_recordAccompanyButton->setObjectName( "recordAccompanyButton" );

	// FIXME: disable record button while it is not implemented
	m_recordButton->setDisabled( true );

	// disable record buttons if capturing is not supported
	if( !engine::mixer()->audioDev()->supportsCapture() )
	{
		m_recordButton->setDisabled( true );
		m_recordAccompanyButton->setDisabled( true );
	}

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
					tr( "Stop song (Space)" ),
					this, SLOT( stop() ), m_toolBar );
	m_stopButton->setObjectName( "stopButton" );

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

	m_addAutomationTrackButton = new toolButton( embed::getIconPixmap(
					"add_automation" ),
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




SongEditor::~SongEditor()
{
}




void SongEditor::setHighQuality( bool _hq )
{
	engine::mixer()->changeQuality( Mixer::qualitySettings(
			_hq ? Mixer::qualitySettings::Mode_HighQuality :
				Mixer::qualitySettings::Mode_Draft ) );
}




void SongEditor::scrolled( int _new_pos )
{
	update();
	emit positionChanged( m_currentPosition = MidiTime( _new_pos, 0 ) );
}




void SongEditor::setPauseIcon( bool pause )
{
	if( pause == true )
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	}
}




void SongEditor::play()
{
	if( engine::getSong()->playMode() != song::Mode_PlaySong )
	{
		engine::getSong()->playSong();
	}
	else
	{
		engine::getSong()->togglePause();
	}
}




void SongEditor::record()
{
	m_s->record();
}




void SongEditor::recordAccompany()
{
	m_s->playAndRecord();
}




void SongEditor::stop()
{
	m_s->stop();
	engine::pianoRoll()->stopRecording();
}




void SongEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( /*_ke->modifiers() & Qt::ShiftModifier*/
		engine::mainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Insert )
	{
		m_s->insertBar();
	}
	else if(/* _ke->modifiers() & Qt::ShiftModifier &&*/
			engine::mainWindow()->isShiftPressed() == TRUE &&
						_ke->key() == Qt::Key_Delete )
	{
		m_s->removeBar();
	}
	else if( _ke->key() == Qt::Key_Left )
	{
		tick_t t = m_s->currentTick() - MidiTime::ticksPerTact();
		if( t >= 0 )
		{
			m_s->setPlayPos( t, song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Right )
	{
		tick_t t = m_s->currentTick() + MidiTime::ticksPerTact();
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




void SongEditor::wheelEvent( QWheelEvent * _we )
{
	if( engine::mainWindow()->isCtrlPressed() == TRUE )
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
	else if( engine::mainWindow()->isShiftPressed() == TRUE )
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




void SongEditor::masterVolumeChanged( int _new_val )
{
	masterVolumeMoved( _new_val );
	if( m_mvsStatus->isVisible() == FALSE && m_s->m_loadingProject == FALSE
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	engine::mixer()->setMasterGain( _new_val / 100.0f );
}




void SongEditor::masterVolumePressed( void )
{
	m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	masterVolumeMoved( m_s->m_masterVolumeModel.value() );
}




void SongEditor::masterVolumeMoved( int _new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( _new_val ) );
}




void SongEditor::masterVolumeReleased( void )
{
	m_mvsStatus->hide();
}




void SongEditor::masterPitchChanged( int _new_val )
{
	masterPitchMoved( _new_val );
	if( m_mpsStatus->isVisible() == FALSE && m_s->m_loadingProject == FALSE
					&& m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
}




void SongEditor::masterPitchPressed( void )
{
	m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	masterPitchMoved( m_s->m_masterPitchModel.value() );
}




void SongEditor::masterPitchMoved( int _new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones").arg( _new_val ) );

}




void SongEditor::masterPitchReleased( void )
{
	m_mpsStatus->hide();
}




void SongEditor::updateScrollBar( int _len )
{
	m_leftRightScroll->setMaximum( _len );
}




static inline void animateScroll( QScrollBar *scrollBar, int newVal, bool smoothScroll )
{
	if( smoothScroll == false )
	{
		scrollBar->setValue( newVal );
	}
	else
	{
		// do smooth scroll animation using QTimeLine
		QTimeLine *t = scrollBar->findChild<QTimeLine *>();
		if( t == NULL )
		{
			t = new QTimeLine( 600, scrollBar );
			t->setFrameRange( scrollBar->value(), newVal );
			t->connect( t, SIGNAL( finished() ), SLOT( deleteLater() ) );

			scrollBar->connect( t, SIGNAL( frameChanged( int ) ), SLOT( setValue( int ) ) );

			t->start();
		}
		else
		{
			// smooth scrolling is still active, therefore just update the end frame
			t->setEndFrame( newVal );
		}
	}
}




void SongEditor::updatePosition( const MidiTime & _t )
{
	int widgetWidth, trackOpWidth;
	if( configManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() )
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT;
		trackOpWidth = TRACK_OP_WIDTH_COMPACT;
	}
	else
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH;
		trackOpWidth = TRACK_OP_WIDTH;
	}

	if( ( m_s->isPlaying() && m_s->m_playMode == song::Mode_PlaySong
		  && m_timeLine->autoScroll() == timeLine::AutoScrollEnabled) ||
							m_scrollBack == true )
	{
		const int w = width() - widgetWidth
							- trackOpWidth
							- 32;	// rough estimation for width of right scrollbar
		if( _t > m_currentPosition + w * MidiTime::ticksPerTact() /
							pixelsPerTact() )
		{
			animateScroll( m_leftRightScroll, _t.getTact(), m_smoothScroll );
		}
		else if( _t < m_currentPosition )
		{
			MidiTime t = qMax(
				(int)( _t - w * MidiTime::ticksPerTact() /
							pixelsPerTact() ),
									0 );
			animateScroll( m_leftRightScroll, t.getTact(), m_smoothScroll );
		}
		m_scrollBack = false;
	}

	const int x = m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
							markerX( _t ) + 8;
	if( x >= trackOpWidth + widgetWidth -1 )
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




void SongEditor::zoomingChanged()
{
	const QString & zfac = m_zoomingComboBox->model()->currentText();
	setPixelsPerTact( zfac.left( zfac.length() - 1 ).toInt() *
					DEFAULT_PIXELS_PER_TACT / 100 );
	m_s->m_playPos[song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
	realignTracks();
}




void SongEditor::adjustUiAfterProjectLoad()
{
	//if( isMaximized() )
	{
		// make sure to bring us to front as the song editor is the central
		// widget in a song and when just opening a song in order to listen to
		// it, it's very annyoing to manually bring up the song editor each time
		engine::mainWindow()->workspace()->setActiveSubWindow(
				qobject_cast<QMdiSubWindow *>( parentWidget() ) );
	}
	scrolled( 0 );
}




bool SongEditor::allowRubberband() const
{
	return( m_editModeButton->isChecked() );
}




#include "moc_SongEditor.cxx"


/* vim: set tw=0 noexpandtab: */
