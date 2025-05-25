/*
 * Note.cpp - implementation of class note
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


#include <QDomElement>

#include <cmath>

#include "Note.h"
#include "DetuningHelper.h"

namespace lmms
{


Note::Note( const TimePos & length, const TimePos & pos,
		int key, volume_t volume, panning_t panning,
						std::shared_ptr<DetuningHelper> detuning ) :
	m_selected( false ),
	m_oldKey(std::clamp(key, 0, NumKeys)),
	m_oldPos( pos ),
	m_oldLength( length ),
	m_isPlaying( false ),
	m_key(std::clamp(key, 0, NumKeys)),
	m_volume(std::clamp(volume, MinVolume, MaxVolume)),
	m_panning(std::clamp(panning, PanningLeft, PanningRight)),
	m_length( length ),
	m_pos(pos),
	m_detuning(std::move(detuning))
{
	if (!detuning)
	{
		createDetuning();
	}
}




Note::Note( const Note & note ) :
	SerializingObject( note ),
	m_selected( note.m_selected ),
	m_oldKey( note.m_oldKey ),
	m_oldPos( note.m_oldPos ),
	m_oldLength( note.m_oldLength ),
	m_isPlaying( note.m_isPlaying ),
	m_key( note.m_key),
	m_volume( note.m_volume ),
	m_panning( note.m_panning ),
	m_length( note.m_length ),
	m_pos( note.m_pos ),
	m_detuning(note.m_detuning),
	m_type(note.m_type)
{
}

Note& Note::operator=(const Note& note)
{
	m_selected = note.m_selected;
	m_oldKey = note.m_oldKey;
	m_oldPos = note.m_oldPos;
	m_oldLength = note.m_oldLength;
	m_isPlaying = note.m_isPlaying;
	m_key = note.m_key;
	m_volume = note.m_volume;
	m_panning = note.m_panning;
	m_length = note.m_length;
	m_pos = note.m_pos;
	m_type = note.m_type;
	m_detuning = note.m_detuning;

	return *this;
}


Note* Note::clone() const
{
	Note* newNote = new Note(*this);
	newNote->m_detuning = std::make_shared<DetuningHelper>(*newNote->m_detuning);
	return newNote;
}



Note::~Note()
{
}




void Note::setLength( const TimePos & length )
{
	m_length = length;
}




void Note::setPos( const TimePos & pos )
{
	m_pos = pos;
}




void Note::setKey( const int key )
{
	const int k = std::clamp(key, 0, NumKeys - 1);
	m_key = k;
}




void Note::setVolume( volume_t volume )
{
	const volume_t v = std::clamp(volume, MinVolume, MaxVolume);
	m_volume = v;
}




void Note::setPanning( panning_t panning )
{
	const panning_t p = std::clamp(panning, PanningLeft, PanningRight);
	m_panning = p;
}




TimePos Note::quantized( const TimePos & m, const int qGrid )
{
	float p = ( (float) m / qGrid );
	if( p - floorf( p ) < 0.5f )
	{
		return static_cast<int>( p ) * qGrid;
	}
	return static_cast<int>( p + 1 ) * qGrid;
}




void Note::quantizeLength( const int qGrid )
{
	setLength( quantized( length(), qGrid ) );
	if( length() == 0 )
	{
		setLength( qGrid );
	}
}




void Note::quantizePos( const int qGrid )
{
	setPos( quantized( pos(), qGrid ) );
}




void Note::saveSettings( QDomDocument & doc, QDomElement & parent )
{
	parent.setAttribute( "key", m_key );
	parent.setAttribute( "vol", m_volume );
	parent.setAttribute( "pan", m_panning );
	parent.setAttribute( "len", m_length );
	parent.setAttribute( "pos", m_pos );
	parent.setAttribute("type", static_cast<int>(m_type));

	if( m_detuning && m_length )
	{
		m_detuning->saveSettings( doc, parent );
	}
}




void Note::loadSettings( const QDomElement & _this )
{
	const int oldKey = _this.attribute( "tone" ).toInt() + _this.attribute( "oct" ).toInt() * KeysPerOctave;
	m_key = std::max(oldKey, _this.attribute("key").toInt());
	m_volume = _this.attribute( "vol" ).toInt();
	m_panning = _this.attribute( "pan" ).toInt();
	m_length = _this.attribute( "len" ).toInt();
	m_pos = _this.attribute( "pos" ).toInt();
	// Default m_type value is 0, which corresponds to RegularNote
	static_assert(0 == static_cast<int>(Type::Regular));
	m_type = static_cast<Type>(_this.attribute("type", "0").toInt());

	if( _this.hasChildNodes() )
	{
		createDetuning();
		m_detuning->loadSettings( _this );
	}
}





void Note::createDetuning()
{
	if( m_detuning == nullptr )
	{
		m_detuning = std::make_shared<DetuningHelper>();
		(void) m_detuning->automationClip();
		m_detuning->setRange( -MaxDetuning, MaxDetuning, 0.5f );
		m_detuning->automationClip()->setProgressionType( AutomationClip::ProgressionType::Linear );
	}
}




bool Note::hasDetuningInfo() const
{
	return m_detuning && m_detuning->hasAutomation();
}



bool Note::withinRange(int tickStart, int tickEnd) const
{
	return pos().getTicks() >= tickStart && pos().getTicks() <= tickEnd
		&& length().getTicks() != 0;
}




/*! \brief Get the start/end/bottom/top positions of notes in a vector
 *
 *  Returns no value if there are no notes
 */
std::optional<NoteBounds> boundsForNotes(const NoteVector& notes)
{
	if (notes.empty()) { return std::nullopt; }

	TimePos start = notes.front()->pos();
	TimePos end = start;
	int lower = notes.front()->key();
	int upper = lower;

	for (const Note* note: notes)
	{
		// TODO should we assume that NoteVector is always sorted correctly,
		// so first() always has the lowest time position?
		start = std::min(start, note->pos());
		end = std::max(end, note->endPos());
		lower = std::min(lower, note->key());
		upper = std::max(upper, note->key());
	}

	return NoteBounds{start, end, lower, upper};
}


} // namespace lmms
