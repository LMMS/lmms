/*
 * PatternEditor.cpp - basic main-window for editing patterns
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

#include "ClipView.h"
#include "ComboBox.h"
#include "DataFile.h"
#include "embed.h"
#include "MainWindow.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TrackView.h"

#include "MidiClip.h"


namespace lmms::gui
{


PatternEditor::PatternEditor(PatternStore* ps) :
	TrackContainerView(ps),
	m_ps(ps)
{
	setModel(ps);
}




void PatternEditor::addSteps()
{
	makeSteps( false );
}

void PatternEditor::cloneSteps()
{
	makeSteps( true );
}




void PatternEditor::removeSteps()
{
	TrackContainer::TrackList tl = model()->tracks();

	for (const auto& track : tl)
	{
		if (track->type() == Track::InstrumentTrack)
		{
			auto p = static_cast<MidiClip*>(track->getClip(m_ps->currentPattern()));
			p->removeSteps();
		}
	}
}




void PatternEditor::addSampleTrack()
{
	(void) Track::create( Track::SampleTrack, model() );
}




void PatternEditor::addAutomationTrack()
{
	(void) Track::create( Track::AutomationTrack, model() );
}




void PatternEditor::removeViewsForPattern(int pattern)
{
	for( TrackView* view : trackViews() )
	{
		view->getTrackContentWidget()->removeClipView(pattern);
	}
}



void PatternEditor::saveSettings(QDomDocument& doc, QDomElement& element)
{
	MainWindow::saveWidgetState( parentWidget(), element );
}

void PatternEditor::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void PatternEditor::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );

		// Ensure pattern clips exist
		bool hasValidPatternClips = false;
		if (t->getClips().size() == m_ps->numOfPatterns())
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
			t->createClipsForPattern(m_ps->numOfPatterns() - 1);
		}
		m_ps->updateAfterTrackAdd();

		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}




void PatternEditor::updatePosition()
{
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void PatternEditor::makeSteps( bool clone )
{
	TrackContainer::TrackList tl = model()->tracks();

	for (const auto& track : tl)
	{
		if (track->type() == Track::InstrumentTrack)
		{
			auto p = static_cast<MidiClip*>(track->getClip(m_ps->currentPattern()));
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

// Creates a clone of the current pattern track with the same content, but no clips in the song editor
// TODO: Avoid repeated code from cloneTrack and clearTrack in TrackOperationsWidget somehow
void PatternEditor::cloneClip()
{
	// Get the current PatternTrack id
	auto ps = static_cast<PatternStore*>(model());
	const int currentPattern = ps->currentPattern();

	PatternTrack* pt = PatternTrack::findPatternTrack(currentPattern);

	if (pt)
	{
		// Clone the track
		Track* newTrack = pt->clone();
		ps->setCurrentPattern(static_cast<PatternTrack*>(newTrack)->patternIndex());

		// Track still have the clips which is undesirable in this case, clear the track
		newTrack->lock();
		newTrack->deleteClips();
		newTrack->unlock();
	}
}




PatternEditorWindow::PatternEditorWindow(PatternStore* ps) :
	Editor(false),
	m_editor(new PatternEditor(ps))
{
	setWindowIcon(embed::getIconPixmap("pattern_track_btn"));
	setWindowTitle(tr("Pattern Editor"));
	setCentralWidget(m_editor);

	setAcceptDrops(true);
	m_toolBar->setAcceptDrops(true);
	connect(m_toolBar, SIGNAL(dragEntered(QDragEnterEvent*)), m_editor, SLOT(dragEnterEvent(QDragEnterEvent*)));
	connect(m_toolBar, SIGNAL(dropped(QDropEvent*)), m_editor, SLOT(dropEvent(QDropEvent*)));

	// TODO: Use style sheet
	if (ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt())
	{
		setMinimumWidth(TRACK_OP_WIDTH_COMPACT + DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + 2 * ClipView::BORDER_WIDTH + 384);
	}
	else
	{
		setMinimumWidth(TRACK_OP_WIDTH + DEFAULT_SETTINGS_WIDGET_WIDTH + 2 * ClipView::BORDER_WIDTH + 384);
	}

	m_playAction->setToolTip(tr("Play/pause current pattern (Space)"));
	m_stopAction->setToolTip(tr("Stop playback of current pattern (Space)"));


	// Pattern selector
	DropToolBar* patternSelectionToolBar = addDropToolBarToTop(tr("Pattern selector"));

	m_patternComboBox = new ComboBox(m_toolBar);
	m_patternComboBox->setFixedSize(200, ComboBox::DEFAULT_HEIGHT);
	m_patternComboBox->setModel(&ps->m_patternComboBoxModel);

	patternSelectionToolBar->addWidget(m_patternComboBox);


	// Track actions
	DropToolBar *trackAndStepActionsToolBar = addDropToolBarToTop(tr("Track and step actions"));


	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_pattern_track"), tr("New pattern"),
						Engine::getSong(), SLOT(addPatternTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("clone_pattern_track_clip"), tr("Clone pattern"),
						m_editor, SLOT(cloneClip()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_sample_track"),	tr("Add sample-track"),
						m_editor, SLOT(addSampleTrack()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("add_automation"), tr("Add automation-track"),
						m_editor, SLOT(addAutomationTrack()));

	auto stretch = new QWidget(m_toolBar);
	stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	trackAndStepActionsToolBar->addWidget(stretch);


	// Step actions
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_remove"), tr("Remove steps"),
						m_editor, SLOT(removeSteps()));
	trackAndStepActionsToolBar->addAction(embed::getIconPixmap("step_btn_add"), tr("Add steps"),
						m_editor, SLOT(addSteps()));
	trackAndStepActionsToolBar->addAction( embed::getIconPixmap("step_btn_duplicate"), tr("Clone Steps"),
						m_editor, SLOT(cloneSteps()));

	connect(&ps->m_patternComboBoxModel, SIGNAL(dataChanged()),
			m_editor, SLOT(updatePosition()));

	auto viewNext = new QAction(this);
	connect(viewNext, SIGNAL(triggered()), m_patternComboBox, SLOT(selectNext()));
	viewNext->setShortcut(Qt::Key_Plus);
	addAction(viewNext);

	auto viewPrevious = new QAction(this);
	connect(viewPrevious, SIGNAL(triggered()), m_patternComboBox, SLOT(selectPrevious()));
	viewPrevious->setShortcut(Qt::Key_Minus);
	addAction(viewPrevious);
}


QSize PatternEditorWindow::sizeHint() const
{
	return {minimumWidth() + 10, 300};
}


void PatternEditorWindow::play()
{
	if (Engine::getSong()->playMode() != Song::Mode_PlayPattern)
	{
		Engine::getSong()->playPattern();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}


void PatternEditorWindow::stop()
{
	Engine::getSong()->stop();
}


} // namespace lmms::gui