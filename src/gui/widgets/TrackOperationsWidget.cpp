/*
 * TrackOperationsWidget.cpp - implementation of TrackOperationsWidget class
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

#include "TrackOperationsWidget.h"

#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QCheckBox>

#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "ColorChooser.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "PixmapButton.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "ToolTip.h"
#include "Track.h"
#include "TrackContainerView.h"
#include "TrackView.h"

/*! \brief Create a new trackOperationsWidget
 *
 * The trackOperationsWidget is the grip and the mute button of a track.
 *
 * \param parent the trackView to contain this widget
 */
TrackOperationsWidget::TrackOperationsWidget( TrackView * parent ) :
	QWidget( parent ),             /*!< The parent widget */
	m_trackView( parent )          /*!< The parent track view */
{
	ToolTip::add( this, tr( "Press <%1> while clicking on move-grip "
				"to begin a new drag'n'drop action." ).arg(UI_CTRL_KEY) );

	QMenu * toMenu = new QMenu( this );
	toMenu->setFont( pointSize<9>( toMenu->font() ) );
	connect( toMenu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );


	setObjectName( "automationEnabled" );


	m_trackOps = new QPushButton( this );
	m_trackOps->move( 12, 1 );
	m_trackOps->setFocusPolicy( Qt::NoFocus );
	m_trackOps->setMenu( toMenu );
	ToolTip::add( m_trackOps, tr( "Actions" ) );


	m_muteBtn = new PixmapButton( this, tr( "Mute" ) );
	m_muteBtn->setActiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_muteBtn->setInactiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_muteBtn->setCheckable( true );

	m_soloBtn = new PixmapButton( this, tr( "Solo" ) );
	m_soloBtn->setActiveGraphic( embed::getIconPixmap( "led_red" ) );
	m_soloBtn->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_soloBtn->setCheckable( true );

	if( ConfigManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		m_muteBtn->move( 46, 0 );
		m_soloBtn->move( 46, 16 );
	}
	else
	{
		m_muteBtn->move( 46, 8 );
		m_soloBtn->move( 62, 8 );
	}

	m_muteBtn->show();
	ToolTip::add( m_muteBtn, tr( "Mute" ) );

	m_soloBtn->show();
	ToolTip::add( m_soloBtn, tr( "Solo" ) );

	connect( this, SIGNAL( trackRemovalScheduled( TrackView * ) ),
			m_trackView->trackContainerView(),
				SLOT( deleteTrackView( TrackView * ) ),
							Qt::QueuedConnection );

	connect( m_trackView->getTrack()->getMutedModel(), SIGNAL( dataChanged() ),
			this, SLOT( update() ) );

}




/*! \brief Destroy an existing trackOperationsWidget
 *
 */
TrackOperationsWidget::~TrackOperationsWidget()
{
}




/*! \brief Respond to trackOperationsWidget mouse events
 *
 *  If it's the left mouse button, and Ctrl is held down, and we're
 *  not a Beat+Bassline Editor track, then start a new drag event to
 *  copy this track.
 *
 *  Otherwise, ignore all other events.
 *
 *  \param me The mouse event to respond to.
 */
void TrackOperationsWidget::mousePressEvent( QMouseEvent * me )
{
	if( me->button() == Qt::LeftButton &&
		me->modifiers() & Qt::ControlModifier &&
			m_trackView->getTrack()->type() != Track::BBTrack )
	{
		DataFile dataFile( DataFile::DragNDropData );
		m_trackView->getTrack()->saveState( dataFile, dataFile.content() );
		new StringPairDrag( QString( "track_%1" ).arg(
					m_trackView->getTrack()->type() ),
			dataFile.toString(), m_trackView->getTrackSettingsWidget()->grab(),
									this );
	}
	else if( me->button() == Qt::LeftButton )
	{
		// track-widget (parent-widget) initiates track-move
		me->ignore();
	}
}




/*! \brief Repaint the trackOperationsWidget
 *
 *  If we're not moving, and in the Beat+Bassline Editor, then turn
 *  automation on or off depending on its previous state and show
 *  ourselves.
 *
 *  Otherwise, hide ourselves.
 *
 *  \todo Flesh this out a bit - is it correct?
 *  \param pe The paint event to respond to
 */
void TrackOperationsWidget::paintEvent( QPaintEvent * pe )
{
	QPainter p( this );

	p.fillRect( rect(), palette().brush(QPalette::Background) );

	if( m_trackView->getTrack()->useColor() && ! m_trackView->getTrack()->getMutedModel()->value() ) 
	{
		QRect coloredRect( 0, 0, 10, m_trackView->getTrack()->getHeight() );
		
		p.fillRect( coloredRect, m_trackView->getTrack()->color() );
	}

	if( m_trackView->isMovingTrack() == false )
	{
		p.drawPixmap( 2, 2, embed::getIconPixmap("track_op_grip"));
	}
	else
	{
		p.drawPixmap( 2, 2, embed::getIconPixmap("track_op_grip_c"));
	}
}


