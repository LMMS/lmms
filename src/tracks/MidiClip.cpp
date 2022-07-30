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

#include <QDomElement>

#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "MidiClipView.h"
#include "PatternStore.h"
#include "PianoRoll.h"



QPixmap * MidiClipView::s_stepBtnOn0 = nullptr;
QPixmap * MidiClipView::s_stepBtnOn200 = nullptr;
QPixmap * MidiClipView::s_stepBtnOff = nullptr;
QPixmap * MidiClipView::s_stepBtnOffLight = nullptr;



MidiClip::MidiClip( InstrumentTrack * _instrument_track ) :
	Clip( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_clipType( BeatClip ),
	m_steps( TimePos::stepsPerBar() )
{
	if (_instrument_track->trackContainer()	== Engine::patternStore())
	{
		resizeToFirstTrack();
	}
	init();
	setAutoResize( true );
}




MidiClip::MidiClip( const MidiClip& other ) :
	Clip( other.m_instrumentTrack ),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_clipType( other.m_clipType ),
	m_steps( other.m_steps )
{
	for( NoteVector::ConstIterator it = other.m_notes.begin(); it != other.m_notes.end(); ++it )
	{
		m_notes.push_back( new Note( **it ) );
	}

	init();
	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::PatternContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
}


MidiClip::~MidiClip()
{
	emit destroyedMidiClip( this );

	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		delete *it;
	}

	m_notes.clear();
}




void MidiClip::resizeToFirstTrack()
{
	// Resize this track to be the same as existing tracks in the pattern
	const TrackContainer::TrackList & tracks =
		m_instrumentTrack->trackContainer()->tracks();
	for(unsigned int trackID = 0; trackID < tracks.size(); ++trackID)
	{
		if(tracks.at(trackID)->type() == Track::InstrumentTrack)
		{
			if(tracks.at(trackID) != m_instrumentTrack)
			{
				unsigned int currentClip = m_instrumentTrack->
					getClips().indexOf(this);
				m_steps = static_cast<MidiClip *>
					(tracks.at(trackID)->getClip(currentClip))
					->m_steps;
			}
			break;
		}
	}
}




void MidiClip::init()
{
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
				this, SLOT( changeTimeSignature() ) );
	saveJournallingState( false );

	updateLength();
	restoreJournallingState();
}




void MidiClip::updateLength()
{
	if( m_clipType == BeatClip )
	{
		changeLength( beatClipLength() );
		updatePatternTrack();
		return;
	}

	tick_t max_length = TimePos::ticksPerBar();

	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() > 0 )
		{
			max_length = qMax<tick_t>( max_length,
							( *it )->endPos() );
		}
	}
	changeLength( TimePos( max_length ).nextFullBar() *
						TimePos::ticksPerBar() );
	updatePatternTrack();
}




TimePos MidiClip::beatClipLength() const
{
	tick_t max_length = TimePos::ticksPerBar();

	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() < 0 )
		{
			max_length = qMax<tick_t>( max_length,
				( *it )->pos() + 1 );
		}
	}

	if( m_steps != TimePos::stepsPerBar() )
	{
		max_length = m_steps * TimePos::ticksPerBar() /
						TimePos::stepsPerBar();
	}

	return TimePos( max_length ).nextFullBar() * TimePos::ticksPerBar();
}




Note * MidiClip::addNote( const Note & _new_note, const bool _quant_pos )
{
	Note * new_note = new Note( _new_note );
	if( _quant_pos && getGUI()->pianoRoll() )
	{
		new_note->quantizePos( getGUI()->pianoRoll()->quantization() );
	}

	instrumentTrack()->lock();
	m_notes.insert(std::upper_bound(m_notes.begin(), m_notes.end(), new_note, Note::lessThan), new_note);
	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();

	return new_note;
}




void MidiClip::removeNote( Note * _note_to_del )
{
	instrumentTrack()->lock();
	NoteVector::Iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( *it == _note_to_del )
		{
			delete *it;
			m_notes.erase( it );
			break;
		}
		++it;
	}
	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();
}


// returns a pointer to the note at specified step, or NULL if note doesn't exist

