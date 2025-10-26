/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
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


#include "InstrumentFunctions.h"
#include "AudioEngine.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "PresetPreviewPlayHandle.h"

#include <vector>
#include <algorithm>

namespace lmms
{

std::array<InstrumentFunctionNoteStacking::ChordTable::Init, InstrumentFunctionNoteStacking::NUM_CHORD_TABLES>
	InstrumentFunctionNoteStacking::ChordTable::s_initTable =
	std::array<InstrumentFunctionNoteStacking::ChordTable::Init, NUM_CHORD_TABLES>
{{
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "octave" ), { 0, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Major" ), { 0, 4, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Majb5" ), { 0, 4, 6, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "minor" ), { 0, 3, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "minb5" ), { 0, 3, 6, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "sus2" ), { 0, 2, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "sus4" ), { 0, 5, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "aug" ), { 0, 4, 8, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "augsus4" ), { 0, 5, 8, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "tri" ), { 0, 3, 6, 9, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "6" ), { 0, 4, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "6sus4" ), { 0, 5, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "6add9" ), { 0, 4, 7, 9, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m6" ), { 0, 3, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m6add9" ), { 0, 3, 7, 9, 14, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7" ), { 0, 4, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7sus4" ), { 0, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7#5" ), { 0, 4, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7b5" ), { 0, 4, 6, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7#9" ), { 0, 4, 7, 10, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7b9" ), { 0, 4, 7, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7#5#9" ), { 0, 4, 8, 10, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7#5b9" ), { 0, 4, 8, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7b5b9" ), { 0, 4, 6, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7add11" ), { 0, 4, 7, 10, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7add13" ), { 0, 4, 7, 10, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "7#11" ), { 0, 4, 7, 10, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj7" ), { 0, 4, 7, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj7b5" ), { 0, 4, 6, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj7#5" ), { 0, 4, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj7#11" ), { 0, 4, 7, 11, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj7add13" ), { 0, 4, 7, 11, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m7" ), { 0, 3, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m7b5" ), { 0, 3, 6, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m7b9" ), { 0, 3, 7, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m7add11" ), { 0, 3, 7, 10, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m7add13" ), { 0, 3, 7, 10, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m-Maj7" ), { 0, 3, 7, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m-Maj7add11" ), { 0, 3, 7, 11, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m-Maj7add13" ), { 0, 3, 7, 11, 21, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9" ), { 0, 4, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9sus4" ), { 0, 5, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "add9" ), { 0, 4, 7, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9#5" ), { 0, 4, 8, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9b5" ), { 0, 4, 6, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9#11" ), { 0, 4, 7, 10, 14, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "9b13" ), { 0, 4, 7, 10, 14, 20, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj9" ), { 0, 4, 7, 11, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj9sus4" ), { 0, 5, 7, 11, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj9#5" ), { 0, 4, 8, 11, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj9#11" ), { 0, 4, 7, 11, 14, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m9" ), { 0, 3, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "madd9" ), { 0, 3, 7, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m9b5" ), { 0, 3, 6, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m9-Maj7" ), { 0, 3, 7, 11, 14, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "11" ), { 0, 4, 7, 10, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "11b9" ), { 0, 4, 7, 10, 13, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj11" ), { 0, 4, 7, 11, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m11" ), { 0, 3, 7, 10, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m-Maj11" ), { 0, 3, 7, 11, 14, 17, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "13" ), { 0, 4, 7, 10, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "13#9" ), { 0, 4, 7, 10, 15, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "13b9" ), { 0, 4, 7, 10, 13, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "13b5b9" ), { 0, 4, 6, 10, 13, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Maj13" ), { 0, 4, 7, 11, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m13" ), { 0, 3, 7, 10, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "m-Maj13" ), { 0, 3, 7, 11, 14, 21, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Major" ), { 0, 2, 4, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Harmonic minor" ), { 0, 2, 3, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Melodic minor" ), { 0, 2, 3, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Whole tone" ), { 0, 2, 4, 6, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Diminished" ), { 0, 2, 3, 5, 6, 8, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Major pentatonic" ), { 0, 2, 4, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Minor pentatonic" ), { 0, 3, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Jap in sen" ), { 0, 1, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Major bebop" ), { 0, 2, 4, 5, 7, 8, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Dominant bebop" ), { 0, 2, 4, 5, 7, 9, 10, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Blues" ), { 0, 3, 5, 6, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Arabic" ), { 0, 1, 4, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Enigmatic" ), { 0, 1, 4, 6, 8, 10, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Neopolitan" ), { 0, 1, 3, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Neopolitan minor" ), { 0, 1, 3, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Hungarian minor" ), { 0, 2, 3, 6, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Dorian" ), { 0, 2, 3, 5, 7, 9, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Phrygian" ), { 0, 1, 3, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Lydian" ), { 0, 2, 4, 6, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Mixolydian" ), { 0, 2, 4, 5, 7, 9, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Aeolian" ), { 0, 2, 3, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Locrian" ), { 0, 1, 3, 5, 6, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Minor" ), { 0, 2, 3, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Chromatic" ), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Half-Whole Diminished" ), { 0, 1, 3, 4, 6, 7, 9, 10, -1 } },

	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "5" ), { 0, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Phrygian dominant" ), { 0, 1, 4, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Persian" ), { 0, 1, 4, 5, 6, 8, 11, -1 } }
}};




