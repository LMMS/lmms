/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



ChordCreator::Chord ChordCreator::s_chordTable[] =
{
	{ ChordCreator::tr( "octave" ), { 0, -1 } },
	{ ChordCreator::tr( "Major" ), { 0, 4, 7, -1 } },
	{ ChordCreator::tr( "Majb5" ), { 0, 4, 6, -1 } },
	{ ChordCreator::tr( "minor" ), { 0, 3, 7, -1 } },
	{ ChordCreator::tr( "minb5" ), { 0, 3, 6, -1 } },
	{ ChordCreator::tr( "sus2" ), { 0, 2, 7, -1 } },
	{ ChordCreator::tr( "sus4" ), { 0, 5, 7, -1 } },
	{ ChordCreator::tr( "aug" ), { 0, 4, 8, -1 } },
	{ ChordCreator::tr( "augsus4" ), { 0, 5, 8, -1 } },
	{ ChordCreator::tr( "tri" ), { 0, 3, 6, 9, -1 } },
	
	{ ChordCreator::tr( "6" ), { 0, 4, 7, 9, -1 } },
	{ ChordCreator::tr( "6sus4" ), { 0, 5, 7, 9, -1 } },
	{ ChordCreator::tr( "6add9" ), { 0, 4, 7, 12, -1 } },
	{ ChordCreator::tr( "m6" ), { 0, 3, 7, 9, -1 } },
	{ ChordCreator::tr( "m6add9" ), { 0, 3, 7, 9, 14, -1 } },

	{ ChordCreator::tr( "7" ), { 0, 4, 7, 10, -1 } },
	{ ChordCreator::tr( "7sus4" ), { 0, 5, 7, 10, -1 } },
	{ ChordCreator::tr( "7#5" ), { 0, 4, 8, 10, -1 } },
	{ ChordCreator::tr( "7b5" ), { 0, 4, 6, 10, -1 } },
	{ ChordCreator::tr( "7#9" ), { 0, 4, 7, 10, 13, 18, -1 } },
	{ ChordCreator::tr( "7b9" ), { 0, 4, 7, 10, 13, 16, -1 } },
	{ ChordCreator::tr( "7#5#9" ), { 0, 4, 8, 12, 14, 19, -1 } },
	{ ChordCreator::tr( "7#5b9" ), { 0, 4, 8, 12, 14, 17, -1 } },
	{ ChordCreator::tr( "7b5b9" ), { 0, 4, 6, 10, 12, 15, -1 } },
	{ ChordCreator::tr( "7add11" ), { 0, 4, 7, 10, 17, -1 } },
	{ ChordCreator::tr( "7add13" ), { 0, 4, 7, 10, 21, -1 } },
	{ ChordCreator::tr( "7#11" ), { 0, 4, 7, 10, 18, -1 } },
	{ ChordCreator::tr( "Maj7" ), { 0, 4, 7, 11, -1 } },
	{ ChordCreator::tr( "Maj7b5" ), { 0, 4, 6, 11, -1 } },
	{ ChordCreator::tr( "Maj7#5" ), { 0, 4, 8, 11, -1 } },
	{ ChordCreator::tr( "Maj7#11" ), { 0, 4, 7, 11, 18, -1 } },
	{ ChordCreator::tr( "Maj7add13" ), { 0, 4, 7, 11, 21, -1 } },
	{ ChordCreator::tr( "m7" ), { 0, 3, 7, 10, -1 } },
	{ ChordCreator::tr( "m7b5" ), { 0, 3, 6, 10, -1 } },
	{ ChordCreator::tr( "m7b9" ), { 0, 3, 7, 10, 13, -1 } },
	{ ChordCreator::tr( "m7add11" ), { 0, 3, 7, 10, 17, -1 } },
	{ ChordCreator::tr( "m7add13" ), { 0, 3, 7, 10, 21, -1 } },
	{ ChordCreator::tr( "m-Maj7" ), { 0, 3, 7, 11, -1 } },
	{ ChordCreator::tr( "m-Maj7add11" ), { 0, 3, 7, 11, 17, -1 } },
	{ ChordCreator::tr( "m-Maj7add13" ), { 0, 3, 7, 11, 21, -1 } },

	{ ChordCreator::tr( "9" ), { 0, 4, 7, 10, 14, -1 } },
	{ ChordCreator::tr( "9sus4" ), { 0, 5, 7, 10, 14, -1 } },
	{ ChordCreator::tr( "add9" ), { 0, 4, 7, 14, -1 } },
	{ ChordCreator::tr( "9#5" ), { 0, 4, 8, 10, 14, -1 } },
	{ ChordCreator::tr( "9b5" ), { 0, 4, 6, 10, 14, -1 } },
	{ ChordCreator::tr( "9#11" ), { 0, 4, 7, 10, 14, 18, -1 } },
	{ ChordCreator::tr( "9b13" ), { 0, 4, 7, 10, 14, 20, -1 } },
	{ ChordCreator::tr( "Maj9" ), { 0, 4, 7, 11, 14, -1 } },
	{ ChordCreator::tr( "Maj9sus4" ), { 0, 5, 7, 11, 15, -1 } },
	{ ChordCreator::tr( "Maj9#5" ), { 0, 4, 8, 11, 14, -1 } },
	{ ChordCreator::tr( "Maj9#11" ), { 0, 4, 7, 11, 14, 18, -1 } },
	{ ChordCreator::tr( "m9" ), { 0, 3, 7, 10, 14, -1 } },
	{ ChordCreator::tr( "madd9" ), { 0, 3, 7, 14, -1 } },
	{ ChordCreator::tr( "m9b5" ), { 0, 3, 6, 10, 14, -1 } },
	{ ChordCreator::tr( "m9-Maj7" ), { 0, 3, 7, 11, 14, -1 } },

	{ ChordCreator::tr( "11" ), { 0, 4, 7, 10, 14, 17, -1 } },
	{ ChordCreator::tr( "11b9" ), { 0, 4, 7, 10, 13, 17, -1 } },
	{ ChordCreator::tr( "Maj11" ), { 0, 4, 7, 11, 14, 17, -1 } },
	{ ChordCreator::tr( "m11" ), { 0, 3, 7, 10, 14, 17, -1 } },
	{ ChordCreator::tr( "m-Maj11" ), { 0, 3, 7, 11, 14, 17, -1 } },

	{ ChordCreator::tr( "13" ), { 0, 4, 7, 10, 14, 21, -1 } },
	{ ChordCreator::tr( "13#9" ), { 0, 4, 7, 10, 15, 21, -1 } },
	{ ChordCreator::tr( "13b9" ), { 0, 4, 7, 10, 13, 21, -1 } },
	{ ChordCreator::tr( "13b5b9" ), { 0, 4, 6, 10, 13, 21, -1 } },
	{ ChordCreator::tr( "Maj13" ), { 0, 4, 7, 11, 14, 21, -1 } },
	{ ChordCreator::tr( "m13" ), { 0, 3, 7, 10, 14, 21, -1 } },
	{ ChordCreator::tr( "m-Maj13" ), { 0, 3, 7, 11, 14, 21, -1 } },

	{ ChordCreator::tr( "Major" ), { 0, 2, 4, 5, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Harmonic minor" ), { 0, 2, 3, 5, 7, 8, 11, -1 } },
	{ ChordCreator::tr( "Melodic minor" ), { 0, 2, 3, 5, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Whole tone" ), { 0, 2, 4, 6, 8, 10, -1 } },
	{ ChordCreator::tr( "Diminished" ), { 0, 2, 3, 5, 6, 8, 9, 11, -1 } },
	{ ChordCreator::tr( "Major pentatonic" ), { 0, 2, 4, 7, 10, -1 } },
	{ ChordCreator::tr( "Minor pentatonic" ), { 0, 3, 5, 7, 10, -1 } },
	{ ChordCreator::tr( "Jap in sen" ), { 0, 1, 5, 7, 10, -1 } },
	{ ChordCreator::tr( "Major bebop" ), { 0, 2, 4, 5, 7, 8, 9, 11, -1 } },
	{ ChordCreator::tr( "Dominant bebop" ), { 0, 2, 4, 5, 7, 9, 10, 11, -1 } },
	{ ChordCreator::tr( "Blues" ), { 0, 3, 5, 6, 7, 10, -1 } },
	{ ChordCreator::tr( "Arabic" ), { 0, 1, 4, 5, 7, 8, 11, -1 } },
	{ ChordCreator::tr( "Enigmatic" ), { 0, 1, 4, 6, 8, 10, 11, -1 } },
	{ ChordCreator::tr( "Neopolitan" ), { 0, 1, 3, 5, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Neopolitan minor" ), { 0, 1, 3, 5, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Hungarian minor" ), { 0, 2, 3, 6, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Dorian" ), { 0, 2, 3, 5, 7, 9, 10, -1 } },
	{ ChordCreator::tr( "Phrygolydian" ), { 0, 1, 3, 5, 7, 8, 10, -1 } },
	{ ChordCreator::tr( "Lydian" ), { 0, 2, 4, 6, 7, 9, 11, -1 } },
	{ ChordCreator::tr( "Mixolydian" ), { 0, 2, 4, 5, 7, 9, 10, -1 } },
	{ ChordCreator::tr( "Aeolian" ), { 0, 2, 3, 5, 7, 8, 10, -1 } },
	{ ChordCreator::tr( "Locrian" ), { 0, 1, 3, 5, 6, 8, 10, -1 } },

	{ "", { -1, -1 } }

} ;


ChordCreator::ChordCreator( Model * _parent ) :
	Model( _parent, tr( "Chords" ) ),
	m_chordsEnabledModel( false, this ),
	m_chordsModel( this, tr( "Chord type" ) ),
	m_chordRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Chord range" ) )
{
	for( int i = 0; s_chordTable[i].interval[0] != -1; ++i )
	{
		m_chordsModel.addItem( tr( s_chordTable[i].name.toUtf8().
								constData() ) );
	}
}




ChordCreator::~ChordCreator()
{
}




void ChordCreator::processNote( notePlayHandle * _n )
{
	const int base_note_key = _n->key();
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
				s_chordTable[selected_chord].interval[i] != -1;
									++i )
			{
				// add interval to sub-note-key
				const int sub_note_key = sub_note_key_base +
							(int) s_chordTable[
						selected_chord].interval[i];
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
	for( int i = 0; ChordCreator::s_chordTable[i].interval[0] != -1; ++i )
	{
		m_arpModel.addItem( ChordCreator::tr(
					ChordCreator::s_chordTable[i].
						name.toUtf8().constData() ) );
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
		cnphv.push_back( _n );
	}

	const int cur_chord_size = ChordCreator::getChordSize(
				ChordCreator::s_chordTable[selected_arp] );
	const int range = (int)( cur_chord_size * m_arpRangeModel.value() );
	const int total_range = range * cnphv.size();

	// number of frames that every note should be played
	const f_cnt_t arp_frames = (f_cnt_t)( m_arpTimeModel.value() / 1000.0f *
				engine::getMixer()->processingSampleRate() );
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

	while( frames_processed < engine::getMixer()->framesPerPeriod() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames -
						( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp >
				engine::getMixer()->framesPerPeriod() )
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
				ChordCreator::s_chordTable[selected_arp].
					interval[cur_arp_idx % cur_chord_size];

		// range-checking
		if( sub_note_key >= NumKeys ||
			sub_note_key < 0 ||
					engine::getMixer()->criticalXRuns() )
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

