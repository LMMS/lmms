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
#include <set>
#include <QDomElement>
#include <QHash>
#include <QUuid>

#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "MidiClipView.h"
#include "PatternStore.h"
#include "PianoRoll.h"



namespace lmms
{

//! The notes of a clip, shared by all clips linked to it
struct MidiClip::LinkGroup
{
	LinkGroup() :
		// Each one needs an ID
		id(QUuid::createUuid().toString(QUuid::WithoutBraces))
	{
		registry()[id] = this;
	}

	~LinkGroup()
	{
		if (registry().value(id) == this) { registry().remove(id); }
		for (const auto& note : notes)
		{
			delete note;
		}
	}

	void setId(const QString& newId)
	{
		if (registry().value(id) == this) { registry().remove(id); }
		id = newId;
		registry()[id] = this;
	}

	static LinkGroupPtr create()
	{
		auto group = std::make_shared<LinkGroup>();
		group->self = group;
		return group;
	}

	//! All link groups that currently exist, by ID
	static QHash<QString, LinkGroup*>& registry()
	{
		static QHash<QString, LinkGroup*> groups;
		return groups;
	}

	//! Gives linked groups the first free label and takes it back from groups that are no longer linked
	void updateLabel()
	{
		if (clips.size() < 2) { labelIndex = -1; }
		else if (labelIndex < 0)
		{
			std::set<int> used;
			for (const auto& group : registry())
			{
				if (group != this && group->labelIndex >= 0) { used.insert(group->labelIndex); }
			}
			labelIndex = 0;
			while (used.count(labelIndex)) { ++labelIndex; }
		}
	}