InstrumentFunctionNoteStacking::Chord::Chord( const char * n, const ChordSemiTones & semi_tones ) :
	m_name( InstrumentFunctionNoteStacking::tr( n ) )
{
	for( m_size = 0; m_size < MAX_CHORD_POLYPHONY; m_size++ )
	{
		if( semi_tones[m_size] == -1 )
		{
			break;
		}

		m_semiTones[m_size] = semi_tones[m_size];
	}
}




bool InstrumentFunctionNoteStacking::Chord::hasSemiTone( int8_t semi_tone ) const
{
	for( int i = 0; i < size(); ++i )
	{
		if( semi_tone == m_semiTones[i] )
		{
			return true;
		}
	}
	return false;
}




InstrumentFunctionNoteStacking::ChordTable::ChordTable()
{
	for (const auto& chord : s_initTable)
	{
		m_chords.emplace_back(chord.m_name, chord.m_semiTones);
	}
}




const InstrumentFunctionNoteStacking::Chord & InstrumentFunctionNoteStacking::ChordTable::getByName( const QString & name, bool is_scale ) const
{
	for (const auto& chord : m_chords)
	{
		if (chord.getName() == name && is_scale == chord.isScale())
		{
			return chord;
		}
	}

	static Chord empty;
	return empty;
}




InstrumentFunctionNoteStacking::InstrumentFunctionNoteStacking( Model * _parent ) :
	Model( _parent, tr( "Chords" ) ),
	m_chordsEnabledModel( false, this ),
	m_chordsModel( this, tr( "Chord type" ) ),
	m_chordRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Chord range" ) )
{
	const ChordTable & chord_table = ChordTable::getInstance();

	for (const auto& chord : chord_table.chords())
	{
		m_chordsModel.addItem(chord.getName());
	}
}







void InstrumentFunctionNoteStacking::processNote( NotePlayHandle * _n )
{
	const int base_note_key = _n->key();
	const ChordTable & chord_table = ChordTable::getInstance();
	// we add chord-subnotes to note if either note is a base-note and
	// arpeggio is not used or note is part of an arpeggio
	// at the same time we only add sub-notes if nothing of the note was
	// played yet, because otherwise we would add chord-subnotes every
	// time an audio-buffer is rendered...
	if( ( _n->origin() == NotePlayHandle::Origin::Arpeggio || ( _n->hasParent() == false && _n->instrumentTrack()->isArpeggioEnabled() == false ) ) &&
			_n->totalFramesPlayed() == 0 &&
			m_chordsEnabledModel.value() == true && ! _n->isReleased() )
	{
		// then insert sub-notes for chord
		const int selected_chord = m_chordsModel.value();

		for( int octave_cnt = 0; octave_cnt < m_chordRangeModel.value(); ++octave_cnt )
		{
			const int sub_note_key_base = base_note_key + octave_cnt * KeysPerOctave;

			// process all notes in the chord
			for( int i = 0; i < chord_table.chords()[selected_chord].size(); ++i )
			{
				// add interval to sub-note-key
				const int sub_note_key = sub_note_key_base + (int) chord_table.chords()[selected_chord][i];
				// maybe we're out of range -> let's get outta
				// here!
				if( sub_note_key > NumKeys )
				{
					break;
				}
				// create copy of base-note
				Note note_copy( _n->length(), 0, sub_note_key, _n->getVolume(), _n->getPanning(), _n->detuning() );

				// create sub-note-play-handle, only note is
				// different
				Engine::audioEngine()->addPlayHandle(
						NotePlayHandleManager::acquire( _n->instrumentTrack(), _n->offset(), _n->frames(), note_copy,
									_n, -1, NotePlayHandle::Origin::NoteStacking )
						);
			}
		}
	}
}




