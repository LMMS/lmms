/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include <QtXml/QDomElement>

#include "InstrumentFunctions.h"
#include "embed.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "note_play_handle.h"
#include "preset_preview_play_handle.h"



ChordCreator::ChordTable::Init ChordCreator::ChordTable::s_initTable[] =
{
	{ QT_TRANSLATE_NOOP( "ChordCreator", "octave" ), { 0, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Major" ), { 0, 4, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Majb5" ), { 0, 4, 6, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "minor" ), { 0, 3, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "minb5" ), { 0, 3, 6, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "sus2" ), { 0, 2, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "sus4" ), { 0, 5, 7, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "aug" ), { 0, 4, 8, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "augsus4" ), { 0, 5, 8, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "tri" ), { 0, 3, 6, 9, -1 } },
	
	{ QT_TRANSLATE_NOOP( "ChordCreator", "6" ), { 0, 4, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "6sus4" ), { 0, 5, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "6add9" ), { 0, 4, 7, 9, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m6" ), { 0, 3, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m6add9" ), { 0, 3, 7, 9, 14, -1 } },

	{ QT_TRANSLATE_NOOP( "ChordCreator", "7" ), { 0, 4, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7sus4" ), { 0, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7#5" ), { 0, 4, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7b5" ), { 0, 4, 6, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7#9" ), { 0, 4, 7, 10, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7b9" ), { 0, 4, 7, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7#5#9" ), { 0, 4, 8, 10, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7#5b9" ), { 0, 4, 8, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7b5b9" ), { 0, 4, 6, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7add11" ), { 0, 4, 7, 10, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7add13" ), { 0, 4, 7, 10, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "7#11" ), { 0, 4, 7, 10, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj7" ), { 0, 4, 7, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj7b5" ), { 0, 4, 6, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj7#5" ), { 0, 4, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj7#11" ), { 0, 4, 7, 11, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj7add13" ), { 0, 4, 7, 11, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m7" ), { 0, 3, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m7b5" ), { 0, 3, 6, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m7b9" ), { 0, 3, 7, 10, 13, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m7add11" ), { 0, 3, 7, 10, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m7add13" ), { 0, 3, 7, 10, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m-Maj7" ), { 0, 3, 7, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m-Maj7add11" ), { 0, 3, 7, 11, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m-Maj7add13" ), { 0, 3, 7, 11, 21, -1 } },

	{ QT_TRANSLATE_NOOP( "ChordCreator", "9" ), { 0, 4, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "9sus4" ), { 0, 5, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "add9" ), { 0, 4, 7, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "9#5" ), { 0, 4, 8, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "9b5" ), { 0, 4, 6, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "9#11" ), { 0, 4, 7, 10, 14, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "9b13" ), { 0, 4, 7, 10, 14, 20, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj9" ), { 0, 4, 7, 11, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj9sus4" ), { 0, 5, 7, 11, 15, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj9#5" ), { 0, 4, 8, 11, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj9#11" ), { 0, 4, 7, 11, 14, 18, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m9" ), { 0, 3, 7, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "madd9" ), { 0, 3, 7, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m9b5" ), { 0, 3, 6, 10, 14, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m9-Maj7" ), { 0, 3, 7, 11, 14, -1 } },

	{ QT_TRANSLATE_NOOP( "ChordCreator", "11" ), { 0, 4, 7, 10, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "11b9" ), { 0, 4, 7, 10, 13, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj11" ), { 0, 4, 7, 11, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m11" ), { 0, 3, 7, 10, 14, 17, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m-Maj11" ), { 0, 3, 7, 11, 14, 17, -1 } },

	{ QT_TRANSLATE_NOOP( "ChordCreator", "13" ), { 0, 4, 7, 10, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "13#9" ), { 0, 4, 7, 10, 15, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "13b9" ), { 0, 4, 7, 10, 13, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "13b5b9" ), { 0, 4, 6, 10, 13, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Maj13" ), { 0, 4, 7, 11, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m13" ), { 0, 3, 7, 10, 14, 21, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "m-Maj13" ), { 0, 3, 7, 11, 14, 21, -1 } },

	{ QT_TRANSLATE_NOOP( "ChordCreator", "Major" ), { 0, 2, 4, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Harmonic minor" ), { 0, 2, 3, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Melodic minor" ), { 0, 2, 3, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Whole tone" ), { 0, 2, 4, 6, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Diminished" ), { 0, 2, 3, 5, 6, 8, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Major pentatonic" ), { 0, 2, 4, 7, 9, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Minor pentatonic" ), { 0, 3, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Jap in sen" ), { 0, 1, 5, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Major bebop" ), { 0, 2, 4, 5, 7, 8, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Dominant bebop" ), { 0, 2, 4, 5, 7, 9, 10, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Blues" ), { 0, 3, 5, 6, 7, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Arabic" ), { 0, 1, 4, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Enigmatic" ), { 0, 1, 4, 6, 8, 10, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Neopolitan" ), { 0, 1, 3, 5, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Neopolitan minor" ), { 0, 1, 3, 5, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Hungarian minor" ), { 0, 2, 3, 6, 7, 8, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Dorian" ), { 0, 2, 3, 5, 7, 9, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Phrygolydian" ), { 0, 1, 3, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Lydian" ), { 0, 2, 4, 6, 7, 9, 11, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Mixolydian" ), { 0, 2, 4, 5, 7, 9, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Aeolian" ), { 0, 2, 3, 5, 7, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Locrian" ), { 0, 1, 3, 5, 6, 8, 10, -1 } },
	{ QT_TRANSLATE_NOOP( "ChordCreator", "Minor" ), { 0, 2, 3, 5, 7, 8, 10, -1 } },
} ;




ChordCreator::Chord::Chord( const char * n, const ChordSemiTones & semi_tones ) :
	m_name( ChordCreator::tr( n ) )
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




bool ChordCreator::Chord::hasSemiTone( Sint8 semi_tone ) const
{
	for( int i = 0; i < size(); ++i )
	{
		if( semi_tone == m_semiTones[i] )
			return true;
	}
	return false;
}




ChordCreator::ChordTable::ChordTable() :
	QVector<Chord>()
{
	for( int i = 0;
		i < static_cast<int>( sizeof s_initTable / sizeof *s_initTable );
		i++ )
	{
		push_back( Chord( s_initTable[i].m_name, s_initTable[i].m_semiTones ) );
	}
}




const ChordCreator::Chord & ChordCreator::ChordTable::getByName( const QString & name, bool is_scale ) const
{
	for( int i = 0; i < size(); i++ )
	{
		if( at( i ).getName() == name && is_scale == at( i ).isScale() )
			return at( i );
	}

	static Chord empty;
	return empty;
}




ChordCreator::ChordCreator( Model * _parent ) :
	Model( _parent, tr( "Chords" ) ),
	m_chordsEnabledModel( false, this ),
	m_chordsModel( this, tr( "Chord type" ) ),
	m_chordRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Chord range" ) )
{
	const ChordTable & chord_table = ChordTable::getInstance();
	for( int i = 0; i < chord_table.size(); ++i )
	{
		m_chordsModel.addItem( chord_table[i].getName() );
	}
}




ChordCreator::~ChordCreator()
{
}




void ChordCreator::processNote( notePlayHandle * _n )
{
	const int base_note_key = _n->key();
	const ChordTable & chord_table = ChordTable::getInstance();
	// we add chord-subnotes to note if either note is a base-note and
	// arpeggio is not used or note is part of an arpeggio
	// at the same time we only add sub-notes if nothing of the note was
	// played yet, because otherwise we would add chord-subnotes every
	// time an audio-buffer is rendered...
	if( ( ( _n->isTopNote() &&
		_n->instrumentTrack()->isArpeggiatorEnabled() == false ) ||
						_n->isPartOfArpeggio() ) &&
				_n->totalFramesPlayed() == 0 &&
				m_chordsEnabledModel.value() == true )
	{
		// then insert sub-notes for chord
		const int selected_chord = m_chordsModel.value();

		for( int octave_cnt = 0;
			octave_cnt < m_chordRangeModel.value(); ++octave_cnt )
		{
			const int sub_note_key_base = base_note_key +
						octave_cnt * KeysPerOctave;
			// if octave_cnt == 1 we're in the first octave and
			// the base-note is already done, so we don't have to
			// create it in the following loop, then we loop until
			// there's a -1 in the interval-array
			for( int i = ( octave_cnt == 0 ) ? 1 : 0;
				i < chord_table[selected_chord].size();
									++i )
			{
				// add interval to sub-note-key
				const int sub_note_key = sub_note_key_base +
							(int) chord_table[
						selected_chord][i];
				// maybe we're out of range -> let's get outta
				// here!
				if( sub_note_key > NumKeys )
				{
					break;
				}
				// create copy of base-note
				note note_copy( _n->length(), 0, sub_note_key,
							_n->getVolume(),
							_n->getPanning(),
							_n->detuning() );
				// create sub-note-play-handle, only note is
				// different
				new notePlayHandle( _n->instrumentTrack(),
							_n->offset(),
							_n->frames(), note_copy,
							_n );
			}
		}
	}


}




void ChordCreator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_chordsEnabledModel.saveSettings( _doc, _this, "chord-enabled" );
	m_chordsModel.saveSettings( _doc, _this, "chord" );
	m_chordRangeModel.saveSettings( _doc, _this, "chordrange" );
}




void ChordCreator::loadSettings( const QDomElement & _this )
{
	m_chordsEnabledModel.loadSettings( _this, "chord-enabled" );
	m_chordsModel.loadSettings( _this, "chord" );
	m_chordRangeModel.loadSettings( _this, "chordrange" );
}







Arpeggiator::Arpeggiator( Model * _parent ) :
	Model( _parent, tr( "Arpeggio" ) ),
	m_arpEnabledModel( false ),
	m_arpModel( this, tr( "Arpeggio type" ) ),
	m_arpRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Arpeggio range" ) ),
	m_arpTimeModel( 100.0f, 25.0f, 2000.0f, 1.0f, 2000, this,
							tr( "Arpeggio time" ) ),
	m_arpGateModel( 100.0f, 1.0f, 200.0f, 1.0f, this,
							tr( "Arpeggio gate" ) ),
	m_arpDirectionModel( this, tr( "Arpeggio direction" ) ),
	m_arpModeModel( this, tr( "Arpeggio mode" ) )
{
	const ChordCreator::ChordTable & chord_table = ChordCreator::ChordTable::getInstance();
	for( int i = 0; i < chord_table.size(); ++i )
	{
		m_arpModel.addItem( chord_table[i].getName() );
	}

	m_arpDirectionModel.addItem( tr( "Up" ), new PixmapLoader( "arp_up" ) );
	m_arpDirectionModel.addItem( tr( "Down" ), new PixmapLoader( "arp_down" ) );
	m_arpDirectionModel.addItem( tr( "Up and down" ),
										new PixmapLoader( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Random" ),
										new PixmapLoader( "arp_random" ) );
	m_arpDirectionModel.setInitValue( ArpDirUp );

	m_arpModeModel.addItem( tr( "Free" ), new PixmapLoader( "arp_free" ) );
	m_arpModeModel.addItem( tr( "Sort" ), new PixmapLoader( "arp_sort" ) );
	m_arpModeModel.addItem( tr( "Sync" ), new PixmapLoader( "arp_sync" ) );
}




Arpeggiator::~Arpeggiator()
{
}




void Arpeggiator::processNote( notePlayHandle * _n )
{
	const int base_note_key = _n->key();
	if( _n->isTopNote() == false ||
			!m_arpEnabledModel.value() ||
			( _n->released() && _n->releaseFramesDone() >=
					_n->actualReleaseFramesToDo() ) )
	{
		return;
	}


	const int selected_arp = m_arpModel.value();

	ConstNotePlayHandleList cnphv = notePlayHandle::nphsOfInstrumentTrack(
													_n->instrumentTrack() );
	if( m_arpModeModel.value() != FreeMode && cnphv.size() == 0 )
	{
		// maybe we're playing only a preset-preview-note?
		cnphv = presetPreviewPlayHandle::nphsOfInstrumentTrack(
						_n->instrumentTrack() );
		if( cnphv.size() == 0 )
		{
			// still nothing found here, so lets return
			//return;
			cnphv.push_back( _n );
		}
	}

	const ChordCreator::ChordTable & chord_table = ChordCreator::ChordTable::getInstance();
	const int cur_chord_size = chord_table[selected_arp].size();
	const int range = (int)( cur_chord_size * m_arpRangeModel.value() );
	const int total_range = range * cnphv.size();

	// number of frames that every note should be played
	const f_cnt_t arp_frames = (f_cnt_t)( m_arpTimeModel.value() / 1000.0f *
				engine::mixer()->processingSampleRate() );
	const f_cnt_t gated_frames = (f_cnt_t)( m_arpGateModel.value() *
							arp_frames / 100.0f );

	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = ( ( m_arpModeModel.value() != FreeMode ) ?
				cnphv.first()->totalFramesPlayed() :
				_n->totalFramesPlayed() ) + arp_frames - 1;
	// used for loop
	f_cnt_t frames_processed = 0;

	while( frames_processed < engine::mixer()->framesPerPeriod() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames -
						( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp >
				engine::mixer()->framesPerPeriod() )
		{
			// then we don't have to do something!
			break;
		}

		frames_processed += remaining_frames_for_cur_arp;

		// init with zero
		int cur_arp_idx = 0;

		// in sorted mode: is it our turn or do we have to be quiet for
		// now?
		if( m_arpModeModel.value() == SortMode &&
				( ( cur_frame / arp_frames ) % total_range ) /
						range != (f_cnt_t) _n->index() )
		{
			// update counters
			frames_processed += arp_frames;
			cur_frame += arp_frames;
			continue;
		}

		const int dir = m_arpDirectionModel.value();
		// process according to arpeggio-direction...
		if( dir == ArpDirUp )
		{
			cur_arp_idx = ( cur_frame / arp_frames ) % range;
		}
		else if( dir == ArpDirDown )
		{
			cur_arp_idx = range - ( cur_frame / arp_frames ) %
								range - 1;
		}
		else if( dir == ArpDirUpAndDown && range > 1 )
		{
			// imagine, we had to play the arp once up and then
			// once down -> makes 2 * range possible notes...
			// because we don't play the lower and upper notes
			// twice, we have to subtract 2
			cur_arp_idx = ( cur_frame / arp_frames ) %
							( range * 2 - 2 );
			// if greater than range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if( cur_arp_idx >= range )
			{
				cur_arp_idx = range - cur_arp_idx %
							( range - 1 ) - 1;
			}
		}
		else if( dir == ArpDirRandom )
		{
			// just pick a random chord-index
			cur_arp_idx = (int)( range * ( (float) rand() /
							(float) RAND_MAX ) );
		}

		// now calculate final key for our arp-note
		const int sub_note_key = base_note_key + (cur_arp_idx /
							cur_chord_size ) *
							KeysPerOctave +
				chord_table[selected_arp][cur_arp_idx % cur_chord_size];

		// range-checking
		if( sub_note_key >= NumKeys ||
			sub_note_key < 0 ||
					engine::mixer()->criticalXRuns() )
		{
			continue;
		}

		float vol_level = 1.0f;
		if( _n->released() )
		{
			vol_level = _n->volumeLevel( cur_frame + gated_frames );
		}

		// create new arp-note
		note new_note( midiTime( 0 ), midiTime( 0 ),
				sub_note_key,
				(volume_t)
					qRound( _n->getVolume() * vol_level ),
				_n->getPanning(), _n->detuning() );

		// create sub-note-play-handle, only ptr to note is different
		// and is_arp_note=true
		new notePlayHandle( _n->instrumentTrack(),
				( ( m_arpModeModel.value() != FreeMode ) ?
						cnphv.first()->offset() :
						_n->offset() ) +
							frames_processed,
						gated_frames,
						new_note,
						_n, true );

		// update counters
		frames_processed += arp_frames;
		cur_frame += arp_frames;
	}

	// make sure, note is handled as arp-base-note, even if we didn't add a
	// sub-note so far
	if( m_arpModeModel.value() != FreeMode )
	{
		_n->setPartOfArpeggio( true );
	}
}




void Arpeggiator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_arpEnabledModel.saveSettings( _doc, _this, "arp-enabled" );
	m_arpModel.saveSettings( _doc, _this, "arp" );
	m_arpRangeModel.saveSettings( _doc, _this, "arprange" );
	m_arpTimeModel.saveSettings( _doc, _this, "arptime" );
	m_arpGateModel.saveSettings( _doc, _this, "arpgate" );
	m_arpDirectionModel.saveSettings( _doc, _this, "arpdir" );

	m_arpModeModel.saveSettings( _doc, _this, "arpmode" );
}




void Arpeggiator::loadSettings( const QDomElement & _this )
{
	m_arpEnabledModel.loadSettings( _this, "arp-enabled" );
	m_arpModel.loadSettings( _this, "arp" );
	m_arpRangeModel.loadSettings( _this, "arprange" );
	m_arpTimeModel.loadSettings( _this, "arptime" );
	m_arpGateModel.loadSettings( _this, "arpgate" );
	m_arpDirectionModel.loadSettings( _this, "arpdir" );
/*
	// Keep compatibility with version 0.2.1 file format
	if( _this.hasAttribute( "arpsyncmode" ) )
	{
	 	m_arpTimeKnob->setSyncMode( 
 		( tempoSyncKnob::tempoSyncMode ) _this.attribute(
 						 "arpsyncmode" ).toInt() );
	}*/

	m_arpModeModel.loadSettings( _this, "arpmode" );
}


#include "moc_InstrumentFunctions.cxx"
