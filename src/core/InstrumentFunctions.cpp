/*
 * InstrumentFunctions.cpp - models for instrument-function-tab
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "InstrumentFunctions.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "PresetPreviewPlayHandle.h"



//Volume (uint8_t): 0->200 Panning (int8_t): -100 -> +100
InstrumentFunctionNoteStacking::ChordTable::Init InstrumentFunctionNoteStacking::ChordTable::s_initTable[] =
{
    /*
    The data for the semitone: {a,b,c,d,e}
    a: key
    b: volume - from 0 to 200 (or in percentage??)
    c: panning - from -100 to 100 (or in percentage??)
    d: active (true/false) - if the note is skipped
    e: silenced (true/false) - if the note is played or there is silence
    */
    { QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "octave" ), {{ 0, -1, -1, 1, 1 } ,{ }} },
    { QT_TRANSLATE_NOOP( "InstrumentFunctionNoteStacking", "Batman" ), { 0, 0, -2, -2, -5, -5, -2, -2, BOUNDARY_CHORD } }
} ;



/**
 * <p>Gets data into Chord!</p>
 * @brief InstrumentFunctionNoteStacking::Chord::Chord
 * @param n
 * @param semi_tones
 */
InstrumentFunctionNoteStacking::Chord::Chord( const char * n, const ChordSemiTones & semi_tones ) :
    m_name( InstrumentFunctionNoteStacking::tr( n ) ) //assegnazione diretta, inizializzazione
{
    //poglej ce dela, sicer bos moral implementirat iterator;
    //probaj tudi m_semiTones=&semi_tones
    m_semiTones=semi_tones;

}



/**
 * <p>Checks if the chord contains the semitone; To be considered equal the semitone must have the same key, volume, panning etc. values.</p>
 * <p>It relies on the operator function == of the struct chord_arp_note_struct</p>
 * @brief InstrumentFunctionNoteStacking::Chord::hasSemiTone
 * @param semi_tone
 * @return
 */
bool InstrumentFunctionNoteStacking::Chord::hasSemiTone( chord_arp_note semi_tone ) const
{
    //Using the qvector function contains
    return m_semiTones.contains(semi_tone);
}



/**
 * <p>Gets data from the table above</p>
 * TLE NAREDIS PROGRAMSKO LOGIKO zamenjaj stvari gor!
 * @brief InstrumentFunctionNoteStacking::ChordTable::ChordTable
 */
InstrumentFunctionNoteStacking::ChordTable::ChordTable() :
	QVector<Chord>()
{
	for( int i = 0;
		i < static_cast<int>( sizeof s_initTable / sizeof *s_initTable );
		i++ )
	{
		push_back( Chord( s_initTable[i].m_name, s_initTable[i].m_semiTones ) );
	}
}