void InstrumentFunctionNoteStacking::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_chordsEnabledModel.saveSettings( _doc, _this, "chord-enabled" );
	m_chordsModel.saveSettings( _doc, _this, "chord" );
	m_chordRangeModel.saveSettings( _doc, _this, "chordrange" );
}




void InstrumentFunctionNoteStacking::loadSettings( const QDomElement & _this )
{
	m_chordsEnabledModel.loadSettings( _this, "chord-enabled" );
	m_chordsModel.loadSettings( _this, "chord" );
	m_chordRangeModel.loadSettings( _this, "chordrange" );
}







InstrumentFunctionArpeggio::InstrumentFunctionArpeggio( Model * _parent ) :
	Model( _parent, tr( "Arpeggio" ) ),
	m_arpEnabledModel( false ),
	m_arpModel( this, tr( "Arpeggio type" ) ),
	m_arpRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Arpeggio range" ) ),
	m_arpRepeatsModel( 1.0f, 1.0f, 8.0f, 1.0f, this, tr( "Note repeats" ) ),
	m_arpCycleModel( 0.0f, 0.0f, 6.0f, 1.0f, this, tr( "Cycle steps" ) ),
	m_arpSkipModel( 0.0f, 0.0f, 100.0f, 1.0f, this, tr( "Skip rate" ) ),
	m_arpMissModel( 0.0f, 0.0f, 100.0f, 1.0f, this, tr( "Miss rate" ) ),
	m_arpTimeModel( 200.0f, 25.0f, 2000.0f, 1.0f, 2000, this, tr( "Arpeggio time" ) ),
	m_arpGateModel( 100.0f, 1.0f, 200.0f, 1.0f, this, tr( "Arpeggio gate" ) ),
	m_arpDirectionModel( this, tr( "Arpeggio direction" ) ),
	m_arpModeModel( this, tr( "Arpeggio mode" ) )
{
	const InstrumentFunctionNoteStacking::ChordTable & chord_table = InstrumentFunctionNoteStacking::ChordTable::getInstance();
	for (auto& chord : chord_table.chords())
	{
		m_arpModel.addItem(chord.getName());
	}

	m_arpDirectionModel.addItem( tr( "Up" ), std::make_unique<PixmapLoader>( "arp_up" ) );
	m_arpDirectionModel.addItem( tr( "Down" ), std::make_unique<PixmapLoader>( "arp_down" ) );
	m_arpDirectionModel.addItem( tr( "Up and down" ), std::make_unique<PixmapLoader>( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Down and up" ), std::make_unique<PixmapLoader>( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Random" ), std::make_unique<PixmapLoader>( "arp_random" ) );
	m_arpDirectionModel.setInitValue( static_cast<float>(ArpDirection::Up) );

	m_arpModeModel.addItem( tr( "Free" ), std::make_unique<PixmapLoader>( "arp_free" ) );
	m_arpModeModel.addItem( tr( "Sort" ), std::make_unique<PixmapLoader>( "arp_sort" ) );
	m_arpModeModel.addItem( tr( "Sync" ), std::make_unique<PixmapLoader>( "arp_sync" ) );
}






