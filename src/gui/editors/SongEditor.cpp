/*
 * SongEditor.cpp - basic window for song-editing
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>

#include "AutomatableSlider.h"
#include "ComboBox.h"
#include "ConfigManager.h"
#include "CPULoadWidget.h"
#include "embed.h"
#include "GuiApplication.h"
#include "LcdSpinBox.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "Mixer.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "ToolTip.h"
#include "VisualizationWidget.h"
#include "TimeDisplayWidget.h"
#include "AudioDevice.h"
#include "PianoRoll.h"
#include "Track.h"

positionLine::positionLine( QWidget * parent ) :
	QWidget( parent )
{
	setFixedWidth( 1 );
	setAttribute( Qt::WA_NoSystemBackground, true );
}




void positionLine::paintEvent( QPaintEvent * pe )
{
	QPainter p( this );
	p.fillRect( rect(), QColor( 255, 255, 255, 153 ) );
}

const QVector<double> SongEditor::m_zoomLevels =
		{ 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };


SongEditor::SongEditor( Song * song ) :
	TrackContainerView( song ),
	m_song( song ),
	m_zoomingModel(new ComboBoxModel()),
	m_snappingModel(new ComboBoxModel()),
	m_proportionalSnap( false ),
	m_scrollBack( false ),
	m_smoothScroll( ConfigManager::inst()->value( "ui", "smoothscroll" ).toInt() ),
	m_mode(DrawMode),
	m_origin(),
	m_scrollPos(),
	m_mousePos(),
	m_rubberBandStartTrackview(0),
	m_rubberbandStartMidipos(0),
	m_currentZoomingValue(m_zoomingModel->value()),
	m_trackHeadWidth(ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt()==1
					 ? DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT
					 : DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH),
	m_selectRegion(false)
{
	m_zoomingModel->setParent(this);
	m_snappingModel->setParent(this);
	m_timeLine = new TimeLineWidget( m_trackHeadWidth, 32,
					pixelsPerBar(),
					m_song->m_playPos[Song::Mode_PlaySong],
					m_currentPosition,
					Song::Mode_PlaySong, this );
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

	gui->mainWindow()->addSpacingToToolBar( 40 );

	m_tempoSpinBox = new LcdSpinBox( 3, tb, tr( "Tempo" ) );
	m_tempoSpinBox->setModel( &m_song->m_tempoModel );
	m_tempoSpinBox->setLabel( tr( "TEMPO" ) );
	ToolTip::add( m_tempoSpinBox, tr( "Tempo in BPM" ) );

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
	ToolTip::add( m_masterVolumeSlider, tr( "Master volume" ) );

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
	ToolTip::add( m_masterPitchSlider, tr( "Master pitch" ) );
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
	connect(m_leftRightScroll, SIGNAL(valueChanged(int)),this, SLOT(updateRubberband()));
	connect(contentWidget()->verticalScrollBar(), SIGNAL(valueChanged(int)),this, SLOT(updateRubberband()));
	connect(m_timeLine, SIGNAL(selectionFinished()), this, SLOT(stopSelectRegion()));


	//Set up zooming model
	for( float const & zoomLevel : m_zoomLevels )
	{
		m_zoomingModel->addItem( QString( "%1\%" ).arg( zoomLevel * 100 ) );
	}
	m_zoomingModel->setInitValue(
			m_zoomingModel->findText( "100%" ) );
	connect( m_zoomingModel, SIGNAL( dataChanged() ),
					this, SLOT( zoomingChanged() ) );

	//Set up snapping model, 2^i
	for ( int i = 3; i >= -4; i-- )
	{
		if ( i > 0 )
		{
			m_snappingModel->addItem( QString( "%1 Bars").arg( 1 << i ) );
		}
		else if ( i == 0 )
		{
			m_snappingModel->addItem( "1 Bar" );
		}
		else
		{
			m_snappingModel->addItem( QString( "1/%1 Bar" ).arg( 1 << (-i) ) );
		}
	}
	m_snappingModel->setInitValue( m_snappingModel->findText( "1 Bar" ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();
}




SongEditor::~SongEditor()
{
}

void SongEditor::saveSettings( QDomDocument& doc, QDomElement& element )
{
	MainWindow::saveWidgetState( parentWidget(), element );
}

void SongEditor::loadSettings( const QDomElement& element )
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




float SongEditor::getSnapSize() const
{
	// 1 Bar is the third value in the snapping dropdown
	int val = -m_snappingModel->value() + 3;
	// If proportional snap is on, we snap to finer values when zoomed in
	if (m_proportionalSnap)
	{
		val = val - m_zoomingModel->value() + 3;
	}
	val = max(val, -6); // -6 gives 1/64th bar snapping. Lower values cause crashing.

	if ( val >= 0 ){
		return 1 << val;
	}
	else {
		return 1.0 / ( 1 << -val );
	}
}

QString SongEditor::getSnapSizeString() const
{
	int val = -m_snappingModel->value() + 3;
	val = val - m_zoomingModel->value() + 3;
	val = max(val, -6); // -6 gives 1/64th bar snapping. Lower values cause crashing.

	if ( val >= 0 ){
		int bars = 1 << val;
		if ( bars == 1 ) { return QString("1 Bar"); }
		else
		{
			return QString( "%1 Bars" ).arg(bars);
		}
	}
	else {
		int div = ( 1 << -val );
		return QString( "1/%1 Bar" ).arg(div);
	}
}




void SongEditor::setHighQuality( bool hq )
{
	Engine::mixer()->changeQuality( Mixer::qualitySettings(
			hq ? Mixer::qualitySettings::Mode_HighQuality :
				Mixer::qualitySettings::Mode_Draft ) );
}




void SongEditor::scrolled( int new_pos )
{
	update();
	emit positionChanged( m_currentPosition = MidiTime( new_pos, 0 ) );
}




void SongEditor::selectRegionFromPixels(int xStart, int xEnd)
{
	if (!m_selectRegion)
	{
		m_selectRegion = true;

		//deselect all tcos
		for (auto &it : findChildren<selectableObject *>()) { it->setSelected(false); }

		rubberBand()->setEnabled(true);
		rubberBand()->show();

		//we save the position of scrollbars, mouse position and zooming level
		m_origin = QPoint(xStart, 0);
		m_scrollPos = QPoint(m_leftRightScroll->value(), contentWidget()->verticalScrollBar()->value());
		m_currentZoomingValue = zoomingModel()->value();

		//calculate the song position where the mouse was clicked
		m_rubberbandStartMidipos = MidiTime((xStart - m_trackHeadWidth)
											/ pixelsPerBar() * MidiTime::ticksPerBar())
											+ m_currentPosition;
		m_rubberBandStartTrackview = 0;
	}
	//the current mouse position within the borders of song editor
	m_mousePos = QPoint(qMax(m_trackHeadWidth, qMin(xEnd, width()))
						, std::numeric_limits<int>::max());
	updateRubberband();
}




void SongEditor::stopSelectRegion()
{
	m_selectRegion = false;
}




void SongEditor::updateRubberband()
{
	if (rubberBandActive())
	{
		int originX = m_origin.x();

		//take care of the zooming
		if (m_currentZoomingValue != m_zoomingModel->value())
		{
			originX = m_trackHeadWidth + (originX - m_trackHeadWidth)
					* m_zoomLevels[m_zoomingModel->value()] / m_zoomLevels[m_currentZoomingValue];
		}

		//take care of the scrollbar position
		int hs = (m_leftRightScroll->value() - m_scrollPos.x()) * pixelsPerBar();
		int vs = contentWidget()->verticalScrollBar()->value() - m_scrollPos.y();

		//the adjusted origin point
		QPoint origin = QPoint(qMax(originX - hs, m_trackHeadWidth), m_origin.y() - vs);

		//paint the rubber band rect
		rubberBand()->setGeometry(QRect(origin,
										contentWidget()->mapFromParent(QPoint(m_mousePos.x(), m_mousePos.y()))
										).normalized());

		//the index of the TrackView the mouse is hover
		int rubberBandTrackview = trackIndexFromSelectionPoint(m_mousePos.y());

		//the miditime the mouse is hover
		MidiTime rubberbandMidipos = MidiTime((qMin(m_mousePos.x(), width()) - m_trackHeadWidth)
											  / pixelsPerBar() * MidiTime::ticksPerBar())
											  + m_currentPosition;

		//are tcos in the rect of selection?
		for (auto &it : findChildren<selectableObject *>())
		{
			TrackContentObjectView * tco = dynamic_cast<TrackContentObjectView*>(it);
			if (tco)
			{
				auto indexOfTrackView = trackViews().indexOf(tco->getTrackView());
				bool isBeetweenRubberbandViews = indexOfTrackView >= qMin(m_rubberBandStartTrackview, rubberBandTrackview)
											  && indexOfTrackView <= qMax(m_rubberBandStartTrackview, rubberBandTrackview);
				bool isBeetweenRubberbandMidiPos = tco->getTrackContentObject()->endPosition() >= qMin(m_rubberbandStartMidipos, rubberbandMidipos)
											  && tco->getTrackContentObject()->startPosition() <= qMax(m_rubberbandStartMidipos, rubberbandMidipos);
				it->setSelected(isBeetweenRubberbandViews && isBeetweenRubberbandMidiPos);
			}
		}
	}
}




void SongEditor::setEditMode( EditMode mode )
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

void SongEditor::toggleProportionalSnap()
{
	m_proportionalSnap = !m_proportionalSnap;
}




void SongEditor::keyPressEvent( QKeyEvent * ke )
{
	if( ke->modifiers() & Qt::ShiftModifier &&
						( ke->key() == Qt::Key_Insert || ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return ) )
	{
		m_song->insertBar();
	}
	else if( ke->modifiers() & Qt::ShiftModifier &&
						( ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace ) )
	{
		m_song->removeBar();
	}
	else if( ke->key() == Qt::Key_Left )
	{
		tick_t t = m_song->currentTick() - MidiTime::ticksPerBar();
		if( t >= 0 )
		{
			m_song->setPlayPos( t, Song::Mode_PlaySong );
		}
	}
	else if( ke->key() == Qt::Key_Right )
	{
		tick_t t = m_song->currentTick() + MidiTime::ticksPerBar();
		if( t < MaxSongLength )
		{
			m_song->setPlayPos( t, Song::Mode_PlaySong );
		}
	}
	else if( ke->key() == Qt::Key_Home )
	{
		m_song->setPlayPos( 0, Song::Mode_PlaySong );
	}
	else if( ke->key() == Qt::Key_Delete )
	{
		QVector<TrackContentObjectView *> tcoViews;
		QVector<selectableObject *> so = selectedObjects();
		for( QVector<selectableObject *>::iterator it = so.begin();
				it != so.end(); ++it )
		{
			TrackContentObjectView * tcov =
				dynamic_cast<TrackContentObjectView *>( *it );
			tcov->remove();
		}
	}
	else if( ke->key() == Qt::Key_A && ke->modifiers() & Qt::ControlModifier )
	{
		selectAllTcos( !(ke->modifiers() & Qt::ShiftModifier) );
	}
	else if( ke->key() == Qt::Key_Escape )
	{
		selectAllTcos( false );
	}
	else
	{
		QWidget::keyPressEvent( ke );
	}
}




void SongEditor::wheelEvent( QWheelEvent * we )
{
	if( we->modifiers() & Qt::ControlModifier )
	{
		int z = m_zoomingModel->value();

		if( we->delta() > 0 )
		{
			z++;
		}
		else if( we->delta() < 0 )
		{
			z--;
		}
		z = qBound( 0, z, m_zoomingModel->size() - 1 );


		int x = we->x() - m_trackHeadWidth;
		// bar based on the mouse x-position where the scroll wheel was used
		int bar = x / pixelsPerBar();
		// what would be the bar in the new zoom level on the very same mouse x
		int newBar = x / DEFAULT_PIXELS_PER_BAR / m_zoomLevels[z];
		// scroll so the bar "selected" by the mouse x doesn't move on the screen
		m_leftRightScroll->setValue(m_leftRightScroll->value() + bar - newBar);

		// update combobox with zooming-factor
		m_zoomingModel->setValue( z );

		// update timeline
		m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
					setPixelsPerBar( pixelsPerBar() );
		// and make sure, all TCO's are resized and relocated
		realignTracks();
	}
	else if( we->modifiers() & Qt::ShiftModifier || we->orientation() == Qt::Horizontal )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							we->delta() / 30 );
	}
	else
	{
		we->ignore();
		return;
	}
	we->accept();
}



void SongEditor::closeEvent( QCloseEvent * ce )
{
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	ce->ignore();
}




void SongEditor::mousePressEvent(QMouseEvent *me)
{
	if (allowRubberband())
	{
		//we save the position of scrollbars, mouse position and zooming level
		m_scrollPos = QPoint(m_leftRightScroll->value(), contentWidget()->verticalScrollBar()->value());
		m_origin = contentWidget()->mapFromParent(QPoint(me->pos().x(), me->pos().y()));
		m_currentZoomingValue = zoomingModel()->value();

		//paint the rubberband
		rubberBand()->setEnabled(true);
		rubberBand()->setGeometry(QRect(m_origin, QSize()));
		rubberBand()->show();

		//the trackView(index) and the miditime where the mouse was clicked
		m_rubberBandStartTrackview = trackIndexFromSelectionPoint(me->y());
		m_rubberbandStartMidipos = MidiTime((me->x() - m_trackHeadWidth)
											/ pixelsPerBar() * MidiTime::ticksPerBar())
											+ m_currentPosition;
	}
	QWidget::mousePressEvent(me);
}




void SongEditor::mouseMoveEvent(QMouseEvent *me)
{
	m_mousePos = me->pos();
	updateRubberband();
	QWidget::mouseMoveEvent(me);
}




void SongEditor::mouseReleaseEvent(QMouseEvent *me)
{
	rubberBand()->hide();
	rubberBand()->setEnabled(false);
	QWidget::mouseReleaseEvent(me);
}




void SongEditor::setMasterVolume( int new_val )
{
	updateMasterVolumeFloat( new_val );

	if( !m_mvsStatus->isVisible() && !m_song->m_loadingProject
					&& m_masterVolumeSlider->showStatus() )
	{
		m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
		m_mvsStatus->setVisibilityTimeOut( 1000 );
	}
	Engine::mixer()->setMasterGain( new_val / 100.0f );
}




void SongEditor::showMasterVolumeFloat( void )
{
	m_mvsStatus->moveGlobal( m_masterVolumeSlider,
			QPoint( m_masterVolumeSlider->width() + 2, -2 ) );
	m_mvsStatus->show();
	updateMasterVolumeFloat( m_song->m_masterVolumeModel.value() );
}




void SongEditor::updateMasterVolumeFloat( int new_val )
{
	m_mvsStatus->setText( tr( "Value: %1%" ).arg( new_val ) );
}




void SongEditor::hideMasterVolumeFloat( void )
{
	m_mvsStatus->hide();
}




void SongEditor::setMasterPitch( int new_val )
{
	updateMasterPitchFloat( new_val );
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




void SongEditor::updateMasterPitchFloat( int new_val )
{
	m_mpsStatus->setText( tr( "Value: %1 semitones").arg( new_val ) );

}




void SongEditor::hideMasterPitchFloat( void )
{
	m_mpsStatus->hide();
}




void SongEditor::updateScrollBar( int len )
{
	m_leftRightScroll->setMaximum( len );
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




void SongEditor::updatePosition( const MidiTime & t )
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
		if( t > m_currentPosition + w * MidiTime::ticksPerBar() /
							pixelsPerBar() )
		{
			animateScroll( m_leftRightScroll, t.getBar(), m_smoothScroll );
		}
		else if( t < m_currentPosition )
		{
			animateScroll( m_leftRightScroll, t.getBar(), m_smoothScroll );
		}
		m_scrollBack = false;
	}

	const int x = m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
							markerX( t ) + 8;
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




void SongEditor::updatePositionLine()
{
	m_positionLine->setFixedHeight( height() );
}




void SongEditor::zoomingChanged()
{
	setPixelsPerBar( m_zoomLevels[m_zoomingModel->value()] * DEFAULT_PIXELS_PER_BAR );

	m_song->m_playPos[Song::Mode_PlaySong].m_timeLine->
					setPixelsPerBar( pixelsPerBar() );
	realignTracks();
	updateRubberband();
}




void SongEditor::selectAllTcos( bool select )
{
	QVector<selectableObject *> so = select ? rubberBand()->selectableObjects() : rubberBand()->selectedObjects();
	for( int i = 0; i < so.count(); ++i )
	{
		so.at(i)->setSelected( select );
	}
}




bool SongEditor::allowRubberband() const
{
	return m_mode == SelectMode;
}




int SongEditor::trackIndexFromSelectionPoint(int yPos)
{
	const TrackView * tv = trackViewAt(yPos - m_timeLine->height());
	return tv ? indexOfTrackView(tv)
			  : yPos < m_timeLine->height() ? 0
											: trackViews().count();
}




int SongEditor::indexOfTrackView(const TrackView *tv)
{
	return static_cast<int>(std::distance(trackViews().begin(),
										  std::find(trackViews().begin(), trackViews().end(), tv)));
}




ComboBoxModel *SongEditor::zoomingModel() const
{
	return m_zoomingModel;
}




ComboBoxModel *SongEditor::snappingModel() const
{
	return m_snappingModel;
}




SongEditorWindow::SongEditorWindow(Song* song) :
	Editor(Engine::mixer()->audioDev()->supportsCapture(), false),
	m_editor(new SongEditor(song)),
	m_crtlAction( NULL ),
	m_snapSizeLabel( new QLabel( m_toolBar ) )
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


	// Track actions
	DropToolBar *trackActionsToolBar = addDropToolBarToTop(tr("Track actions"));

	m_addBBTrackAction = new QAction(embed::getIconPixmap("add_bb_track"),
									 tr("Add beat/bassline"), this);

	m_addSampleTrackAction = new QAction(embed::getIconPixmap("add_sample_track"),
										 tr("Add sample-track"), this);

	m_addAutomationTrackAction = new QAction(embed::getIconPixmap("add_automation"),
											 tr("Add automation-track"), this);

	connect(m_addBBTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addBBTrack()));
	connect(m_addSampleTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addSampleTrack()));
	connect(m_addAutomationTrackAction, SIGNAL(triggered()), m_editor->m_song, SLOT(addAutomationTrack()));

	trackActionsToolBar->addAction( m_addBBTrackAction );
	trackActionsToolBar->addAction( m_addSampleTrackAction );
	trackActionsToolBar->addAction( m_addAutomationTrackAction );


	// Edit actions
	DropToolBar *editActionsToolBar = addDropToolBarToTop(tr("Edit actions"));

	m_editModeGroup = new ActionGroup(this);
	m_drawModeAction = m_editModeGroup->addAction(embed::getIconPixmap("edit_draw"), tr("Draw mode"));
	m_selectModeAction = m_editModeGroup->addAction(embed::getIconPixmap("edit_select"), tr("Edit mode (select and move)"));
	m_drawModeAction->setChecked(true);

	connect(m_drawModeAction, SIGNAL(triggered()), m_editor, SLOT(setEditModeDraw()));
	connect(m_selectModeAction, SIGNAL(triggered()), m_editor, SLOT(setEditModeSelect()));

	editActionsToolBar->addAction( m_drawModeAction );
	editActionsToolBar->addAction( m_selectModeAction );

	DropToolBar *timeLineToolBar = addDropToolBarToTop(tr("Timeline controls"));
	m_editor->m_timeLine->addToolButtons(timeLineToolBar);


	DropToolBar *zoomToolBar = addDropToolBarToTop(tr("Zoom controls"));

	QLabel * zoom_lbl = new QLabel( m_toolBar );
	zoom_lbl->setPixmap( embed::getIconPixmap( "zoom" ) );

	//Set up zooming-stuff
	m_zoomingComboBox = new ComboBox( m_toolBar );
	m_zoomingComboBox->setFixedSize( 80, 22 );
	m_zoomingComboBox->move( 580, 4 );
	m_zoomingComboBox->setModel(m_editor->m_zoomingModel);
	m_zoomingComboBox->setToolTip(tr("Horizontal zooming"));
	connect(m_editor->zoomingModel(), SIGNAL(dataChanged()), this, SLOT(updateSnapLabel()));

	zoomToolBar->addWidget( zoom_lbl );
	zoomToolBar->addWidget( m_zoomingComboBox );

	DropToolBar *snapToolBar = addDropToolBarToTop(tr("Snap controls"));
	QLabel * snap_lbl = new QLabel( m_toolBar );
	snap_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	//Set up quantization/snapping selector
	m_snappingComboBox = new ComboBox( m_toolBar );
	m_snappingComboBox->setFixedSize( 80, 22 );
	m_snappingComboBox->setModel(m_editor->m_snappingModel);
	m_snappingComboBox->setToolTip(tr("Clip snapping size"));
	connect(m_editor->snappingModel(), SIGNAL(dataChanged()), this, SLOT(updateSnapLabel()));

	m_setProportionalSnapAction = new QAction(embed::getIconPixmap("proportional_snap"),
											 tr("Toggle proportional snap on/off"), this);
	m_setProportionalSnapAction->setCheckable(true);
	m_setProportionalSnapAction->setChecked(false);
	connect(m_setProportionalSnapAction, SIGNAL(triggered()), m_editor, SLOT(toggleProportionalSnap()));
	connect(m_setProportionalSnapAction, SIGNAL(triggered()), this, SLOT(updateSnapLabel()) );

	snapToolBar->addWidget( snap_lbl );
	snapToolBar->addWidget( m_snappingComboBox );
	snapToolBar->addSeparator();
	snapToolBar->addAction( m_setProportionalSnapAction );

	snapToolBar->addSeparator();
	snapToolBar->addWidget( m_snapSizeLabel );

	connect(song, SIGNAL(projectLoaded()), this, SLOT(adjustUiAfterProjectLoad()));
	connect(this, SIGNAL(resized()), m_editor, SLOT(updatePositionLine()));
}

QSize SongEditorWindow::sizeHint() const
{
	return {720, 300};
}

void SongEditorWindow::updateSnapLabel(){
	if (m_setProportionalSnapAction->isChecked())
	{
		m_snapSizeLabel->setText(QString("Snap: ") + m_editor->getSnapSizeString());
		m_snappingComboBox->setToolTip(tr("Base snapping size"));
	}
	else
	{
		m_snappingComboBox->setToolTip(tr("Clip snapping size"));
		m_snapSizeLabel->clear();
	}
}




void SongEditorWindow::resizeEvent(QResizeEvent *event)
{
	emit resized();
}


void SongEditorWindow::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);
	if (event->type() == QEvent::WindowStateChange)
	{
		m_editor->realignTracks();
	}
}


void SongEditorWindow::play()
{
	emit playTriggered();
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




void SongEditorWindow::lostFocus()
{
	if( m_crtlAction )
	{
		m_crtlAction->setChecked( true );
		m_crtlAction->trigger();
	}
}




void SongEditorWindow::adjustUiAfterProjectLoad()
{
	// make sure to bring us to front as the song editor is the central
	// widget in a song and when just opening a song in order to listen to
	// it, it's very annyoing to manually bring up the song editor each time
	gui->mainWindow()->workspace()->setActiveSubWindow(
			qobject_cast<QMdiSubWindow *>( parentWidget() ) );
	connect( qobject_cast<SubWindow *>( parentWidget() ), SIGNAL( focusLost() ), this, SLOT( lostFocus() ) );
	m_editor->scrolled(0);
}




void SongEditorWindow::keyPressEvent( QKeyEvent *ke )
{
	if( ke->key() == Qt::Key_Control )
	{
		m_crtlAction = m_editModeGroup->checkedAction();
		m_selectModeAction->setChecked( true );
		m_selectModeAction->trigger();
	}
}




void SongEditorWindow::keyReleaseEvent( QKeyEvent *ke )
{
	if( ke->key() == Qt::Key_Control )
	{
		if( m_crtlAction )
		{
			m_crtlAction->setChecked( true );
			m_crtlAction->trigger();
		}
	}
}
