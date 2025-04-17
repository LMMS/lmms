/*
 * PatternClipView.cpp
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

#include "PatternClipView.h"

#include <set>

#include <QApplication>
#include <QMenu>
#include <QPainter>

#include "AutomationClip.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "PatternClip.h"
#include "PatternTrack.h"
#include "PatternTrackView.h"
#include "PatternStore.h"
#include "RenameDialog.h"
#include "SampleClip.h"
#include "Song.h"

namespace lmms::gui
{


PatternClipView::PatternClipView(Clip* _clip, TrackView* _tv) :
	ClipView( _clip, _tv ),
	m_patternClip(dynamic_cast<PatternClip*>(_clip)),
	m_paintPixmap()
{
	connect( _clip->getTrack(), SIGNAL(dataChanged()), 
			this, SLOT(update()));

	setStyle( QApplication::style() );
}

void PatternClipView::constructContextMenu(QMenu* _cm)
{
	auto a = new QAction(embed::getIconPixmap("pattern_track"), tr("Open in Pattern Editor"), _cm);
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL(triggered(bool)),
			this, SLOT(openInPatternEditor()));
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT(resetName()));
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT(changeName()));
	if (canCopySelectionToNewTrack())
	{
		_cm->addAction(
			embed::getIconPixmap("pattern_track"),
			tr("Copy to New Pattern Track"),
			this,
			&PatternClipView::copySelectionToNewPatternTrack
		);
	}
}

bool PatternClipView::canCopySelectionToNewTrack()
{
	QVector<ClipView*> clipvs = getClickedClips();
	// We check if the owner of the first Clip is a Pattern Track
	bool isPatternTrack = dynamic_cast<PatternTrackView*>(clipvs.at(0)->getTrackView());
	if (!isPatternTrack) { return false; }
	// Make sure every track is a pattern track
	for (auto clipv: clipvs) {
		if (!dynamic_cast<PatternTrackView*>(clipv->getTrackView())) { return false; }
	}
	return true;
}

void PatternClipView::copySelectionToNewPatternTrack()
{
	QVector<ClipView*> clipvs = getClickedClips();
	std::set<TrackView*> ownerTracks;

	// Find the first clip's start position in the selection
	TimePos firstClipStartPos = m_patternClip->startPosition();
	for (auto clipv: clipvs)
	{
		firstClipStartPos = std::min(firstClipStartPos, clipv->getClip()->startPosition());
		// And build up a set of the affected pattern tracks
		ownerTracks.insert(clipv->getTrackView());
	}

	// Create new pattern track and clip
	Track* new_track = Track::create(Track::Type::Pattern, Engine::getSong());
	PatternClip* new_clip = new PatternClip(new_track);

	new_clip->movePosition(firstClipStartPos);

	//const int oldPatternTrackIndex = static_cast<PatternTrack*>(m_patternClip->getTrack())->patternIndex();
	const int newPatternTrackIndex = static_cast<PatternTrack*>(new_track)->patternIndex();

	TimePos maxNotePos = 0;

	for (const auto& patternTrack : ownerTracks)
	{
		for (const auto& track : Engine::patternStore()->tracks())
		{
			int ownerPatternTrackIndex = static_cast<PatternTrack*>(patternTrack->getTrack())->patternIndex();
			Clip* clip = track->getClip(ownerPatternTrackIndex);
			auto sClip = dynamic_cast<SampleClip*>(clip);
			auto mClip = dynamic_cast<MidiClip*>(clip);
			auto aClip = dynamic_cast<AutomationClip*>(clip);
			Clip* newClip = track->getClip(newPatternTrackIndex);
			if (sClip)
			{
				// TODO
				Clip::copyStateTo(clip, newClip);
			}
			else if (mClip)
			{
				MidiClip* newMidiClip = dynamic_cast<MidiClip*>(newClip);
				for (auto clipv: clipvs)
				{
					if (clipv->getTrackView() != patternTrack) { continue; }
					// Figure out how many times this clip repeats itself. At maximum it could touch (length roudned up + 1) bars
					// when accounting for the fact that the start offset could make it play the end of a bar before starting the first full bar.
					// Here we go the safe way and iterate through the maximum possible repetitions, and discard any notes outside of the range.
					// First +1 for ceiling, second +1 for possible previous bar.
					int maxPossibleRepetitions = clipv->getClip()->length() / mClip->length() + 1 + 1; 

					TimePos clipRelativePos = clipv->getClip()->startPosition() - firstClipStartPos;
					TimePos startTimeOffset = clipv->getClip()->startTimeOffset();

					for (Note const* note : mClip->notes())
					{
						// Start loop at i = -1 to get the bar touched by start offset
						for (int i = -1; i < maxPossibleRepetitions - 1; i++)
						{
							auto newNote = Note{*note};

							TimePos newNotePos = note->pos() + clipRelativePos + startTimeOffset + i * mClip->length().nextFullBar() * TimePos::ticksPerBar();
							TimePos newNotePosRelativeToClip = note->pos() + startTimeOffset + i * mClip->length().nextFullBar()  * TimePos::ticksPerBar();
							
							if (newNotePosRelativeToClip < 0 || newNotePosRelativeToClip >= clipv->getClip()->length()) { continue; }

							newNote.setPos(newNotePos);
							newMidiClip->addNote(newNote, false);
							maxNotePos = std::max(maxNotePos, newNotePos);
						}
					}
				}
			}
			else if (aClip)
			{
				// TODO
				Clip::copyStateTo(clip, newClip);
			}
		}
	}
	// Update the number of steps/bars for all tracks. For some reason addNote for midi clips does not update the length automatically.
	for (const auto& track : Engine::patternStore()->tracks())
	{
		for (int i = 0; i < maxNotePos.getBar(); i++)
		{
			static_cast<MidiClip*>(track->getClip(newPatternTrackIndex))->addSteps();
		}
	}

	// Now that we know the maximum number of bars, set the length of the new pattern clip
	new_clip->changeLength(maxNotePos.nextFullBar() * TimePos::ticksPerBar());
}

void PatternClipView::mouseDoubleClickEvent(QMouseEvent*)
{
	openInPatternEditor();
}




void PatternClipView::paintEvent(QPaintEvent*)
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	
	lingrad.setColorAt( 0, c.lighter( 130 ) );
	lingrad.setColorAt( 1, c.lighter( 70 ) );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}
	
	// bar lines
	const int lineSize = 3;
	int pixelsPerPattern = Engine::patternStore()->lengthOfPattern(m_patternClip->patternIndex()) * pixelsPerBar();
	int offset = static_cast<int>(m_patternClip->startTimeOffset() * (pixelsPerBar() / TimePos::ticksPerBar()))
			% pixelsPerPattern;
	if (offset < 2) {
		offset += pixelsPerPattern;
	}

	p.setPen( c.darker( 200 ) );

	if (pixelsPerPattern > 0)
	{
		for (int x = offset; x < width() - 2; x += pixelsPerPattern)
		{
			p.drawLine( x, BORDER_WIDTH, x, BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - BORDER_WIDTH );
		}
	}

	// clip name
	paintTextLabel(m_patternClip->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
		rect().bottom() - BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the clip was manualy muted
	if (m_patternClip->isMuted())
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void PatternClipView::openInPatternEditor()
{
	Engine::patternStore()->setCurrentPattern(m_patternClip->patternIndex());

	getGUI()->mainWindow()->togglePatternEditorWin(true);
}




void PatternClipView::resetName() { m_patternClip->setName(""); }




void PatternClipView::changeName()
{
	QString s = m_patternClip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_patternClip->setName(s);
}



void PatternClipView::update()
{
	setToolTip(m_patternClip->name());

	ClipView::update();
}


} // namespace lmms::gui
