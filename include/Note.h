/*
 * Note.h - declaration of class note which contains all informations about a
 *          note + definitions of several constants and enums
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

#ifndef LMMS_NOTE_H
#define LMMS_NOTE_H

#include <memory>
#include <optional>
#include <vector>

#include "volume.h"
#include "panning.h"
#include "SerializingObject.h"
#include "TimePos.h"


namespace lmms
{


class DetuningHelper;


enum class Key : int
{
	C = 0,
	Cis = 1, Des = 1,
	D = 2,
	Dis = 3, Es = 3,
	E = 4, Fes = 4,
	F = 5,
	Fis = 6, Ges = 6,
	G = 7,
	Gis = 8, As = 8,
	A = 9,
	Ais = 10, B = 10,
	H = 11
} ;


enum class Octave : int
{
	Octave_m1,	// MIDI standard starts at C-1
	Octave_0,
	Octave_1,
	Octave_2,
	Octave_3,
	Octave_4,
	Octave_5,
	Octave_6,
	Octave_7,
	Octave_8,
	Octave_9,	// incomplete octave, MIDI only goes up to G9
};

const int FirstOctave = -1;
const int KeysPerOctave = 12;

constexpr inline auto operator+(Octave octave, Key key) -> int
{
	return static_cast<int>(octave) * KeysPerOctave + static_cast<int>(key);
}

constexpr auto DefaultOctave = Octave::Octave_4;
const int DefaultKey = DefaultOctave + Key::A;
//! Number of physical keys, limited to MIDI range (valid for both MIDI 1.0 and 2.0)
const int NumKeys = 128;

const int DefaultMiddleKey = Octave::Octave_4 + Key::C;
const int DefaultBaseKey = Octave::Octave_4 + Key::A;
const float DefaultBaseFreq = 440.f;

const float MaxDetuning = 5 * 12.0f;



class LMMS_EXPORT Note : public SerializingObject
{
public:
	Note( const TimePos & length = TimePos( 0 ),
		const TimePos & pos = TimePos( 0 ),
		int key = DefaultKey,
		volume_t volume = DefaultVolume,
		panning_t panning = DefaultPanning,
		std::shared_ptr<DetuningHelper> detuning = nullptr);
	Note( const Note & note );
	~Note() override;

	Note& operator=(const Note& note);

	//! Performs a deep copy and returns an owning raw pointer
	Note* clone() const;

	// Note types
	enum class Type
	{
		Regular = 0,
		Step
	};

	Type type() const { return m_type; }
	inline void setType(Type t) { m_type = t; }

	// used by GUI
	inline void setSelected( const bool selected ) { m_selected = selected; }
	inline void setOldKey( const int oldKey ) { m_oldKey = oldKey; }
	inline void setOldPos( const TimePos & oldPos ) { m_oldPos = oldPos; }

	inline void setOldLength( const TimePos & oldLength )
	{
		m_oldLength = oldLength;
	}
	inline void setIsPlaying( const bool isPlaying )
	{
		m_isPlaying = isPlaying;
	}


	void setLength( const TimePos & length );
	void setPos( const TimePos & pos );
	void setKey( const int key );
	virtual void setVolume( volume_t volume );
	virtual void setPanning( panning_t panning );
	void quantizeLength( const int qGrid );
	void quantizePos( const int qGrid );

	static inline bool lessThan( const Note * lhs, const Note * rhs )
	{
		// function to compare two notes - must be called explictly when
		// using qSort
		if( (int)( *lhs ).pos() < (int)( *rhs ).pos() )
		{
			return true;
		}
		else if( (int)( *lhs ).pos() > (int)( *rhs ).pos() )
		{
			return false;
		}
		return ( (int)( *lhs ).key() > (int)( *rhs ).key() );
	}

	inline bool selected() const
	{
		return m_selected;
	}

	inline int oldKey() const
	{
		return m_oldKey;
	}

	inline TimePos oldPos() const
	{
		return m_oldPos;
	}

	inline TimePos oldLength() const
	{
		return m_oldLength;
	}

	inline bool isPlaying() const
	{
		return m_isPlaying;
	}

	inline TimePos endPos() const
	{
		const int l = length();
		return pos() + l;
	}

	inline const TimePos & length() const
	{
		return m_length;
	}

	inline const TimePos & pos() const
	{
		return m_pos;
	}

	inline TimePos pos( TimePos basePos ) const
	{
		const int bp = basePos;
		return m_pos - bp;
	}

	inline int key() const
	{
		return m_key;
	}

	inline volume_t getVolume() const
	{
		return m_volume;
	}

	int midiVelocity( int midiBaseVelocity ) const
	{
		return std::min(MidiMaxVelocity, getVolume() * midiBaseVelocity / DefaultVolume);
	}

	inline panning_t getPanning() const
	{
		return m_panning;
	}

	static QString classNodeName()
	{
		return "note";
	}

	inline QString nodeName() const override
	{
		return classNodeName();
	}

	static TimePos quantized( const TimePos & m, const int qGrid );

	const std::shared_ptr<DetuningHelper>& detuning() const { return m_detuning; }

	bool hasDetuningInfo() const;
	bool withinRange(int tickStart, int tickEnd) const;

	void createDetuning();


protected:
	void saveSettings( QDomDocument & doc, QDomElement & parent ) override;
	void loadSettings( const QDomElement & _this ) override;


private:
	// for piano roll editing
	bool m_selected;
	int m_oldKey;
	TimePos m_oldPos;
	TimePos m_oldLength;
	bool m_isPlaying;

	int m_key;
	volume_t m_volume;
	panning_t m_panning;
	TimePos m_length;
	TimePos m_pos;
	std::shared_ptr<DetuningHelper> m_detuning;

	Type m_type = Type::Regular;
};

using NoteVector = std::vector<Note*>;

struct NoteBounds
{
	TimePos start;
	TimePos end;
	int lowest;
	int highest;
};


std::optional<NoteBounds> boundsForNotes(const NoteVector& notes);


} // namespace lmms

#endif // LMMS_NOTE_H
