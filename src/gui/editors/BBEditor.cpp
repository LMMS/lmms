/*
 * BBEditor.cpp - basic main-window for editing of beats and basslines
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "BBEditor.h"

#include <QAction>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>

#include "ComboBox.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "MainWindow.h"
#include "Song.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "StringPairDrag.h"

#include "TrackContainer.h"
#include "Pattern.h"



BBEditor::BBEditor( BBTrackContainer* tc ) :
	Editor(false),
	m_trackContainerView( new BBTrackContainerView(tc) )
{
	setWindowIcon( embed::getIconPixmap( "bb_track_btn" ) );
	setWindowTitle( tr( "Beat+Bassline Editor" ) );
	setCentralWidget(m_trackContainerView);

	setAcceptDrops(true);
	m_toolBar->setAcceptDrops(true);
	connect(m_toolBar, SIGNAL(dragEntered(QDragEnterEvent*)), m_trackContainerView, SLOT(dragEnterEvent(QDragEnterEvent*)));
	connect(m_toolBar, SIGNAL(dropped(QDropEvent*)), m_trackContainerView, SLOT(dropEvent(QDropEvent*)));

	// TODO: Use style sheet
	if( ConfigManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		setMinimumWidth( TRACK_OP_WIDTH_COMPACT + DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT
			     + 2 * TCO_BORDER_WIDTH + 264 );
	}
	else
	{
		setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH
			     + 2 * TCO_BORDER_WIDTH + 264 );
	}


	m_playAction->setToolTip(tr( "Play/pause current beat/bassline (Space)" ));
	m_stopAction->setToolTip(tr( "Stop playback of current beat/bassline (Space)" ));

	m_playAction->setWhatsThis(
		tr( "Click here to play the current "
			"beat/bassline.  The beat/bassline is automatically "
			"looped when its end is reached." ) );
	m_stopAction->setWhatsThis(
		tr( "Click here to stop playing of current "
							"beat/bassline." ) );


	// Beat selector
	DropToolBar *beatSelectionToolBar = addDropToolBarToTop(tr("Beat selector"));

	m_bbComboBox = new ComboBox( m_toolBar );
	m_bbComboBox->setFixedSize( 200, 22 );
	m_bbComboBox->setModel( &tc->m_bbComboBoxModel );

	beatSelectionToolBar->addWidget( m_bbComboBox );


	// Track actions
	DropToolBar *trackAndStepActionsToolBar = addDropToolBarToTop(tr("Track and step actions"));


	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_bb_track"), tr("Add beat/bassline"),
						 Engine::getSong(), SLOT(addBBTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_automation"), tr("Add automation-track"),
						 m_trackContainerView, SLOT(addAutomationTrack()));

	QWidget* stretch = new QWidget(m_toolBar);
	stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	trackAndStepActionsToolBar->addWidget(stretch);


	// Step actions
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_remove"), tr("Remove steps"),
						 m_trackContainerView, SLOT(removeSteps()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_add"), tr("Add steps"),
						 m_trackContainerView, SLOT( addSteps()));
	trackAndStepActionsToolBar->addAction( embed::getIconPixmap( "step_btn_duplicate" ), tr( "Clone Steps" ),
						  m_trackContainerView, SLOT( cloneSteps() ) );

	connect( &tc->m_bbComboBoxModel, SIGNAL( dataChanged() ),
			m_trackContainerView, SLOT( updatePosition() ) );


	QAction* viewNext = new QAction(this);
	connect(viewNext, SIGNAL(triggered()), m_bbComboBox, SLOT(selectNext()));
	viewNext->setShortcut(Qt::Key_Plus);
	addAction(viewNext);

	QAction* viewPrevious = new QAction(this);
	connect(viewPrevious, SIGNAL(triggered()), m_bbComboBox, SLOT(selectPrevious()));
	viewPrevious->setShortcut(Qt::Key_Minus);
	addAction(viewPrevious);
}


BBEditor::~BBEditor()
{
}


QSize BBEditor::sizeHint() const
{
	return {minimumWidth()+10, 300};
}


void BBEditor::removeBBView( int bb )
{
	m_trackContainerView->removeBBView(bb);
}


void BBEditor::play()
{
	if( Engine::getSong()->playMode() != Song::Mode_PlayBB )
	{
		Engine::getSong()->playBB();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}


void BBEditor::stop()
{
	Engine::getSong()->stop();
}




BBTrackContainerView::BBTrackContainerView(BBTrackContainer* tc) :
	TrackContainerView(tc),
	m_bbtc(tc)
{
	setModel( tc );

	connect ( m_bbtc, SIGNAL( postTrackAdded ( Track * ) ),
			this, SLOT( postTrackWasAdded( Track * ) ) );
}




void BBTrackContainerView::addSteps()
{
	makeSteps( false );
}

void BBTrackContainerView::cloneSteps()
{
	makeSteps( true );
}




void BBTrackContainerView::removeSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->removeSteps();
		}
	}
}




void BBTrackContainerView::addAutomationTrack()
{
	(void) Track::create( Track::AutomationTrack, model() );
}




void BBTrackContainerView::removeBBView(int bb)
{
	for( TrackView* view : trackViews() )
	{
		view->getTrackContentWidget()->removeTCOView( bb );
	}
}



void BBTrackContainerView::saveSettings(QDomDocument& doc, QDomElement& element)
{
	MainWindow::saveWidgetState(parentWidget(), element, QSize( 640, 400 ) );
}

void BBTrackContainerView::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void BBTrackContainerView::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );

		t->deleteTCOs();
		m_bbtc->updateAfterTrackAdd();

		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}




void BBTrackContainerView::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void BBTrackContainerView::postTrackWasAdded( Track * track )
{
	resizeNewTrack(track);
}




void BBTrackContainerView::makeSteps( bool clone )
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			if( clone )
			{
				p->cloneSteps();
			} else
			{
				p->addSteps();
			}
		}
	}
}




void BBTrackContainerView::resizeNewTrack(Track * track)
{
	TrackContainer::TrackList tl = model()->tracks();

	// Resizing is unnecessary if there does not already exist a track
	if(tl.size() <= 1)
	{
		return;
	}

	// Ensure that the newly added track has a TCO
	m_bbtc->updateAfterTrackAdd();

	// Default a single tact if there does not exist tracks in the Beat/Baseline
	// editor
	int steps = MidiTime::stepsPerTact();

	// get size of pre-existing track
	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it)
	{
		if( ( *it )->type() == Track::InstrumentTrack &&
			( *it ) != track )
		{
			std::cout << m_bbtc->currentBB() << std::endl;
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			steps = p->getSteps();
			break;
		}
	}

	std::cout << m_bbtc->currentBB() << std::endl;
	Pattern* p = static_cast<Pattern *>( track->getTCO( m_bbtc->currentBB() ) );

	// If the track already has the same amount of steps, no need to resize
	// (For example, when cloning a track)
	if(p->getSteps() == steps)
	{
		return;
	}

	// convert steps to tacts
	int tacts = steps / MidiTime::stepsPerTact();

	// A newly created track has 1 tact, so increment tacts by "tacts - 1"
	while(--tacts > 0)
	{
		p->addSteps();
	}
}