void InstrumentFunctionArpeggio::processNote( NotePlayHandle * _n )
{
	const int base_note_key = _n->key();
	if( _n->origin() == NotePlayHandle::Origin::Arpeggio ||
		_n->origin() == NotePlayHandle::Origin::NoteStacking ||
		!m_arpEnabledModel.value() ||
		_n->isReleased() )
	{
		return;
	}

	// Set master note if not playing arp note or it will play as an ordinary note
	_n->setMasterNote();

	const int selected_arp = m_arpModel.value();
	const auto arpMode = static_cast<ArpMode>(m_arpModeModel.value());

	ConstNotePlayHandleList cnphv = NotePlayHandle::nphsOfInstrumentTrack(_n->instrumentTrack());

	if(arpMode != ArpMode::Free && cnphv.size() == 0 )
	{
		// maybe we're playing only a preset-preview-note?
		cnphv = PresetPreviewPlayHandle::nphsOfInstrumentTrack( _n->instrumentTrack() );
		if( cnphv.size() == 0 )
		{
			// still nothing found here, so lets return
			//return;
			cnphv.push_back( _n );
		}
	}

	// avoid playing same key for all
	// currently playing notes if sort mode is enabled
	if (arpMode == ArpMode::Sort && _n != cnphv.first()) { return; }

	const InstrumentFunctionNoteStacking::ChordTable & chord_table = InstrumentFunctionNoteStacking::ChordTable::getInstance();
	const int cur_chord_size = chord_table.chords()[selected_arp].size();
	const int total_chord_size = cur_chord_size * cnphv.size();
	// how many notes are in a single chord (multiplied by range)
	const int singleNoteRange = static_cast<int>(cur_chord_size * m_arpRangeModel.value() * m_arpRepeatsModel.value());
	// how many notes are in the final chord
	const int range = arpMode == ArpMode::Sort ? singleNoteRange * cnphv.size() : singleNoteRange;

	if (arpMode == ArpMode::Sort)
	{
		std::sort(cnphv.begin(), cnphv.end(), [](const NotePlayHandle* a, const NotePlayHandle* b)
		{
			return a->key() < b->key();
		});
	}

	// number of frames that every note should be played
	const auto arp_frames = (f_cnt_t)(m_arpTimeModel.value() / 1000.0f * Engine::audioEngine()->outputSampleRate());
	const auto gated_frames = (f_cnt_t)(m_arpGateModel.value() * arp_frames / 100.0f);

	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = (arpMode != ArpMode::Free ?
						cnphv.first()->totalFramesPlayed() :
						_n->totalFramesPlayed()) + arp_frames - 1;
	// used for loop
	f_cnt_t frames_processed = arpMode != ArpMode::Free ? cnphv.first()->noteOffset() : _n->noteOffset();

	while( frames_processed < Engine::audioEngine()->framesPerPeriod() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames - ( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer or is the remaining time just
		// a short bit that we can discard?
		if( remaining_frames_for_cur_arp > Engine::audioEngine()->framesPerPeriod() ||
			_n->frames() - _n->totalFramesPlayed() < arp_frames / 5 )
		{
			// then we don't have to do something!
			break;
		}

		frames_processed += remaining_frames_for_cur_arp;

		// Skip notes randomly
		if( m_arpSkipModel.value() )
		{
			if (100 * static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f) < m_arpSkipModel.value())
			{
				// update counters
				frames_processed += arp_frames;
				cur_frame += arp_frames;
				continue;
			}
		}

		auto dir = static_cast<ArpDirection>(m_arpDirectionModel.value());

		// Miss notes randomly. We intercept int dir and abuse it
		// after need.  :)

		if( m_arpMissModel.value() )
		{
			if (100 * static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f) < m_arpMissModel.value())
			{
				dir = ArpDirection::Random;
			}
		}

		int cur_arp_idx = 0;
		// process according to arpeggio-direction...
		if (dir == ArpDirection::Up || dir == ArpDirection::Down)
		{
			cur_arp_idx = (cur_frame / arp_frames) % range;
		}
		else if ((dir == ArpDirection::UpAndDown || dir == ArpDirection::DownAndUp) && range > 1)
		{
			// imagine, we had to play the arp once up and then
			// once down -> makes 2 * range possible notes...
			// because we don't play the lower and upper notes
			// twice, we have to subtract 2
			cur_arp_idx = (cur_frame / arp_frames) % (range * 2 - (2 * static_cast<int>(m_arpRepeatsModel.value())));
			// if greater than range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if (cur_arp_idx >= range)
			{
				cur_arp_idx = range - cur_arp_idx % (range - 1) - static_cast<int>(m_arpRepeatsModel.value());
			}
		}
		else if( dir == ArpDirection::Random )
		{
			// just pick a random chord-index
			cur_arp_idx = static_cast<int>(range * static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
		}

		// Divide cur_arp_idx with wanted repeats. The repeat feature will not affect random notes.
		cur_arp_idx = static_cast<int>(cur_arp_idx / m_arpRepeatsModel.value());

		// Cycle notes
		if( m_arpCycleModel.value() && dir != ArpDirection::Random )
		{
			cur_arp_idx *= m_arpCycleModel.value() + 1;
			cur_arp_idx %= static_cast<int>(range / m_arpRepeatsModel.value());
		}

		// If ArpDirection::Down or ArpDirection::DownAndUp, invert the final range.
		if (dir == ArpDirection::Down || dir == ArpDirection::DownAndUp)
		{
			cur_arp_idx = static_cast<int>(range / m_arpRepeatsModel.value()) - cur_arp_idx - 1;
		}

		// now calculate final key for our arp-note
		int sub_note_key = 0;
		if (arpMode != ArpMode::Sort)
		{
			sub_note_key = base_note_key + (cur_arp_idx / cur_chord_size) *
							KeysPerOctave + chord_table.chords()[selected_arp][cur_arp_idx % cur_chord_size];
		}
		else
		{
			const auto octaveDiv = std::div(cur_arp_idx, total_chord_size);
			const int octave = octaveDiv.quot;
			const auto arpDiv = std::div(octaveDiv.rem, cnphv.size());
			const int arpIndex = arpDiv.rem;
			const int chordIndex = arpDiv.quot;
			sub_note_key = cnphv[arpIndex]->key()
				+ chord_table.chords()[selected_arp][chordIndex]
				+ octave * KeysPerOctave;
		}

		// range-checking
		if( sub_note_key >= NumKeys ||
			sub_note_key < 0 ||
			Engine::audioEngine()->criticalXRuns() )
		{
			continue;
		}

		// create new arp-note

		// create sub-note-play-handle, only ptr to note is different
		// and is_arp_note=true
		Engine::audioEngine()->addPlayHandle(
				NotePlayHandleManager::acquire( _n->instrumentTrack(),
							frames_processed,
							gated_frames,
							Note( TimePos( 0 ), TimePos( 0 ), sub_note_key, _n->getVolume(),
									_n->getPanning(), _n->detuning() ),
							_n, -1, NotePlayHandle::Origin::Arpeggio )
				);

		// update counters
		frames_processed += arp_frames;
		cur_frame += arp_frames;
	}
}