	QString id;
	NoteVector notes;
	std::vector<MidiClip*> clips;
	std::weak_ptr<LinkGroup> self;
	//! Index of the label shown on linked clips, or -1 when not linked
	int labelIndex = -1;
	bool updating = false;
};




MidiClip::MidiClip( InstrumentTrack * _instrument_track ) :
	Clip( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_clipType( Type::BeatClip ),
	m_links( LinkGroup::create() ),
	m_steps( TimePos::stepsPerBar() )
{
	m_links->clips.push_back(this);
	if (_instrument_track->trackContainer()	== Engine::patternStore())
	{
		resizeToFirstTrack();
	}
	init();
}




MidiClip::MidiClip( const MidiClip& other ) :
	Clip(other),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_clipType( other.m_clipType ),
	m_links( LinkGroup::create() ),
	m_steps( other.m_steps )
{
	m_links->clips.push_back(this);
	for (const auto& note : other.notes())
	{
		m_links->notes.push_back(note->clone());
	}

	init();
}


MidiClip::~MidiClip()
{
	emit destroyedMidiClip( this );

	// The notes are deleted together with the last clip using them
	auto& clips = m_links->clips;
	clips.erase(std::remove(clips.begin(), clips.end(), this), clips.end());

	// A clip left on its own is no longer linked, so it stops showing the label
	const bool wasLinked = m_links->labelIndex >= 0;
	m_links->updateLabel();
	if (wasLinked && clips.size() == 1)
	{
		m_links->updating = true;
		emit clips.front()->dataChanged();
		m_links->updating = false;
	}
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
	connect(Engine::getSong(), &Song::timeSignatureChanged, this, &MidiClip::changeTimeSignature);
	// Notes are often edited directly (e.g. by the piano roll), followed by dataChanged()
	connect(this, &MidiClip::dataChanged, this, &MidiClip::updateLinkedClips);
	if (getTrack()->trackContainer() != Engine::patternStore())
	{
		saveJournallingState(false);
		updateLength();
		restoreJournallingState();
	}
}




void MidiClip::updateLength()
{
	// If the clip hasn't already been manually resized, automatically resize it.
	if (getAutoResize())
	{
		if (m_instrumentTrack->trackContainer()->type() == TrackContainer::Type::Pattern)
		{
			changeLength(beatClipLength());
			updatePatternTrack();
			return;
		}
		tick_t max_length = TimePos::ticksPerBar();

		for (const auto& note : notes())
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

	for (const auto& note : notes())
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

	lockLinkedTracks();
	m_links->notes.insert(std::upper_bound(m_links->notes.begin(), m_links->notes.end(), new_note, Note::lessThan), new_note);
	unlockLinkedTracks();

	checkType();
	updateLength();

	emit dataChanged();

	return new_note;
}




NoteVector::const_iterator MidiClip::removeNote(NoteVector::const_iterator it)
{
	lockLinkedTracks();
	delete *it;
	auto new_it = m_links->notes.erase(it);
	unlockLinkedTracks();

	checkType();
	updateLength();

	emit dataChanged();
	return new_it;
}

NoteVector::const_iterator MidiClip::removeNote(Note* note)
{
	lockLinkedTracks();

	auto& notes = m_links->notes;
	auto it = std::find(notes.begin(), notes.end(), note);
	if (it != notes.end())
	{
		delete *it;
		it = notes.erase(it);
	}

	unlockLinkedTracks();

	checkType();
	updateLength();

	emit dataChanged();
	return it;
}


// Returns a pointer to the note at specified step, or nullptr if note doesn't exist
Note * MidiClip::noteAtStep(int step)
{
	for (const auto& note : notes())
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
	lockLinkedTracks();
	std::sort(m_links->notes.begin(), m_links->notes.end(), Note::lessThan);
	unlockLinkedTracks();
}



void MidiClip::clearNotes()
{
	lockLinkedTracks();
	for (const auto& note : m_links->notes)
	{
		delete note;
	}
	m_links->notes.clear();
	unlockLinkedTracks();

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
	const auto beatClip = std::all_of(notes().begin(), notes().end(), [](auto note) { return note->type() == Note::Type::Step; });

	setType(beatClip ? Type::BeatClip : Type::MelodyClip);
}


void MidiClip::exportToXML(QDomDocument& doc, QDomElement& midiClipElement, bool onlySelectedNotes)
{
	midiClipElement.setAttribute("type", static_cast<int>(m_clipType));
	midiClipElement.setAttribute("name", name());
	midiClipElement.setAttribute("autoresize", QString::number(getAutoResize()));
	midiClipElement.setAttribute("off", startTimeOffset());
	saveLoopSettings(midiClipElement);
	if (isLinked())
	{
		midiClipElement.setAttribute("link", linkId());
	}
	
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
	for (auto& note : notes())
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

	if (loadLinkSettings(_this)) { loadNotes(_this); }

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
	loadLoopSettings(_this);

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
	for (const auto& note : notes())
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
	for (const auto& note : notes())
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




const NoteVector& MidiClip::notes() const
{
	return m_links->notes;
}




bool MidiClip::isLinked() const
{
	return m_links->clips.size() > 1;
}




const std::vector<MidiClip*>& MidiClip::linkedClips() const
{
	return m_links->clips;
}




const QString& MidiClip::linkId() const
{
	return m_links->id;
}




QString MidiClip::linkLabel() const
{
	return isLinked() ? linkLabel(m_links->labelIndex) : QString{};
}




QString MidiClip::linkLabel(int index)
{
	if (index < 0) { return {}; }

	// A to Z, then A1 to Z1, A2 to Z2, ...
	const auto letter = QChar('A' + index % 26);
	const int round = index / 26;
	return round == 0 ? QString{letter} : letter + QString::number(round);
}




MidiClip* MidiClip::linkedClone()
{
	auto clip = new MidiClip(*this);
	clip->setLinkGroup(m_links);
	return clip;
}




void MidiClip::linkTo(MidiClip* other)
{
	if (other == this || other->m_links == m_links) { return; }
	setLinkGroup(other->m_links);
	checkType();
	updateLength();
	emit dataChanged();
}




void MidiClip::unlink()
{
	if (!isLinked()) { return; }

	auto group = LinkGroup::create();
	for (const auto& note : notes())
	{
		group->notes.push_back(note->clone());
	}
	setLinkGroup(group);
	emit dataChanged();
}




void MidiClip::renewLinkIds(QDomElement& element)
{
	QHash<QString, QString> newIds;
	const QDomNodeList clips = element.elementsByTagName("midiclip");
	for (int i = 0; i < clips.length(); ++i)
	{
		QDomElement clip = clips.item(i).toElement();
		const QString id = clip.attribute("link");
		if (id.isEmpty()) { continue; }

		if (!newIds.contains(id))
		{
			newIds[id] = QUuid::createUuid().toString(QUuid::WithoutBraces);
		}
		clip.setAttribute("link", newIds[id]);
	}
}




void MidiClip::setLinkGroup(LinkGroupPtr group)
{
	if (group == m_links) { return; }

	// The audio thread may be playing this clip's notes
	instrumentTrack()->lock();
	auto& clips = m_links->clips;
	clips.erase(std::remove(clips.begin(), clips.end(), this), clips.end());
	// Keep the old notes alive until the track is unlocked since this may have been their last clip
	const auto oldGroup = std::exchange(m_links, std::move(group));
	m_links->clips.push_back(this);
	instrumentTrack()->unlock();

	// Free the old group's label first so that it can be reused right away
	oldGroup->updateLabel();
	m_links->updateLabel();

	// Let the views of both groups show whether their clip is still linked
	for (const auto& affected : {oldGroup.get(), m_links.get()})
	{
		affected->updating = true;
		for (const auto& clip : affected->clips) { emit clip->dataChanged(); }
		affected->updating = false;
	}
}




void MidiClip::setLinkId(const QString& id)
{
	if (const auto other = LinkGroup::registry().value(id); other && other != m_links.get())
	{
		// IDs are unique, so this only happens when joining the group was intended
		setLinkGroup(other->self.lock());
		return;
	}
	m_links->setId(id);
}



// "Reduced Complexity"
bool MidiClip::loadLinkSettings(const QDomElement& element)
{
	// Linked clips share their notes, so a clip joining existing linked clips keeps their notes
	const QString link = supportsLinking() ? element.attribute("link") : QString{};
	if (link == linkId()) { return true; }

	if (const auto group = link.isEmpty() ? nullptr : LinkGroup::registry().value(link))
	{
		setLinkGroup(group->self.lock());
		return false;
	}

	if (isLinked()) { unlink(); }
	if (!link.isEmpty()) { setLinkId(link); }
	return true;
}




void MidiClip::loadNotes(const QDomElement& element)
{
	clearNotes();

	lockLinkedTracks();
	for (QDomNode node = element.firstChild(); !node.isNull(); node = node.nextSibling())
	{
		if (node.isElement() && !node.toElement().attribute("metadata").toInt())
		{
			auto n = new Note;
			n->restoreState(node.toElement());
			m_links->notes.push_back(n);
		}
	}
	unlockLinkedTracks();
}




void MidiClip::updateLinkedClips()
{
	if (m_links->updating || !isLinked()) { return; }

	m_links->updating = true;
	const auto clips = m_links->clips;
	for (const auto& clip : clips)
	{
		if (clip == this) { continue; }
		clip->checkType();
		clip->updateLength();
		emit clip->dataChanged();
	}
	m_links->updating = false;
}




void MidiClip::lockLinkedTracks()
{
	// Lock each track once, in a consistent order
	std::set<Track*> tracks;
	for (const auto& clip : m_links->clips) { tracks.insert(clip->getTrack()); }
	for (const auto& track : tracks) { track->lock(); }
}




void MidiClip::unlockLinkedTracks()
{
	std::set<Track*> tracks;
	for (const auto& clip : m_links->clips) { tracks.insert(clip->getTrack()); }
	for (auto it = tracks.rbegin(); it != tracks.rend(); ++it) { (*it)->unlock(); }
}


} // namespace lmms
