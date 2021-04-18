/*
 * Pattern.cpp - implementation of class pattern which holds notes
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

#include "Pattern.h"

#include "BBTrackContainer.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "PianoRoll.h"

#include <limits>


QPixmap * PatternView::s_stepBtnOn0 = NULL;
QPixmap * PatternView::s_stepBtnOn200 = NULL;
QPixmap * PatternView::s_stepBtnOff = NULL;
QPixmap * PatternView::s_stepBtnOffLight = NULL;



Pattern::Pattern( InstrumentTrack * _instrument_track ) :
	TrackContentObject( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_patternType( BeatPattern ),
	m_steps( TimePos::stepsPerBar() )
{
	if( _instrument_track->trackContainer()
					== Engine::getBBTrackContainer() )
	{
		resizeToFirstTrack();
	}
	init();
	setAutoResize( true );
}




Pattern::Pattern( const Pattern& other ) :
	TrackContentObject( other.m_instrumentTrack ),
	m_instrumentTrack( other.m_instrumentTrack ),
	m_patternType( other.m_patternType ),
	m_steps( other.m_steps )
{
	for( NoteVector::ConstIterator it = other.m_notes.begin(); it != other.m_notes.end(); ++it )
	{
		m_notes.push_back( new Note( **it ) );
	}

	init();
	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::BBContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
}


Pattern::~Pattern()
{
	emit destroyedPattern( this );

	for( NoteVector::Iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		delete *it;
	}

	m_notes.clear();
}




void Pattern::resizeToFirstTrack()
{
	// Resize this track to be the same as existing tracks in the BB
	const TrackContainer::TrackList & tracks =
		m_instrumentTrack->trackContainer()->tracks();
	for(unsigned int trackID = 0; trackID < tracks.size(); ++trackID)
	{
		if(tracks.at(trackID)->type() == Track::InstrumentTrack)
		{
			if(tracks.at(trackID) != m_instrumentTrack)
			{
				unsigned int currentTCO = m_instrumentTrack->
					getTCOs().indexOf(this);
				m_steps = static_cast<Pattern *>
					(tracks.at(trackID)->getTCO(currentTCO))
					->m_steps;
			}
			break;
		}
	}
}




void Pattern::init()
{
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
				this, SLOT( changeTimeSignature() ) );
	saveJournallingState( false );

	updateLength();
	restoreJournallingState();
}




void Pattern::updateLength()
{
	if( m_patternType == BeatPattern )
	{
		changeLength( beatPatternLength() );
		updateBBTrack();
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
	updateBBTrack();
}




TimePos Pattern::beatPatternLength() const
{
	tick_t max_length = TimePos::ticksPerBar();

	for (NoteVector::ConstIterator it = m_notes.begin(); it != m_notes.end(); ++it)
	{
		if ((*it)->type() == Note::StepNote)
		{
			max_length = qMax<tick_t>(max_length, (*it)->pos() + 1);
		}
	}

	if (m_steps != TimePos::stepsPerBar())
	{
		max_length = m_steps * TimePos::ticksPerBar() / TimePos::stepsPerBar();
	}

	return TimePos(max_length).nextFullBar() * TimePos::ticksPerBar();
}




Note * Pattern::addNote( const Note & _new_note, const bool _quant_pos )
{
	Note * new_note = new Note( _new_note );
	if( _quant_pos && gui->pianoRoll() )
	{
		new_note->quantizePos( gui->pianoRoll()->quantization() );
	}

	instrumentTrack()->lock();
	m_notes.insert(std::upper_bound(m_notes.begin(), m_notes.end(), new_note, Note::lessThan), new_note);
	instrumentTrack()->unlock();

	checkType();
	updateLength();

	emit dataChanged();

	return new_note;
}




void Pattern::removeNote( Note * _note_to_del )
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


// Returns a pointer to the note at specified step, or nullptr if note doesn't exist
Note * Pattern::noteAtStep(int step)
{
	for (NoteVector::Iterator it = m_notes.begin(); it != m_notes.end(); ++it)
	{
		if ((*it)->pos() == TimePos::stepPosition(step)
			&& (*it)->type() == Note::StepNote)
		{
			return *it;
		}
	}
	return nullptr;
}



void Pattern::rearrangeAllNotes()
{
	// sort notes by start time
	std::sort(m_notes.begin(), m_notes.end(), Note::lessThan);
}



void Pattern::clearNotes()
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




Note * Pattern::addStepNote(int step)
{
	auto prevPatternType = m_patternType;
	Note * n =
		addNote(
			Note(TimePos(DefaultTicksPerBar / 16),
			TimePos::stepPosition(step)), false
		);
	n->setType(Note::StepNote);
	// addNote will change a BeatPattern to a MelodyPattern
	// because it calls checkType after adding a regular note.
	// We need to revert it back to a BeatPattern if it was one
	// before we added the note.
	if (prevPatternType == BeatPattern) { setType(BeatPattern); }
	return n;
}




void Pattern::setStep( int step, bool enabled )
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




void Pattern::splitNotes(NoteVector notes, TimePos pos)
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




void Pattern::setType( PatternTypes _new_pattern_type )
{
	if( _new_pattern_type == BeatPattern ||
				_new_pattern_type == MelodyPattern )
	{
		m_patternType = _new_pattern_type;
	}
}




void Pattern::checkType()
{
	NoteVector::Iterator it = m_notes.begin();

	// If all notes are StepNotes, we have a BeatPattern
	bool beatPattern = true;
	while (it != m_notes.end())
	{
		if ((*it)->type() != Note::StepNote)
		{
			beatPattern = false;
			break;
		}
		++it;
	}

	setType(beatPattern ? BeatPattern : MelodyPattern);
}




void Pattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "type", m_patternType );
	_this.setAttribute( "name", name() );
	
	if( usesCustomClipColor() )
	{
		_this.setAttribute( "color", color().name() );
	}
	// as the target of copied/dragged pattern is always an existing
	// pattern, we must not store actual position, instead we store -1
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




void Pattern::loadSettings( const QDomElement & _this )
{
	m_patternType = static_cast<PatternTypes>( _this.attribute( "type"
								).toInt() );
	setName( _this.attribute( "name" ) );
	
	if( _this.hasAttribute( "color" ) )
	{
		useCustomClipColor( true );
		setColor( _this.attribute( "color" ) );
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




Pattern *  Pattern::previousPattern() const
{
	return adjacentPatternByOffset(-1);
}




Pattern *  Pattern::nextPattern() const
{
	return adjacentPatternByOffset(1);
}




Pattern * Pattern::adjacentPatternByOffset(int offset) const
{
	QVector<TrackContentObject *> tcos = m_instrumentTrack->getTCOs();
	int tcoNum = m_instrumentTrack->getTCONum(this);
	return dynamic_cast<Pattern*>(tcos.value(tcoNum + offset, NULL));
}




void Pattern::clear()
{
	addJournalCheckPoint();
	clearNotes();
}




void Pattern::addSteps()
{
	m_steps += TimePos::stepsPerBar();
	updateLength();
	emit dataChanged();
}

void Pattern::cloneSteps()
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




void Pattern::removeSteps()
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




TrackContentObjectView * Pattern::createView( TrackView * _tv )
{
	return new PatternView( this, _tv );
}




void Pattern::updateBBTrack()
{
	if( getTrack()->trackContainer() == Engine::getBBTrackContainer() )
	{
		Engine::getBBTrackContainer()->updateBBTrack( this );
	}

	if( gui && gui->pianoRoll() && gui->pianoRoll()->currentPattern() == this )
	{
		gui->pianoRoll()->update();
	}
}




bool Pattern::empty()
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




void Pattern::changeTimeSignature()
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