void InstrumentFunctionArpeggio::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_arpEnabledModel.saveSettings( _doc, _this, "arp-enabled" );
	m_arpModel.saveSettings( _doc, _this, "arp" );
	m_arpRangeModel.saveSettings( _doc, _this, "arprange" );
	m_arpRepeatsModel.saveSettings( _doc, _this, "arprepeats" );
	m_arpCycleModel.saveSettings( _doc, _this, "arpcycle" );
	m_arpSkipModel.saveSettings( _doc, _this, "arpskip" );
	m_arpMissModel.saveSettings( _doc, _this, "arpmiss" );
	m_arpTimeModel.saveSettings( _doc, _this, "arptime" );
	m_arpGateModel.saveSettings( _doc, _this, "arpgate" );
	m_arpDirectionModel.saveSettings( _doc, _this, "arpdir" );
	m_arpModeModel.saveSettings( _doc, _this, "arpmode" );
}




void InstrumentFunctionArpeggio::loadSettings( const QDomElement & _this )
{
	m_arpEnabledModel.loadSettings( _this, "arp-enabled" );
	m_arpModel.loadSettings( _this, "arp" );
	m_arpRangeModel.loadSettings( _this, "arprange" );
	m_arpRepeatsModel.loadSettings( _this, "arprepeats" );
	m_arpCycleModel.loadSettings( _this, "arpcycle" );
	m_arpSkipModel.loadSettings( _this, "arpskip" );
	m_arpMissModel.loadSettings( _this, "arpmiss" );
	m_arpTimeModel.loadSettings( _this, "arptime" );
	m_arpGateModel.loadSettings( _this, "arpgate" );
	m_arpDirectionModel.loadSettings( _this, "arpdir" );
	m_arpModeModel.loadSettings( _this, "arpmode" );
}


} // namespace lmms
