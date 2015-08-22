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

#include "SongEditor.h"

#include <QTimeLine>
#include <QAction>
#include <QButtonGroup>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>
#include <QScrollBar>

#include <math.h>

#include "ActionGroup.h"
#include "AutomatableSlider.h"
#include "ComboBox.h"
#include "ConfigManager.h"
#include "CPULoadWidget.h"
#include "embed.h"
#include "GuiApplication.h"
#include "LcdSpinBox.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "ToolTip.h"
#include "VisualizationWidget.h"
#include "TimeDisplayWidget.h"
#include "AudioDevice.h"
#include "PianoRoll.h"



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




SongEditor::SongEditor( Song * _song ) :
	TrackContainerView( _song ),
	m_song( _song ),
	m_zoomingModel(new ComboBoxModel()),
	m_scrollBack( false ),
	m_smoothScroll( ConfigManager::inst()->value( "ui", "smoothscroll" ).toInt() ),
	m_mode(DrawMode)
{
	// create time-line
	int widgetTotal = ConfigManager::inst()->value( "ui",
							"compacttrackbuttons" ).toInt()==1 ?
		DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT :
		DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;
	m_timeLine = new TimeLineWidget( widgetTotal, 32,
					pixelsPerTact(),
					m_song->m_playPos[Song::Mode_PlaySong],
					m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
				m_song->m_playPos[Song::Mode_PlaySong].m_timeLine,
			SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( regionSelectedFromPixels( int, int ) ),
			this, SLOT( selectRegionFromPixels( int, int ) ) );
	connect( m_timeLine, SIGNAL( selectionFinished() ),
			 this, SLOT( stopRubberBand() ) );

	m_positionLine = new positionLine( this );

	static_cast<QVBoxLayout *>( layout() )->insertWidget( 1, m_timeLine );


	// add some essential widgets to global tool-bar
	QWidget * tb = gui->mainWindow()->toolBar();

	gui->mainWindow()->addSpacingToToolBar( 10 );

	m_tempoSpinBox = new LcdSpinBox( 3, tb, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &m_song->m_tempoModel );
	m_tempoSpinBox->setLabel( tr( "TEMPO/BPM" ) );
	ToolTip::add( m_tempoSpinBox, tr( "tempo of song" ) );

	m_tempoSpinBox->setWhatsThis(
		tr( "The tempo of a song is specified in beats per minute "
			"(BPM). If you want to change the tempo of your "
			"song, change this value. Every measure has four beats, "
			"so the tempo in BPM specifies, how many measures / 4 "
			"should be played within a minute (or how many measures "
			"should be played within four minutes)." ) );

	int tempoSpinBoxCol = gui->mainWindow()->addWidgetToToolBar( m_tempoSpinBox, 0 );

#if 0
	toolButton * hq_btn = new toolButton( embed::getIconPixmap( "hq_mode" ),
						tr( "High quality mode" ),
						NULL, NULL, tb );
	hq_btn->setCheckable( true );
	connect( hq_btn, SIGNAL( toggled( bool ) ),
			this, SLOT( setHighQuality( bool ) ) );
	hq_btn->setFixedWidth( 42 );
	gui->mainWindow()->addWidgetToToolBar( hq_btn, 1, col );
#endif

	gui->mainWindow()->addWidgetToToolBar( new TimeDisplayWidget, 1, tempoSpinBoxCol );

	gui->mainWindow()->addSpacingToToolBar( 10 );

	m_timeSigDisplay = new MeterDialog( this, true );
	m_timeSigDisplay->setModel( &m_song->m_timeSigModel );
	gui->mainWindow()->addWidgetToToolBar( m_timeSigDisplay );

	gui->mainWindow()->addSpacingToToolBar( 10 );


	QLabel * master_vol_lbl = new QLabel( tb );
	master_vol_lbl->setPixmap( embed::getIconPixmap( "master_volume" ) );

	m_masterVolumeSlider = new AutomatableSlider( tb,
							tr( "Master volume" ) );
	m_masterVolumeSlider->setModel( &m_song->m_masterVolumeModel );
	m_masterVolumeSlider->setOrientation( Qt::Vertical );
	m_masterVolumeSlider->setPageStep( 1 );
	m_masterVolumeSlider->setTickPosition( QSlider::TicksLeft );
	m_masterVolumeSlider->setFixedSize( 26, 60 );
	m_masterVolumeSlider->setTickInterval( 50 );
	ToolTip::add( m_masterVolumeSlider, tr( "master volume" ) );

	connect( m_masterVolumeSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( setMasterVolume( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderPressed() ), this,
			SLOT( showMasterVolumeFloat()) );
	connect( m_masterVolumeSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( updateMasterVolumeFloat( int ) ) );
	connect( m_masterVolumeSlider, SIGNAL( sliderReleased() ), this,
			SLOT( hideMasterVolumeFloat() ) );

	m_mvsStatus = new TextFloat;
	m_mvsStatus->setTitle( tr( "Master volume" ) );
	m_mvsStatus->setPixmap( embed::getIconPixmap( "master_volume" ) );

	gui->mainWindow()->addWidgetToToolBar( master_vol_lbl );
	gui->mainWindow()->addWidgetToToolBar( m_masterVolumeSlider );


	gui->mainWindow()->addSpacingToToolBar( 10 );


	QLabel * master_pitch_lbl = new QLabel( tb );
	master_pitch_lbl->setPixmap( embed::getIconPixmap( "master_pitch" ) );
	master_pitch_lbl->setFixedHeight( 64 );

	m_masterPitchSlider = new AutomatableSlider( tb, tr( "Master pitch" ) );
	m_masterPitchSlider->setModel( &m_song->m_masterPitchModel );
	m_masterPitchSlider->setOrientation( Qt::Vertical );
	m_masterPitchSlider->setPageStep( 1 );
	m_masterPitchSlider->setTickPosition( QSlider::TicksLeft );
	m_masterPitchSlider->setFixedSize( 26, 60 );
	m_masterPitchSlider->setTickInterval( 12 );
	ToolTip::add( m_masterPitchSlider, tr( "master pitch" ) );
	connect( m_masterPitchSlider, SIGNAL( logicValueChanged( int ) ), this,
			SLOT( setMasterPitch( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderPressed() ), this,
			SLOT( showMasterPitchFloat() ) );
	connect( m_masterPitchSlider, SIGNAL( logicSliderMoved( int ) ), this,
			SLOT( updateMasterPitchFloat( int ) ) );
	connect( m_masterPitchSlider, SIGNAL( sliderReleased() ), this,
			SLOT( hideMasterPitchFloat() ) );

	m_mpsStatus = new TextFloat;
	m_mpsStatus->setTitle( tr( "Master pitch" ) );
	m_mpsStatus->setPixmap( embed::getIconPixmap( "master_pitch" ) );

	gui->mainWindow()->addWidgetToToolBar( master_pitch_lbl );
	gui->mainWindow()->addWidgetToToolBar( m_masterPitchSlider );

	gui->mainWindow()->addSpacingToToolBar( 10 );

	// create widget for visualization- and cpu-load-widget
	QWidget * vc_w = new QWidget( tb );
	QVBoxLayout * vcw_layout = new QVBoxLayout( vc_w );
	vcw_layout->setMargin( 0 );
	vcw_layout->setSpacing( 0 );

	//vcw_layout->addStretch();
	vcw_layout->addWidget( new VisualizationWidget(
			embed::getIconPixmap( "output_graph" ), vc_w ) );

	vcw_layout->addWidget( new CPULoadWidget( vc_w ) );
	vcw_layout->addStretch();

	gui->mainWindow()->addWidgetToToolBar( vc_w );

	static_cast<QVBoxLayout *>( layout() )->insertWidget( 0, m_timeLine );

	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setMinimum( 0 );
	m_leftRightScroll->setMaximum( 0 );
	m_leftRightScroll->setSingleStep( 1 );
	m_leftRightScroll->setPageStep( 20 );
	static_cast<QVBoxLayout *>( layout() )->addWidget( m_leftRightScroll );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ),
					this, SLOT( scrolled( int ) ) );
	connect( m_song, SIGNAL( lengthChanged( int ) ),
			this, SLOT( updateScrollBar( int ) ) );

	// Set up zooming model
	for( int i = 0; i < 7; ++i )
	{
		m_zoomingModel->addItem(
					QString::number( 25 << i ) + "%" );
	}
	m_zoomingModel->setInitValue(
			m_zoomingModel->findText( "100%" ) );
	connect( m_zoomingModel, SIGNAL( dataChanged() ),
					this, SLOT( zoomingChanged() ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();
}




SongEditor::~SongEditor()
{
}

void SongEditor::saveSettings( QDomDocument& doc, QDomElement& element )
{
	MainWindow::saveWidgetState(parentWidget(), element);
}

void SongEditor::loadSettings( const QDomElement& element )
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void SongEditor::setHighQuality( bool _hq )
{
	Engine::mixer()->changeQuality( Mixer::qualitySettings(
			_hq ? Mixer::qualitySettings::Mode_HighQuality :
				Mixer::qualitySettings::Mode_Draft ) );
}




void SongEditor::scrolled( int _new_pos )
{
	update();
	emit positionChanged( m_currentPosition = MidiTime( _new_pos, 0 ) );
}




void SongEditor::setEditMode(EditMode mode)
{
	m_mode = mode;
}

void SongEditor::setEditModeDraw()
{
	setEditMode(DrawMode);
}

void SongEditor::setEditModeSelect()
{
	setEditMode(SelectMode);
}




void SongEditor::keyPressEvent( QKeyEvent * _ke )
{
	if( /*_ke->modifiers() & Qt::ShiftModifier*/
		gui->mainWindow()->isShiftPressed() == true &&
						_ke->key() == Qt::Key_Insert )
	{
		m_song->insertBar();
	}
	else if(/* _ke->modifiers() & Qt::ShiftModifier &&*/
			gui->mainWindow()->isShiftPressed() == true &&
						_ke->key() == Qt::Key_Delete )
	{
		m_song->removeBar();
	}
	else if( _ke->key() == Qt::Key_Left )
	{
		tick_t t = m_song->currentTick() - MidiTime::ticksPerTact();
		if( t >= 0 )
		{
			m_song->setPlayPos( t, Song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Right )
	{
		tick_t t = m_song->currentTick() + MidiTime::ticksPerTact();
		if( t < MaxSongLength )
		{
			m_song->setPlayPos( t, Song::Mode_PlaySong );
		}
	}
	else if( _ke->key() == Qt::Key_Home )
	{
		m_song->setPlayPos( 0, Song::Mode_PlaySong );
	}
	else
	{
		QWidget::keyPressEvent( _ke );
	}
}




void SongEditor::wheelEvent( QWheelEvent * _we )
{
	if( gui->mainWindow()->isCtrlPressed() == true )
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
		m_zoomingModel->setValue(
			m_zoomingModel->findText(
				QString::number(
					static_cast<int>( pixelsPerTact() *
					100 / DEFAULT_PIXELS_PER_TACT ) ) +
									"%" ) );
		// update timeline
		m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
		// and make sure, all TCO's are resized and relocated
		realignTracks();
	}
	else if( gui->mainWindow()->isShiftPressed() == true || _we->orientation() == Qt::Horizontal )
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



void SongEditor::closeEvent( QCloseEvent * _ce )
 {
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }




void SongEditor::setMasterVolume( int _new_val )
{
	updateMasterVolumeFloat( _new_val );

	if( !m_mvsStatus->isVisible() && !m_song->m_loadingProject
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	Engine::mixer()->setMasterGain( _new_val / 100.0f );
}




void SongEditor::showMasterVolumeFloat( void )
{
	m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	updateMasterVolumeFloat( m_song->m_masterVolumeModel.value() );
}




void SongEditor::updateMasterVolumeFloat( int _new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( _new_val ) );
}




void SongEditor::hideMasterVolumeFloat( void )
{
	m_mvsStatus->hide();
}




void SongEditor::setMasterPitch( int _new_val )
{
	updateMasterPitchFloat( _new_val );
	if( m_mpsStatus->isVisible() == false && m_song->m_loadingProject == false
					&& m_masterPitchSlider->showStatus() )
	{
		m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
		m_mpsStatus->setVisibilityTimeOut( 1000 );
	}
}




void SongEditor::showMasterPitchFloat( void )
{
	m_mpsStatus->moveGlobal( m_masterPitchSlider,
			QPoint( m_masterPitchSlider->width() + 2, -2 ) );
	m_mpsStatus->show();
	updateMasterPitchFloat( m_song->m_masterPitchModel.value() );
}




void SongEditor::updateMasterPitchFloat( int _new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones").arg( _new_val ) );

}




void SongEditor::hideMasterPitchFloat( void )
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
	if( ConfigManager::inst()->value( "ui", "compacttrackbuttons" ).toInt() )
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT;
		trackOpWidth = TRACK_OP_WIDTH_COMPACT;
	}
	else
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH;
		trackOpWidth = TRACK_OP_WIDTH;
	}

	if( ( m_song->isPlaying() && m_song->m_playMode == Song::Mode_PlaySong
		  && m_timeLine->autoScroll() == TimeLineWidget::AutoScrollEnabled) ||
							m_scrollBack == true )
	{
		m_smoothScroll = ConfigManager::inst()->value( "ui", "smoothscroll" ).toInt();
		const int w = width() - widgetWidth
							- trackOpWidth
							- contentWidget()->verticalScrollBar()->width(); // width of right scrollbar
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

	const int x = m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
							markerX( _t ) + 8;
	if( x >= trackOpWidth + widgetWidth -1 )
	{
		m_positionLine->show();
		m_positionLine->move( x, m_timeLine->height() );
	}
	else
	{
		m_positionLine->hide();
	}

	m_positionLine->setFixedHeight( height() );
}




void SongEditor::zoomingChanged()
{
	const QString & zfac = m_zoomingModel->currentText();
	setPixelsPerTact( zfac.left( zfac.length() - 1 ).toInt() *
					DEFAULT_PIXELS_PER_TACT / 100 );
	m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
					setPixelsPerTact( pixelsPerTact() );
	realignTracks();
}



bool SongEditor::allowRubberband() const
{
	return m_mode == SelectMode;
}


SongEditorWindow::SongEditorWindow(Song* song) :
	Editor(Engine::mixer()->audioDev()->supportsCapture()),
	m_editor(new SongEditor(song))
{
	setWindowTitle( tr( "Song-Editor" ) );
	setWindowIcon( embed::getIconPixmap( "songeditor" ) );

	setCentralWidget(m_editor);
	setAcceptDrops(true);
	m_toolBar->setAcceptDrops(true);
	connect(m_toolBar, SIGNAL(dragEntered(QDragEnterEvent*)), m_editor, SLOT(dragEnterEvent(QDragEnterEvent*)));
	connect(m_toolBar, SIGNAL(dropped(QDropEvent*)), m_editor, SLOT(dropEvent(QDropEvent*)));

	// Set up buttons
	m_playAction->setToolTip(tr("Play song (Space)"));
	m_recordAction->setToolTip(tr("Record samples from Audio-device"));
	m_recordAccompanyAction->setToolTip(tr( "Record samples from Audio-device while playing song or BB track"));
	m_stopAction->setToolTip(tr( "Stop song (Space)" ));

	m_addBBTrackAction = new QAction(embed::getIconPixmap("add_bb_track"),
									 tr("Add beat/bassline"), this);

	m_addSampleTrackAction = new QAction(embed::getIconPixmap("add_sample_track"),
										 tr("Add sample-track"), this);

	m_addAutomationTrackAction = new QAction(embed::getIconPixmap("add_automation"),
											 tr("Add automation-track"), this);

	connect(m_addBBTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addBBTrack()));
	connect(m_addSampleTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addSampleTrack()));
	connect(m_addAutomationTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addAutomationTrack()));

	ActionGroup* editModeGroup = new ActionGroup(this);
	m_drawModeAction = editModeGroup->addAction(embed::getIconPixmap("edit_draw"), tr("Draw mode"));
	m_selectModeAction = editModeGroup->addAction(embed::getIconPixmap("edit_select"), tr("Edit mode (select and move)"));

	m_drawModeAction->setChecked(true);

	connect(m_drawModeAction, SIGNAL(triggered()), m_editor, SLOT(setEditModeDraw()));
	connect(m_selectModeAction, SIGNAL(triggered()), m_editor, SLOT(setEditModeSelect()));


	m_playAction->setWhatsThis(
				tr("Click here, if you want to play your whole song. "
				   "Playing will be started at the song-position-marker (green). "
				   "You can also move it while playing."));
	m_stopAction->setWhatsThis(
				tr("Click here, if you want to stop playing of your song. "
				   "The song-position-marker will be set to the start of your song."));


	QLabel * zoom_lbl = new QLabel( m_toolBar );
	zoom_lbl->setPixmap( embed::getIconPixmap( "zoom" ) );

	// setup zooming-stuff
	m_zoomingComboBox = new ComboBox( m_toolBar );
	m_zoomingComboBox->setFixedSize( 80, 22 );
	m_zoomingComboBox->move( 580, 4 );
	m_zoomingComboBox->setModel(m_editor->m_zoomingModel);


	m_toolBar->addSeparator();
	m_toolBar->addAction( m_addBBTrackAction );
	m_toolBar->addAction( m_addSampleTrackAction );
	m_toolBar->addAction( m_addAutomationTrackAction );
	m_toolBar->addSeparator();
	m_toolBar->addAction( m_drawModeAction );
	m_toolBar->addAction( m_selectModeAction );
	m_toolBar->addSeparator();
	m_editor->m_timeLine->addToolButtons(m_toolBar);
	m_toolBar->addSeparator();
	m_toolBar->addWidget( zoom_lbl );
	m_toolBar->addWidget( m_zoomingComboBox );

	connect(song, SIGNAL(projectLoaded()), this, SLOT(adjustUiAfterProjectLoad()));
}

QSize SongEditorWindow::sizeHint() const
{
	return {600, 300};
}


void SongEditorWindow::play()
{
	if( Engine::getSong()->playMode() != Song::Mode_PlaySong )
	{
		Engine::getSong()->playSong();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}


void SongEditorWindow::record()
{
	m_editor->m_song->record();
}


void SongEditorWindow::recordAccompany()
{
	m_editor->m_song->playAndRecord();
}


void SongEditorWindow::stop()
{
	m_editor->m_song->stop();
	gui->pianoRoll()->stopRecording();
}

void SongEditorWindow::adjustUiAfterProjectLoad()
{
	// make sure to bring us to front as the song editor is the central
	// widget in a song and when just opening a song in order to listen to
	// it, it's very annyoing to manually bring up the song editor each time
	gui->mainWindow()->workspace()->setActiveSubWindow(
			qobject_cast<QMdiSubWindow *>( parentWidget() ) );
	m_editor->scrolled(0);
}
