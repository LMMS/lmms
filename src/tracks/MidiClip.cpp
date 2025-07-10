/*
 * MidiClip.cpp - implementation of class MidiClip, which holds notes
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
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

#include "MidiClip.h"

#include <algorithm>
#include <QDomElement>

#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "MidiClipView.h"
#include "PatternStore.h"
#include "PianoRoll.h"



namespace lmms
{

MidiClip::MidiClip( InstrumentTrack * _instrument_track ) :
	Clip( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_clipType( Type::BeatClip ),
	m_steps( TimePos::stepsPerBar() )
{
	if (_instrument_track->trackContainer()	== Engine::patternStore())
	{
		resizeToFirstTrack();
		setResizable(false);
	}
	else
	{
		setResizable(true);
	}
	init();
}




MidiClip::MidiClip( const MidiClip& other ) :
	Clip(other),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_clipType( other.m_clipType ),
	m_steps( other.m_steps )
{
	for (const auto& note : other.m_notes)
	{
		m_notes.push_back(note->clone());
	}

	init();
	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::Type::Pattern:
			setResizable(false);
			break;

		case TrackContainer::Type::Song:
			// move down
		default:
			setResizable(true);
			break;
	}
}


MidiClip::~MidiClip()
{
	emit destroyedMidiClip( this );

	for (const auto& note : m_notes)
	{
		delete note;
	}

	m_notes.clear();
}




void MidiClip::resizeToFirstTrack()
{
	// Resize this track to be the same as existing tracks in the pattern
	const TrackContainer::TrackList & tracks =
		m_instrumentTrack->trackContainer()->tracks();
	for (const auto& track : tracks)
	{
		if (track->type() == Track::Type::Instrument)
		{
			if (track != m_instrumentTrack)
			{
				const auto& instrumentTrackClips = m_instrumentTrack->getClips();
				const auto currentClipIt = std::find(instrumentTrackClips.begin(), instrumentTrackClips.end(), this);
				unsigned int currentClip = currentClipIt != instrumentTrackClips.end() ?
					std::distance(instrumentTrackClips.begin(), currentClipIt) : -1;
				m_steps = static_cast<MidiClip*>(track->getClip(currentClip))->m_steps;
			}
			break;
		}
	}
}




void MidiClip::init()
{
	connect( Engine::getSong(), SIGNAL(timeSignatureChanged(int,int)),
				this, SLOT(changeTimeSignature()));
	saveJournallingState( false );

	updateLength();
	restoreJournallingState();
}




void MidiClip::updateLength()
{
	if( m_clipType == Type::BeatClip )
	{
		changeLength( beatClipLength() );
		updatePatternTrack();
		return;
	}

	// If the clip has already been manually resized, don't automatically resize it.
	// Unless we are in a pattern, where you can't resize stuff manually
	if (getAutoResize() || !getResizable())
	{
		tick_t max_length = TimePos::ticksPerBar();

		for (const auto& note : m_notes)
		{
			if (note->length() > 0)
			{
				max_length = std::max<tick_t>(max_length, note->endPos());
			}
		}
		changeLength( TimePos( max_length ).nextFullBar() *
							TimePos::ticksPerBar() );
		setStartTimeOffset(TimePos(0));
		updatePatternTrack();
	}
}




TimePos MidiClip::beatClipLength() const
{
	tick_t max_length = TimePos::ticksPerBar();

	for (const auto& note : m_notes)
	{
		if (note->type() == Note::Type::Step)
		{
			max_length = std::max<tick_t>(max_length, note->pos() + 1);
		}
	}

	if (m_steps != TimePos::stepsPerBar())
	{
		max_length = m_steps * TimePos::ticksPerBar() / TimePos::stepsPerBar();
	}

	return TimePos{max_length}.nextFullBar() * TimePos::ticksPerBar();
}




Note * MidiClip::addNote( const Note & _new_note, const bool _quant_pos )
{
	auto new_note = _new_note.clone();
	if (_quant_pos && gui::getGUI()->pianoRoll())
	{
		new_note->quantizePos(gui::getGUI()->pianoRoll()->quantization());
	}

	instrumentTrack()->lock();
	m_notes.insert(std::upper_bound(m_notes.begin(), m_notes.end(), new_note, Note::lessThan), new_note);
	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();

	return new_note;
}




NoteVector::const_iterator MidiClip::removeNote(NoteVector::const_iterator it)
{
	instrumentTrack()->lock();
	delete *it;
	auto new_it = m_notes.erase(it);
	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();
	return new_it;
}

NoteVector::const_iterator MidiClip::removeNote(Note* note)
{
	instrumentTrack()->lock();

	auto it = std::find(m_notes.begin(), m_notes.end(), note);
	if (it != m_notes.end())
	{
		delete *it;
		it = m_notes.erase(it);
	}

	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();
	return it;
}


// Returns a pointer to the note at specified step, or nullptr if note doesn't exist
Note * MidiClip::noteAtStep(int step)
{
	for (const auto& note : m_notes)
	{
		if (note->pos() == TimePos::stepPosition(step)
			&& note->type() == Note::Type::Step)
		{
			return note;
		}
	}
	return nullptr;
}



void MidiClip::rearrangeAllNotes()
{
	// sort notes by start time
	std::sort(m_notes.begin(), m_notes.end(), Note::lessThan);
}



void MidiClip::clearNotes()
{
	instrumentTrack()->lock();
	for (const auto& note : m_notes)
	{
		delete note;
	}
	m_notes.clear();
	instrumentTrack()->unlock();

	checkType();
	updateLength();
	emit dataChanged();
}




Note * MidiClip::addStepNote( int step )
{
	Note stepNote = Note(TimePos(DefaultTicksPerBar / 16), TimePos::stepPosition(step));
	stepNote.setType(Note::Type::Step);

	return addNote(stepNote, false);
}




void MidiClip::setStep( int step, bool enabled )
{
	if( enabled )
	{
		if ( !noteAtStep( step ) )
		{
			addStepNote( step );
		}
		return;
	}

	while( Note * note = noteAtStep( step ) )
	{
		removeNote( note );
	}
}



void MidiClip::reverseNotes(const NoteVector& notes)
{
	if (notes.empty()) { return; }

	addJournalCheckPoint();

	// Find the very first start position and the very last end position of all the notes.
	TimePos firstPos = (*std::min_element(notes.begin(), notes.end(), [](const Note* n1, const Note* n2){ return Note::lessThan(n1, n2); }))->pos();
	TimePos lastPos = (*std::max_element(notes.begin(), notes.end(), [](const Note* n1, const Note* n2){ return n1->endPos() < n2->endPos(); }))->endPos();

	for (auto note : notes)
	{
		TimePos newStart = lastPos - (note->pos() - firstPos) - note->length();
		note->setPos(newStart);
	}

	rearrangeAllNotes();
	emit dataChanged();
}



void MidiClip::splitNotes(const NoteVector& notes, TimePos pos)
{
	if (notes.empty()) { return; }

	addJournalCheckPoint();

	for (const auto& note : notes)
	{
		int leftLength = pos.getTicks() - note->pos();
		int rightLength = note->length() - leftLength;

		// Split out of bounds
		if (leftLength <= 0 || rightLength <= 0)
		{
			continue;
		}

		// Reduce note length
		note->setLength(leftLength);

		// Add new note with the remaining length
		Note newNote = Note(*note);
		newNote.setLength(rightLength);
		newNote.setPos(note->pos() + leftLength);

		addNote(newNote, false);
	}
}

void MidiClip::splitNotesAlongLine(const NoteVector notes, TimePos pos1, int key1, TimePos pos2, int key2, bool deleteShortEnds)
{
	if (notes.empty()) { return; }

	// Don't split if the line is horitzontal
	if (key1 == key2) { return; }

	addJournalCheckPoint();

	const auto slope = 1.f * (pos2 - pos1) / (key2 - key1);
	const auto& [minKey, maxKey] = std::minmax(key1, key2);

	for (const auto& note : notes)
	{
		// Skip if the key is <= to minKey, since the line is drawn from the top of minKey to the top of maxKey, but only passes through maxKey - minKey - 1 total keys.
		if (note->key() <= minKey || note->key() > maxKey) { continue; }

		// Subtracting 0.5 to get the line's intercept at the "center" of the key, not the top.
		const TimePos keyIntercept = slope * (note->key() - 0.5 - key1) + pos1;
		if (note->pos() < keyIntercept && note->endPos() > keyIntercept)
		{
			auto newNote1 = Note{*note};
			newNote1.setLength(keyIntercept - note->pos());

			auto newNote2 = Note{*note};
			newNote2.setPos(keyIntercept);
			newNote2.setLength(note->endPos() - keyIntercept);

			if (deleteShortEnds)
			{
				addNote(newNote1.length() >= newNote2.length() ? newNote1 : newNote2, false);
			}
			else
			{
				addNote(newNote1, false);
				addNote(newNote2, false);
			}

			removeNote(note);
		}
	}
}



void MidiClip::setType( Type _new_clip_type )
{
	if( _new_clip_type == Type::BeatClip ||
				_new_clip_type == Type::MelodyClip )
	{
		m_clipType = _new_clip_type;
	}
}




void MidiClip::checkType()
{
	// If all notes are StepNotes, we have a BeatClip
	const auto beatClip = std::all_of(m_notes.begin(), m_notes.end(), [](auto note) { return note->type() == Note::Type::Step; });

	setType(beatClip ? Type::BeatClip : Type::MelodyClip);
}


void MidiClip::exportToXML(QDomDocument& doc, QDomElement& midiClipElement, bool onlySelectedNotes)
{
	midiClipElement.setAttribute("type", static_cast<int>(m_clipType));
	midiClipElement.setAttribute("name", name());
	midiClipElement.setAttribute("autoresize", QString::number(getAutoResize()));
	midiClipElement.setAttribute("off", startTimeOffset());
	
	if (const auto& c = color())
	{
		midiClipElement.setAttribute("color", c->name());
	}
	// as the target of copied/dragged MIDI clip is always an existing
	// MIDI clip, we must not store actual position, instead we store -1
	// which tells loadSettings() not to mess around with position
	if (midiClipElement.parentNode().nodeName() == "clipboard" ||
			midiClipElement.parentNode().nodeName() == "dnddata")
	{
		midiClipElement.setAttribute("pos", -1);
	}
	else
	{
		midiClipElement.setAttribute("pos", startPosition());
	}
	midiClipElement.setAttribute("muted", isMuted());
	midiClipElement.setAttribute("steps", m_steps);
	midiClipElement.setAttribute("len", length());

	// now save settings of all notes
	for (auto& note : m_notes)
	{
		if (!onlySelectedNotes || note->selected())
		{
			note->saveState(doc, midiClipElement);
		}
	}
}


void MidiClip::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	exportToXML(_doc, _this);
}




void MidiClip::loadSettings( const QDomElement & _this )
{
	m_clipType = static_cast<Type>( _this.attribute( "type"
								).toInt() );
	setName( _this.attribute( "name" ) );

	if (_this.hasAttribute("color"))
	{
		setColor(QColor{_this.attribute("color")});
	}

	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	if (static_cast<bool>(_this.attribute("muted").toInt()) != isMuted())
	{
		toggleMute();
	}

	clearNotes();

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			auto n = new Note;
			n->restoreState( node.toElement() );
			m_notes.push_back( n );
		}
		node = node.nextSibling();
        }

	m_steps = _this.attribute( "steps" ).toInt();
	if( m_steps == 0 )
	{
		m_steps = TimePos::stepsPerBar();
	}

	checkType();

	int len = _this.attribute("len").toInt();
	if (len <= 0)
	{
		// TODO: Handle with an upgrade method
		updateLength();
	}
	else
	{
		changeLength(len);
	}
	
	setAutoResize(_this.attribute("autoresize", "1").toInt());
	setStartTimeOffset(_this.attribute("off").toInt());

	emit dataChanged();
}




MidiClip *  MidiClip::previousMidiClip() const
{
	return adjacentMidiClipByOffset(-1);
}




MidiClip *  MidiClip::nextMidiClip() const
{
	return adjacentMidiClipByOffset(1);
}




MidiClip * MidiClip::adjacentMidiClipByOffset(int offset) const
{
	auto& clips = m_instrumentTrack->getClips();
	int clipNum = m_instrumentTrack->getClipNum(this) + offset;
	if (clipNum < 0 || static_cast<size_t>(clipNum) >= clips.size()) { return nullptr; }
	return dynamic_cast<MidiClip*>(clips[clipNum]);
}




void MidiClip::clear()
{
	addJournalCheckPoint();
	clearNotes();
}




void MidiClip::addSteps()
{
	m_steps += TimePos::stepsPerBar();
	updateLength();
	emit dataChanged();
}

void MidiClip::cloneSteps()
{
	int oldLength = m_steps;
	m_steps *= 2; // cloning doubles the track
	for(int i = 0; i < oldLength; ++i )
	{
		Note *toCopy = noteAtStep( i );
		if( toCopy )
		{
			setStep( oldLength + i, true );
			Note *newNote = noteAtStep( oldLength + i );
			newNote->setKey( toCopy->key() );
			newNote->setLength( toCopy->length() );
			newNote->setPanning( toCopy->getPanning() );
			newNote->setVolume( toCopy->getVolume() );
		}
	}
	updateLength();
	emit dataChanged();
}




void MidiClip::removeSteps()
{
	int n = TimePos::stepsPerBar();
	if( n < m_steps )
	{
		for( int i = m_steps - n; i < m_steps; ++i )
		{
			setStep( i, false );
		}
		m_steps -= n;
		updateLength();
		emit dataChanged();
	}
}




gui::ClipView * MidiClip::createView( gui::TrackView * _tv )
{
	return new gui::MidiClipView( this, _tv );
}




void MidiClip::updatePatternTrack()
{
	if (getTrack()->trackContainer() == Engine::patternStore())
	{
		Engine::patternStore()->updatePatternTrack(this);
	}

	if (gui::getGUI() != nullptr
		&& gui::getGUI()->pianoRoll()
		&& gui::getGUI()->pianoRoll()->currentMidiClip() == this)
	{
		gui::getGUI()->pianoRoll()->update();
	}
}




bool MidiClip::empty()
{
	for (const auto& note : m_notes)
	{
		if (note->length() != 0)
		{
			return false;
		}
	}
	return true;
}




void MidiClip::changeTimeSignature()
{
	TimePos last_pos = TimePos::ticksPerBar() - 1;
	for (const auto& note : m_notes)
	{
		if (note->length() < 0 && note->pos() > last_pos)
		{
			last_pos = note->pos() + TimePos::ticksPerBar() / TimePos::stepsPerBar();
		}
	}
	last_pos = last_pos.nextFullBar() * TimePos::ticksPerBar();
	m_steps = std::max<tick_t>(TimePos::stepsPerBar(),
				last_pos.getBar() * TimePos::stepsPerBar());
	updateLength();
}


} // namespace lmms
