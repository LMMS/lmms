/*
 * PatternEditor.cpp - basic main-window for editing pattern clips
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "PatternEditor.h"

#include <QAction>
#include <QKeyEvent>
#include <QLayout>

#include "ComboBox.h"
#include "DataFile.h"
#include "embed.h"
#include "MainWindow.h"
#include "PatternTrack.h"
#include "PatternTrackContainer.h"
#include "Song.h"
#include "StringPairDrag.h"

#include "MidiClip.h"



PatternEditor::PatternEditor( PatternTrackContainer* tc ) :
	Editor(false),
	m_trackContainerView( new PatternTrackContainerView(tc) )
{
	setWindowIcon( embed::getIconPixmap( "pattern_track_btn" ) );
	setWindowTitle( tr( "Pattern Editor" ) );
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
			     + 2 * CLIP_BORDER_WIDTH + 384 );
	}
	else
	{
		setMinimumWidth( TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH
			     + 2 * CLIP_BORDER_WIDTH + 384 );
	}


	m_playAction->setToolTip(tr( "Play/pause current pattern (Space)" ));
	m_stopAction->setToolTip(tr( "Stop playback of current pattern (Space)" ));


	// Pattern selector
	DropToolBar *patternSelectionToolBar = addDropToolBarToTop(tr("Pattern selector"));

	m_patternComboBox = new ComboBox( m_toolBar );
	m_patternComboBox->setFixedSize( 200, ComboBox::DEFAULT_HEIGHT );
	m_patternComboBox->setModel( &tc->m_patternComboBoxModel );

	patternSelectionToolBar->addWidget( m_patternComboBox );


	// Track actions
	DropToolBar *trackAndStepActionsToolBar = addDropToolBarToTop(tr("Track and step actions"));


	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_pattern_track"), tr("New pattern"),
						 Engine::getSong(), SLOT(addPatternTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("clone_pattern_track_clip"), tr("Clone pattern"),
						 m_trackContainerView, SLOT(cloneClip()));
	trackAndStepActionsToolBar->addAction(
				embed::getIconPixmap("add_sample_track"),
				tr("Add sample-track"), m_trackContainerView,
				SLOT(addSampleTrack()));
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

	connect( &tc->m_patternComboBoxModel, SIGNAL( dataChanged() ),
			m_trackContainerView, SLOT( updatePosition() ) );


	QAction* viewNext = new QAction(this);
	connect(viewNext, SIGNAL(triggered()), m_patternComboBox, SLOT(selectNext()));
	viewNext->setShortcut(Qt::Key_Plus);
	addAction(viewNext);

	QAction* viewPrevious = new QAction(this);
	connect(viewPrevious, SIGNAL(triggered()), m_patternComboBox, SLOT(selectPrevious()));
	viewPrevious->setShortcut(Qt::Key_Minus);
	addAction(viewPrevious);
}


PatternEditor::~PatternEditor()
{
}


QSize PatternEditor::sizeHint() const
{
	return {minimumWidth()+10, 300};
}


void PatternEditor::removePatternView(int pattern)
{
	m_trackContainerView->removePatternView(pattern);
}


void PatternEditor::play()
{
	if( Engine::getSong()->playMode() != Song::Mode_PlayPattern )
	{
		Engine::getSong()->playPattern();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}


void PatternEditor::stop()
{
	Engine::getSong()->stop();
}




PatternTrackContainerView::PatternTrackContainerView(PatternTrackContainer* tc) :
	TrackContainerView(tc),
	m_ptc(tc)
{
	setModel( tc );
}




void PatternTrackContainerView::addSteps()
{
	makeSteps( false );
}

void PatternTrackContainerView::cloneSteps()
{
	makeSteps( true );
}




void PatternTrackContainerView::removeSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			MidiClip* p = static_cast<MidiClip *>( ( *it )->getClip( m_ptc->currentPattern() ) );
			p->removeSteps();
		}
	}
}




void PatternTrackContainerView::addSampleTrack()
{
	(void) Track::create( Track::SampleTrack, model() );
}




void PatternTrackContainerView::addAutomationTrack()
{
	(void) Track::create( Track::AutomationTrack, model() );
}




void PatternTrackContainerView::removePatternView(int pattern)
{
	for( TrackView* view : trackViews() )
	{
		view->getTrackContentWidget()->removeClipView(pattern);
	}
}



void PatternTrackContainerView::saveSettings(QDomDocument& doc, QDomElement& element)
{
	MainWindow::saveWidgetState( parentWidget(), element );
}

void PatternTrackContainerView::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void PatternTrackContainerView::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );

		// Ensure pattern clips exist
		bool hasValidPatternClips = false;
		if (t->getClips().size() == m_ptc->numOfPatterns())
		{
			hasValidPatternClips = true;
			for (int i = 0; i < t->getClips().size(); ++i)
			{
				if (t->getClips()[i]->startPosition() != TimePos(i, 0))
				{
					hasValidPatternClips = false;
					break;
				}
			}
		}
		if (!hasValidPatternClips)
		{
			t->deleteClips();
			t->createClipsForPattern(m_ptc->numOfPatterns() - 1);
		}
		m_ptc->updateAfterTrackAdd();

		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}




void PatternTrackContainerView::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void PatternTrackContainerView::makeSteps( bool clone )
{
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			MidiClip* p = static_cast<MidiClip *>( ( *it )->getClip( m_ptc->currentPattern() ) );
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

// Creates a clone of the current pattern track with the same clip, but no Clips in the song editor
// TODO: Avoid repeated code from cloneTrack and clearTrack in TrackOperationsWidget somehow
void PatternTrackContainerView::cloneClip()
{
	// Get the current PatternTrack id
	PatternTrackContainer *ptc = static_cast<PatternTrackContainer*>(model());
	const int currentPattern = ptc->currentPattern();

	PatternTrack *pt = PatternTrack::findPatternTrack(currentPattern);

	if( pt )
	{
		// Clone the track
		Track *newTrack = pt->clone();
		ptc->setCurrentPattern( static_cast<PatternTrack *>( newTrack )->index() );

		// Track still have the clips which is undesirable in this case, clear the track
		newTrack->lock();
		newTrack->deleteClips();
		newTrack->unlock();
	}
}