/*! \brief Show a message box warning the user that this track is about to be closed */
bool TrackOperationsWidget::confirmRemoval()
{
	bool needConfirm = ConfigManager::inst()->value("ui", "trackdeletionwarning", "1").toInt();
	if (!needConfirm){ return true; }
	
	QString messageRemoveTrack = tr("After removing a track, it can not "
					"be recovered. Are you sure you want to remove track \"%1\"?")
					.arg(m_trackView->getTrack()->name());
	QString messageTitleRemoveTrack = tr("Confirm removal");
	QString askAgainText = tr("Don't ask again");
	QCheckBox* askAgainCheckBox = new QCheckBox(askAgainText, nullptr);
	connect(askAgainCheckBox, &QCheckBox::stateChanged, [this](int state){
		// Invert button state, if it's checked we *shouldn't* ask again
		ConfigManager::inst()->setValue("ui", "trackdeletionwarning", state ? "0" : "1");
	});

	QMessageBox mb(this);
	mb.setText(messageRemoveTrack);
	mb.setWindowTitle(messageTitleRemoveTrack);
	mb.setIcon(QMessageBox::Warning);
	mb.addButton(QMessageBox::Cancel);
	mb.addButton(QMessageBox::Ok);
	mb.setCheckBox(askAgainCheckBox);
	mb.setDefaultButton(QMessageBox::Cancel);

	int answer = mb.exec();

	if( answer == QMessageBox::Ok )
	{
		return true;
	}
	return false;
}



/*! \brief Clone this track
 *
 */
void TrackOperationsWidget::cloneTrack()
{
	TrackContainerView *tcView = m_trackView->trackContainerView();

	Track *newTrack = m_trackView->getTrack()->clone();
	TrackView *newTrackView = tcView->createTrackView( newTrack );

	int index = tcView->trackViews().indexOf( m_trackView );
	int i = tcView->trackViews().size();
	while ( i != index + 1 )
	{
		tcView->moveTrackView( newTrackView, i - 1 );
		i--;
	}
}


/*! \brief Clear this track - clears all TCOs from the track */
void TrackOperationsWidget::clearTrack()
{
	Track * t = m_trackView->getTrack();
	t->addJournalCheckPoint();
	t->lock();
	t->deleteTCOs();
	t->unlock();
}



/*! \brief Remove this track from the track list
 *
 */
void TrackOperationsWidget::removeTrack()
{
	if (confirmRemoval())
	{
		emit trackRemovalScheduled(m_trackView);
	}
}

void TrackOperationsWidget::changeTrackColor()
{
	QColor new_color = ColorChooser( this ).withPalette( ColorChooser::Palette::Track )-> \
		getColor( m_trackView->getTrack()->color() );

	if( ! new_color.isValid() )
	{ return; }

	emit colorChanged( new_color );

	Engine::getSong()->setModified();
	update();
}

void TrackOperationsWidget::resetTrackColor()
{
	emit colorReset();
	Engine::getSong()->setModified();
	update();
}

void TrackOperationsWidget::randomTrackColor()
{
	QColor buffer = ColorChooser::getPalette( ColorChooser::Palette::Track )[ rand() % 48 ];

	emit colorChanged( buffer );
	Engine::getSong()->setModified();
	update();
}

void TrackOperationsWidget::useTrackColor()
{
	emit colorParented();
	Engine::getSong()->setModified();
}


/*! \brief Update the trackOperationsWidget context menu
 *
 *  For all track types, we have the Clone and Remove options.
 *  For instrument-tracks we also offer the MIDI-control-menu
 *  For automation tracks, extra options: turn on/off recording
 *  on all TCOs (same should be added for sample tracks when
 *  sampletrack recording is implemented)
 */
void TrackOperationsWidget::updateMenu()
{
	QMenu * toMenu = m_trackOps->menu();
	toMenu->clear();
	toMenu->addAction( embed::getIconPixmap( "edit_copy", 16, 16 ),
						tr( "Clone this track" ),
						this, SLOT( cloneTrack() ) );
	toMenu->addAction( embed::getIconPixmap( "cancel", 16, 16 ),
						tr( "Remove this track" ),
						this, SLOT( removeTrack() ) );

	if( ! m_trackView->trackContainerView()->fixedTCOs() )
	{
		toMenu->addAction( tr( "Clear this track" ), this, SLOT( clearTrack() ) );
	}
	if (QMenu *fxMenu = m_trackView->createFxMenu(tr("FX %1: %2"), tr("Assign to new FX Channel")))
	{
		toMenu->addMenu(fxMenu);
	}

	if (InstrumentTrackView * trackView = dynamic_cast<InstrumentTrackView *>(m_trackView))
	{
		toMenu->addSeparator();
		toMenu->addMenu(trackView->midiMenu());
	}
	if( dynamic_cast<AutomationTrackView *>( m_trackView ) )
	{
		toMenu->addAction( tr( "Turn all recording on" ), this, SLOT( recordingOn() ) );
		toMenu->addAction( tr( "Turn all recording off" ), this, SLOT( recordingOff() ) );
	}

	toMenu->addSeparator();
	toMenu->addAction( embed::getIconPixmap( "colorize" ),
						tr( "Change color" ), this, SLOT( changeTrackColor() ) );
	toMenu->addAction( embed::getIconPixmap( "colorize" ),
						tr( "Reset color to default" ), this, SLOT( resetTrackColor() ) );
	toMenu->addAction( embed::getIconPixmap( "colorize" ),
						tr( "Set random color" ), this, SLOT( randomTrackColor() ) );
	toMenu->addSeparator();
	toMenu->addAction( embed::getIconPixmap( "colorize" ),
						tr( "Clear clip colors" ), this, SLOT( useTrackColor() ) );
}


void TrackOperationsWidget::toggleRecording( bool on )
{
	AutomationTrackView * atv = dynamic_cast<AutomationTrackView *>( m_trackView );
	if( atv )
	{
		for( TrackContentObject * tco : atv->getTrack()->getTCOs() )
		{
			AutomationPattern * ap = dynamic_cast<AutomationPattern *>( tco );
			if( ap ) { ap->setRecording( on ); }
		}
		atv->update();
	}
}



void TrackOperationsWidget::recordingOn()
{
	toggleRecording( true );
}


void TrackOperationsWidget::recordingOff()
{
	toggleRecording( false );
}