const InstrumentFunctionNoteStacking::Chord & InstrumentFunctionNoteStacking::ChordTable::getByName( const QString & name, bool is_scale ) const
{
	for( int i = 0; i < size(); i++ )
	{
		if( at( i ).getName() == name && is_scale == at( i ).isScale() )
			return at( i );
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
	for( int i = 0; i < chord_table.size(); ++i )
	{
		m_chordsModel.addItem( chord_table[i].getName() );
	}
}




InstrumentFunctionNoteStacking::~InstrumentFunctionNoteStacking()
{
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
	if( ( _n->origin() == NotePlayHandle::OriginArpeggio || ( _n->hasParent() == false && _n->instrumentTrack()->isArpeggioEnabled() == false ) ) &&
			_n->totalFramesPlayed() == 0 &&
			m_chordsEnabledModel.value() == true && ! _n->isReleased() )
	{
		// then insert sub-notes for chord
		const int selected_chord = m_chordsModel.value();

		for( int octave_cnt = 0; octave_cnt < m_chordRangeModel.value(); ++octave_cnt )
		{
			const int sub_note_key_base = base_note_key + octave_cnt * KeysPerOctave;

			// process all notes in the chord
			for( int i = 0; i < chord_table[selected_chord].size(); ++i )
			{
				// add interval to sub-note-key
				const int sub_note_key = sub_note_key_base + (int) chord_table[selected_chord][i];
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
				Engine::mixer()->addPlayHandle( 
						NotePlayHandleManager::acquire( _n->instrumentTrack(), _n->offset(), _n->frames(), note_copy,
									_n, -1, NotePlayHandle::OriginNoteStacking )
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
	m_arpCycleModel( 0.0f, 0.0f, 6.0f, 1.0f, this, tr( "Cycle steps" ) ),
	m_arpSkipModel( 0.0f, 0.0f, 100.0f, 1.0f, this, tr( "Skip rate" ) ),
	m_arpMissModel( 0.0f, 0.0f, 100.0f, 1.0f, this, tr( "Miss rate" ) ),
	m_arpTimeModel( 100.0f, 25.0f, 2000.0f, 1.0f, 2000, this, tr( "Arpeggio time" ) ),
	m_arpGateModel( 100.0f, 1.0f, 200.0f, 1.0f, this, tr( "Arpeggio gate" ) ),
	m_arpDirectionModel( this, tr( "Arpeggio direction" ) ),
	m_arpModeModel( this, tr( "Arpeggio mode" ) )
{
	const InstrumentFunctionNoteStacking::ChordTable & chord_table = InstrumentFunctionNoteStacking::ChordTable::getInstance();
	for( int i = 0; i < chord_table.size(); ++i )
	{
		m_arpModel.addItem( chord_table[i].getName() );
	}

	m_arpDirectionModel.addItem( tr( "Up" ), new PixmapLoader( "arp_up" ) );
	m_arpDirectionModel.addItem( tr( "Down" ), new PixmapLoader( "arp_down" ) );
	m_arpDirectionModel.addItem( tr( "Up and down" ), new PixmapLoader( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Random" ), new PixmapLoader( "arp_random" ) );
	m_arpDirectionModel.addItem( tr( "Down and up" ), new PixmapLoader( "arp_up_and_down" ) );
	m_arpDirectionModel.setInitValue( ArpDirUp );

	m_arpModeModel.addItem( tr( "Free" ), new PixmapLoader( "arp_free" ) );
	m_arpModeModel.addItem( tr( "Sort" ), new PixmapLoader( "arp_sort" ) );
	m_arpModeModel.addItem( tr( "Sync" ), new PixmapLoader( "arp_sync" ) );
}




InstrumentFunctionArpeggio::~InstrumentFunctionArpeggio()
{
}




void InstrumentFunctionArpeggio::processNote( NotePlayHandle * _n )
{
	const int base_note_key = _n->key();
	if( _n->origin() == NotePlayHandle::OriginArpeggio ||
		_n->origin() == NotePlayHandle::OriginNoteStacking ||
		!m_arpEnabledModel.value() ||
		( _n->isReleased() && _n->releaseFramesDone() >= _n->actualReleaseFramesToDo() ) )
	{
		return;
	}


	const int selected_arp = m_arpModel.value();

	ConstNotePlayHandleList cnphv = NotePlayHandle::nphsOfInstrumentTrack( _n->instrumentTrack() );

	if( m_arpModeModel.value() != FreeMode && cnphv.size() == 0 )
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

	const InstrumentFunctionNoteStacking::ChordTable & chord_table = InstrumentFunctionNoteStacking::ChordTable::getInstance();
	const int cur_chord_size = chord_table[selected_arp].size();
	const int range = (int)( cur_chord_size * m_arpRangeModel.value() );
	const int total_range = range * cnphv.size();

	// number of frames that every note should be played
	const f_cnt_t arp_frames = (f_cnt_t)( m_arpTimeModel.value() / 1000.0f * Engine::mixer()->processingSampleRate() );
	const f_cnt_t gated_frames = (f_cnt_t)( m_arpGateModel.value() * arp_frames / 100.0f );

	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = ( ( m_arpModeModel.value() != FreeMode ) ?
						cnphv.first()->totalFramesPlayed() :
						_n->totalFramesPlayed() ) + arp_frames - 1;
	// used for loop
	f_cnt_t frames_processed = ( m_arpModeModel.value() != FreeMode ) ? cnphv.first()->noteOffset() : _n->noteOffset();

	while( frames_processed < Engine::mixer()->framesPerPeriod() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames - ( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp > Engine::mixer()->framesPerPeriod() )
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
				( ( cur_frame / arp_frames ) % total_range ) / range != (f_cnt_t) _n->index() )
		{
			// update counters
			frames_processed += arp_frames;
			cur_frame += arp_frames;
			continue;
		}

		// Skip notes randomly
		if( m_arpSkipModel.value() )
		{

			if( 100 * ( (float) rand() / (float)( RAND_MAX + 1.0f ) ) < m_arpSkipModel.value() )
			{
				if( cur_arp_idx == 0 )
				{
					_n->setMasterNote();
				}
				// update counters
				frames_processed += arp_frames;
				cur_frame += arp_frames;
				continue;
			}
		}

		int dir = m_arpDirectionModel.value();

		// Miss notes randomly. We intercept int dir and abuse it
		// after need.  :)

		if( m_arpMissModel.value() )
		{
			if( 100 * ( (float) rand() / (float)( RAND_MAX + 1.0f ) ) < m_arpMissModel.value() )
			{
				dir = ArpDirRandom;
			}
		}

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
			cur_arp_idx = ( cur_frame / arp_frames ) % ( range * 2 - 2 );
			// if greater than range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if( cur_arp_idx >= range )
			{
				cur_arp_idx = range - cur_arp_idx % ( range - 1 ) - 1;
			}
		}
		else if( dir == ArpDirDownAndUp && range > 1 )
		{
			// copied from ArpDirUpAndDown above
			cur_arp_idx = ( cur_frame / arp_frames ) % ( range * 2 - 2 );
			// if greater than range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if( cur_arp_idx >= range )
			{
				cur_arp_idx = range - cur_arp_idx % ( range - 1 ) - 1;
			}
			// inverts direction
			cur_arp_idx = range - cur_arp_idx - 1;
		}
		else if( dir == ArpDirRandom )
		{
			// just pick a random chord-index
			cur_arp_idx = (int)( range * ( (float) rand() / (float) RAND_MAX ) );
		}

		// Cycle notes
		if( m_arpCycleModel.value() && dir != ArpDirRandom )
		{
			cur_arp_idx *= m_arpCycleModel.value() + 1;
			cur_arp_idx %= range;
		}

		// now calculate final key for our arp-note
		const int sub_note_key = base_note_key + (cur_arp_idx / cur_chord_size ) *
							KeysPerOctave + chord_table[selected_arp][cur_arp_idx % cur_chord_size];

		// range-checking
		if( sub_note_key >= NumKeys ||
			sub_note_key < 0 ||
			Engine::mixer()->criticalXRuns() )
		{
			continue;
		}

		float vol_level = 1.0f;
		if( _n->isReleased() )
		{
			vol_level = _n->volumeLevel( cur_frame + gated_frames );
		}

		// create new arp-note

		// create sub-note-play-handle, only ptr to note is different
		// and is_arp_note=true
		Engine::mixer()->addPlayHandle(
				NotePlayHandleManager::acquire( _n->instrumentTrack(),
							frames_processed,
							gated_frames,
							Note( MidiTime( 0 ), MidiTime( 0 ), sub_note_key, (volume_t) qRound( _n->getVolume() * vol_level ),
									_n->getPanning(), _n->detuning() ),
							_n, -1, NotePlayHandle::OriginArpeggio )
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
	m_arpCycleModel.loadSettings( _this, "arpcycle" );
	m_arpSkipModel.loadSettings( _this, "arpskip" );
	m_arpMissModel.loadSettings( _this, "arpmiss" );
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