Note * MidiClip::noteAtStep( int _step )
{
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		if( ( *it )->pos() == TimePos::stepPosition( _step )
						&& ( *it )->length() < 0 )
		{
			return *it;
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
	for( NoteVector::Iterator it = m_notes.begin(); it != m_notes.end();
									++it )
	{
		delete *it;
	}
	m_notes.clear();
	instrumentTrack()->unlock();

	checkType();
	emit dataChanged();
}




Note * MidiClip::addStepNote( int step )
{
	return addNote( Note( TimePos( -DefaultTicksPerBar ),
				TimePos::stepPosition( step ) ), false );
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




void MidiClip::splitNotes(NoteVector notes, TimePos pos)
{
	if (notes.empty()) { return; }

	addJournalCheckPoint();

	for (int i = 0; i < notes.size(); ++i)
	{
		Note* note = notes.at(i);

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




void MidiClip::setType( MidiClipTypes _new_clip_type )
{
	if( _new_clip_type == BeatClip ||
				_new_clip_type == MelodyClip )
	{
		m_clipType = _new_clip_type;
	}
}




void MidiClip::checkType()
{
	NoteVector::Iterator it = m_notes.begin();
	while( it != m_notes.end() )
	{
		if( ( *it )->length() > 0 )
		{
			setType( MelodyClip );
			return;
		}
		++it;
	}
	setType( BeatClip );
}




void MidiClip::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "type", m_clipType );
	_this.setAttribute( "name", name() );
	
	if( usesCustomClipColor() )
	{
		_this.setAttribute( "color", color().name() );
	}
	// as the target of copied/dragged MIDI clip is always an existing
	// MIDI clip, we must not store actual position, instead we store -1
	// which tells loadSettings() not to mess around with position
	if( _this.parentNode().nodeName() == "clipboard" ||
			_this.parentNode().nodeName() == "dnddata" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "steps", m_steps );

	// now save settings of all notes
	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		( *it )->saveState( _doc, _this );
	}
}




void MidiClip::loadSettings( const QDomElement & _this )
{
	m_clipType = static_cast<MidiClipTypes>( _this.attribute( "type"
								).toInt() );
	setName( _this.attribute( "name" ) );
	
	if( _this.hasAttribute( "color" ) )
	{
		useCustomClipColor( true );
		setColor( _this.attribute( "color" ) );
	}
	else
	{
		useCustomClipColor(false);
	}
	
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	if( _this.attribute( "muted" ).toInt() != isMuted() )
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
			Note * n = new Note;
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
	updateLength();

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
	QVector<Clip *> clips = m_instrumentTrack->getClips();
	int clipNum = m_instrumentTrack->getClipNum(this);
	return dynamic_cast<MidiClip*>(clips.value(clipNum + offset, nullptr));
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




ClipView * MidiClip::createView( TrackView * _tv )
{
	return new MidiClipView( this, _tv );
}




void MidiClip::updatePatternTrack()
{
	if (getTrack()->trackContainer() == Engine::patternStore())
	{
		Engine::patternStore()->updatePatternTrack(this);
	}

	if( getGUI() != nullptr
		&& getGUI()->pianoRoll()
		&& getGUI()->pianoRoll()->currentMidiClip() == this )
	{
		getGUI()->pianoRoll()->update();
	}
}




bool MidiClip::empty()
{
	for( NoteVector::ConstIterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		if( ( *it )->length() != 0 )
		{
			return false;
		}
	}
	return true;
}




void MidiClip::changeTimeSignature()
{
	TimePos last_pos = TimePos::ticksPerBar() - 1;
	for( NoteVector::ConstIterator cit = m_notes.begin();
						cit != m_notes.end(); ++cit )
	{
		if( ( *cit )->length() < 0 && ( *cit )->pos() > last_pos )
		{
			last_pos = ( *cit )->pos()+TimePos::ticksPerBar() /
						TimePos::stepsPerBar();
		}
	}
	last_pos = last_pos.nextFullBar() * TimePos::ticksPerBar();
	m_steps = qMax<tick_t>( TimePos::stepsPerBar(),
				last_pos.getBar() * TimePos::stepsPerBar() );
	updateLength();
}
