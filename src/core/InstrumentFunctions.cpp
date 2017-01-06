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

#include <QDir>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomNode>

#include "ConfigManager.h"

#include "InstrumentFunctions.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "PresetPreviewPlayHandle.h"

/*****************************************************************************************************
 *
 * The InstrumentFunctionNoteStacking class
 *
******************************************************************************************************/
InstrumentFunctionNoteStacking::InstrumentFunctionNoteStacking( Model * _parent ) :
	Model( _parent, tr( "Chords" ) ),
	m_chordsEnabledModel( false, this ),
	m_chordsModel( this, tr( "Chord type" ) ),
	m_chordRangeModel( 1.0f, 1.0f, 9.0f, 1.0f, this, tr( "Chord range" ) )
{
	m_chordTable = Engine::chordTable();
	for( int i = 0; i < m_chordTable->size(); i++ )
	{
		m_chordsModel.addItem( m_chordTable->at(i)->getName() );
	}

	//on chord Table change we reload the chord and scale combo boxes models
	connect( m_chordTable,SIGNAL(chordNameChanged()),this,SLOT (updateChordTable()));

}

InstrumentFunctionNoteStacking::~InstrumentFunctionNoteStacking()
{
}

void InstrumentFunctionNoteStacking::processNote( NotePlayHandle *_n ) 
{
	// Getting base note key
	const int base_note_key = _n->key();
	// Getting base note volume and panning
	const volume_t base_note_vol = _n->getVolume();
	const panning_t base_note_pan = _n->getPanning();
	//
	// we add chord-subnotes to note if either the note is a base-note and
	// arpeggio is not used or the note is part of an arpeggio
	// at the same time we only add sub-notes if nothing of the note was
	// played yet, because otherwise we would add chord-subnotes every
	// time an audio-buffer is rendered...
	if ((_n->origin() == NotePlayHandle::OriginArpeggio ||
			 (_n->hasParent() == false &&
				_n->instrumentTrack()->isArpeggioEnabled() == false)) &&
			_n->totalFramesPlayed() == 0 && m_chordsEnabledModel.value() == true &&
			!_n->isReleased())
	{

		// get the selected chord then insert sub-notes for the chord
		const int selected_chord = m_chordsModel.value();

		for (int octave_cnt = 0; octave_cnt < m_chordRangeModel.value();++octave_cnt)
		{
			const int sub_note_key_base = base_note_key + octave_cnt * KeysPerOctave;

			Chord *c = m_chordTable->at(selected_chord);
			ChordSemiTone *cst;
			// process all notes in the chord
			for (int i=0;i<c->size();i++)
			{
				cst=c->at(i);
				if (cst->active->value())
				{ // if the note is active process it, otherwise skip

					// getting the base note key
					int sub_note_key = sub_note_key_base + cst->key->value();

					// the new volume and panning
					volume_t sub_note_vol;
					panning_t sub_note_pan;

					if (cst->bare->value())
					{ // forget other settings but key, get the original
						// data
						sub_note_vol = base_note_vol;
						sub_note_pan = base_note_pan;
					}
					else
					{
						// The note is silenced: playing it with no volume
						if (cst->silenced->value())
						{
							sub_note_vol = 0;
							sub_note_pan = 0;
						}
						else
						{ // all modifications active, add interval to sub-note-key,
							// volume, panning
							sub_note_vol = base_note_vol * ((float)cst->vol->value() / (float)100);
							sub_note_pan = cst->pan->value();
						}
					}
					// maybe we're out of range -> let's get outta
					// here!
					if (sub_note_key > NumKeys || sub_note_key < 0 || Engine::mixer()->criticalXRuns())
					{
						break;
					}
					// create copy of base-note
					Note note_copy(_n->length(), 0, sub_note_key, sub_note_vol, sub_note_pan, _n->detuning());

					// create sub-note-play-handle, only note is
					// different

					Engine::mixer()->addPlayHandle(NotePlayHandleManager::acquire(
																					 _n->instrumentTrack(), _n->offset(), _n->frames(), note_copy, _n,
																					 -1, NotePlayHandle::OriginNoteStacking));
				}
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

void InstrumentFunctionNoteStacking::updateChordTable()
{
	//getting the existing value
	int v = m_chordsModel.value();
	m_chordsModel.clear();
	for( int i = 0; i < m_chordTable->size(); i++ )
	{
		m_chordsModel.addItem( m_chordTable->at(i)->getName() );
	}
	//setting back the value
	m_chordsModel.setValue(v);
}

/*****************************************************************************************************
 *
 * The InstrumentFunctionArpeggio class
 *
******************************************************************************************************/
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

	m_chordTable = Engine::chordTable();
	for( int i = 0; i < m_chordTable->size(); i++ )
	{
		m_arpModel.addItem( m_chordTable->at(i)->getName() );
	}

	m_arpDirectionModel.addItem( tr( "Up" ), new PixmapLoader( "arp_up" ) );
	m_arpDirectionModel.addItem( tr( "Down" ), new PixmapLoader( "arp_down" ) );
	m_arpDirectionModel.addItem( tr( "Up and down" ), new PixmapLoader( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Down and up" ), new PixmapLoader( "arp_up_and_down" ) );
	m_arpDirectionModel.addItem( tr( "Random" ), new PixmapLoader( "arp_random" ) );
	m_arpDirectionModel.setInitValue( ArpDirUp );

	m_arpModeModel.addItem( tr( "Free" ), new PixmapLoader( "arp_free" ) );
	m_arpModeModel.addItem( tr( "Sort" ), new PixmapLoader( "arp_sort" ) );
	m_arpModeModel.addItem( tr( "Sync" ), new PixmapLoader( "arp_sync" ) );

	//on chord Table change we reload the chord and scale combo boxes models
	connect( m_chordTable,SIGNAL(chordNameChanged()),this,SLOT (updateChordTable()));

}


//reloads the chordtable into the widget model
void InstrumentFunctionArpeggio::updateChordTable()
{
	//getting the existing value
	int v = m_arpModel.value();
	m_arpModel.clear();

	for( int i = 0; i < m_chordTable->size(); ++i )
	{
		m_arpModel.addItem( m_chordTable->at(i)->getName() );
	}
	//setting back the value
	m_arpModel.setValue(v);
}


InstrumentFunctionArpeggio::~InstrumentFunctionArpeggio()
{
}




void InstrumentFunctionArpeggio::processNote( NotePlayHandle * _n )
{
	// getting base note key
	const int base_note_key = _n->key();
	// Getting volume and panning
	const volume_t base_note_vol = _n->getVolume();
	const panning_t base_note_pan = _n->getPanning();

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

	const int cur_chord_size = m_chordTable->at(selected_arp)->size();
	const int range = (int)(cur_chord_size * m_arpRangeModel.value());
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

		// cst: getting the processed semitone
		ChordSemiTone *cst =		m_chordTable->at(selected_arp)->at(cur_arp_idx % cur_chord_size);

		// The note is active, process all data
		if (cst->active->value())
		{

			// now calculate final key for our arp-note
			//			const int sub_note_key = base_note_key +
			//															 (cur_arp_idx / cur_chord_size) * KeysPerOctave +
			//															 cst.key;
			const int sub_note_key = base_note_key +
															 (cur_arp_idx / cur_chord_size) * KeysPerOctave +
															 cst->key->value();
			volume_t sub_note_vol;
			panning_t sub_note_pan;
			// if the note is bare we don't intervene into panning, volume etc...
			if (cst->bare->value())
			{
				sub_note_vol = base_note_vol;
				sub_note_pan = base_note_pan;
			}
			else
			{
				// The note is silenced: playing it with no volume
				if (cst->silenced->value())
				{
					sub_note_vol = 0;
//					sub_note_pan = 0;
				}
				else
				{ // all modifications active, add interval to sub-note-key,
					// volume, panning
					sub_note_vol = base_note_vol * ((float)cst->vol->value() / (float)100);
					sub_note_pan = cst->pan->value();
				}
			}

			// range-checking
			if (sub_note_key >= NumKeys || sub_note_key < 0 || Engine::mixer()->criticalXRuns())
			{
				continue;
			}

			float vol_level = 1.0f;
			if (_n->isReleased())
			{
				vol_level = _n->volumeLevel(cur_frame + gated_frames);
			}

			// create new arp-note

			// create sub-note-play-handle, only ptr to note is different
			// and is_arp_note=true
			Engine::mixer()->addPlayHandle(NotePlayHandleManager::acquire(
																			 _n->instrumentTrack(),
																			 frames_processed, gated_frames,
																			 Note(MidiTime(0), MidiTime(0), sub_note_key,
																						(volume_t)qRound(sub_note_vol * vol_level), sub_note_pan,
																						_n->detuning()),
																			 _n, -1, NotePlayHandle::OriginArpeggio));

			// update counters
			frames_processed += arp_frames;
			cur_frame += arp_frames;

		} // end cst.active block
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

/*****************************************************************************************************
 *
 * The ChordSemiTone class
 *
******************************************************************************************************/
ChordSemiTone::ChordSemiTone(Chord *_parent) :
	Model(_parent),
	key( new IntModel(KeyDefault,KeyMin,KeyMax,_parent,tr("Key ")+_parent->m_name)),
	vol(new FloatModel(DefaultVolume,MinVolume, MaxVolume, 0.1f, _parent,tr("Volume ")+_parent->m_name)),
	pan(new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _parent, tr( "Panning " )+_parent->m_name)),
	active( new BoolModel(true,_parent,tr("Active ")+_parent->m_name)),
	silenced(new BoolModel(false,_parent,tr("Silenced ")+_parent->m_name)),
	bare( new BoolModel(false,_parent,tr("Bare ")+_parent->m_name))
{
}

ChordSemiTone::ChordSemiTone(ChordSemiTone *_copy) :
	Model( _copy->parentModel()),
	key( new IntModel(KeyDefault,KeyMin,KeyMax,_copy->getChord(),tr("Key ")+_copy->getChord()->m_name)),
	vol(new FloatModel(DefaultVolume,MinVolume, MaxVolume, 0.1f, _copy->getChord(),tr("Volume ")+_copy->getChord()->m_name)),
	pan(new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _copy->getChord(), tr( "Panning " )+_copy->getChord()->m_name)),
	active( new BoolModel(true,_copy->getChord(),tr("Active ")+_copy->getChord()->m_name)),
	silenced(new BoolModel(false,_copy->getChord(),tr("Silenced ")+_copy->getChord()->m_name)),
	bare( new BoolModel(false,_copy->getChord(),tr("Bare ")+_copy->getChord()->m_name))
{
	key->setValue(_copy->key->value());
	vol->setValue(_copy->vol->value());
	pan->setValue(_copy->pan->value());
	active->setValue(_copy->active->value());
	silenced->setValue(_copy->silenced->value());
	bare->setValue(_copy->bare->value());
}

ChordSemiTone::ChordSemiTone(Chord *_parent, QString _string) :
	Model(_parent),
	key( new IntModel(KeyDefault,KeyMin,KeyMax,_parent,tr("Key ")+_parent->m_name)),
	vol(new FloatModel(DefaultVolume,MinVolume, MaxVolume, 0.1f, _parent,tr("Volume ")+_parent->m_name)),
	pan(new FloatModel( PanningCenter, PanningLeft, PanningRight, 0.1f, _parent, tr( "Panning " )+_parent->m_name)),
	active( new BoolModel(true,_parent,tr("Active ")+_parent->m_name)),
	silenced(new BoolModel(false,_parent,tr("Silenced ")+_parent->m_name)),
	bare( new BoolModel(false,_parent,tr("Bare ")+_parent->m_name))
{
	//populates data from string
	parseString(_string);
}

void ChordSemiTone::saveSettings(QDomDocument &_doc, QDomElement &_parent)
{
	key->saveSettings(_doc,_parent,"key");
	vol->saveSettings(_doc,_parent,"vol");
	pan->saveSettings(_doc,_parent,"pan");
	active->saveSettings(_doc,_parent,"active");
	silenced->saveSettings(_doc,_parent,"silenced");
	bare->saveSettings(_doc,_parent,"bare");
}

void ChordSemiTone::loadSettings(const QDomElement &_this)
{
	key->loadSettings(_this,"key");
	vol->loadSettings(_this,"vol");
	pan->loadSettings(_this,"pan");
	active->loadSettings(_this,"active");
	silenced->loadSettings(_this,"silenced");
	bare->loadSettings(_this,"bare");
}

void ChordSemiTone::parseString(QString _string)
{
	QStringList l = _string.remove(' ').split(','); // trims and splits the string
	key->setValue(l[0].toInt());
	vol->setValue(l[1].toFloat());
	pan->setValue(l[2].toFloat());
	active->setValue(l[3].toShort());
	silenced->setValue(l[4].toShort());
	bare->setValue(l[5].toShort());
}

/*****************************************************************************************************
 *
 * The Chord class
 *
******************************************************************************************************/
Chord::Chord(Model *_parent) :
	Model (_parent),
	m_name("empty")
{
}

Chord::Chord(Model *_parent, QString _name, QString _string) :
	Model (_parent)
{
	m_name =_name;
	parseString(_string);
}

Chord::Chord(Model *_parent, QString _name) :
	Model (_parent)
{
	m_name =_name;
}


Chord::Chord(Chord *_copy,QString _name) :
	Model(_copy->parentModel())
{
	m_name = _name;
	ChordSemiTone *csm;
	for (int i=0;i<_copy->size();i++)
	{
		csm= new ChordSemiTone(_copy->at(i));
		push_back(csm);
	}
}

void Chord::saveSettings(QDomDocument &_doc, QDomElement &_parent)
{
	_parent.setAttribute( "name", m_name );
	ChordSemiTone *cst;
	for(int i=0;i<this->size();i++)
	{
		QDomElement semitone = _doc.createElement( QString( "semitone" ) );
		_parent.appendChild( semitone );
		cst=this->at(i);
		cst->saveSettings(_doc,semitone);
	}

}

//ZA VIDIT, TLE KAJ NE BO SLO
void Chord::loadSettings(const QDomElement &_this)
{
	//getting the first chordsemitone data
	m_name=_this.attribute("name");

	//the vector element counter
	int i=0;

	//the first child node
	QDomNode node = _this.firstChild();
	//the child element
	QDomElement semitone;

	//the chord to be read into
	ChordSemiTone *cst;
	while (!node.isNull())
	{
		semitone = node.toElement();
		//if the vector is empty creates new semitone, otherwise it uses the existing one.
		if (i<size())
		{
			cst=at(i);
			cst->loadSettings(semitone);
		} else
		{
			cst=new ChordSemiTone(this);
			cst->loadSettings(semitone);
			push_back(cst);
		}
		i++;
		node = node.nextSibling();
	}
	//removing the extra semitones if they are present
	if (i<size())
	{
		for (int j=size()-1;j>=i;j--)
		{
			remove(j);
		}
	}

}

bool Chord::hasSemiTone(int8_t semiTone) const
{
	for( int i = 0; i < size(); ++i )
	{
		if( semiTone == at(i)->key->value() )
		{
			return true;
		}
	}
	return false;
}

void Chord::addSemiTone()
{
	ChordSemiTone *csm=new ChordSemiTone(this);
	push_back(csm);
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::insertSemiTone(ChordSemiTone *csm, int position)
{
	insert(position,csm);
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::removeSemiTone(int i)
{
	remove(i);
	//emits the data changed signal
	Engine::chordTable()->emitChordTableChangedSignal();
}

void Chord::parseString(QString _string)
{
	//clears the vector;
	this->clear();
	//elaborates the input string
	QStringList l = _string.remove(' ').split(';');
	foreach (QString s, l)
	{
		if (s.isEmpty())
		{ // to eliminate the eventual empty QString derived from the last delimiter
			break;
		}
		// reads the data into semitone and pushes it into the vector
		ChordSemiTone *cst = new ChordSemiTone(this);
		push_back(cst);
	}
}

/*****************************************************************************************************
 *
 * The ChordTable class
 *
******************************************************************************************************/
ChordTable::ChordTable(Model *_parent) :
	Model( _parent )
{
	//reads the preset original data, emits the data changed signal
loadFactoryPreset();
//	readXML();
}

void ChordTable::saveSettings(QDomDocument &_doc, QDomElement &_parent)
{
	Chord *chord;
	for(int i=0;i<this->size();i++)
	{
		chord=this->at(i);
		QDomElement chord_element = _doc.createElement( QString( "chord" ) );
		_parent.appendChild( chord_element );
		chord->saveSettings(_doc,chord_element);
	}
}

void ChordTable::loadSettings(const QDomElement &_this)
{
	//clearing the vector
//	clear();

	int i=0;

	Chord *chord;

	//getting the first chordsemitone data
	QDomNode node = _this.firstChild();
	while (!node.isNull())
	{
		QDomElement chord_element = node.toElement();

		//if the vector is empty creates new chord, otherwise it uses the existing one.
		if (i<this->size())
		{
			chord=at(i);
			chord->loadSettings(chord_element);
		} else
		{
			chord=new Chord(this);
			chord->loadSettings(chord_element);
			push_back(chord);
		}
		i++;
		node = node.nextSibling();
	}
	//removing the extra chords if they are present
	if (i<size())
	{
		for (int j=size()-1;j>=i;j--)
		{
			remove(j);
		}
	}
	//emits the data changed signal
	emit chordTableChanged();
	//emits the chordTable names are changed
	emit chordNameChanged();

}

bool ChordTable::readXML()
{
	ConfigManager* confMgr = ConfigManager::inst();
	//Path to the presets file
	QString m_path= confMgr->factoryPresetsDir()+"ChordTable/arpeggio.xpf";

	//The xml document
	QDomDocument m_doc;

	//The xml file itself
	QFile file(m_path);

	//the lists of chords and keys
	QDomNodeList m_chords_list,m_key_list;

	//The xml chord and single key nodes
	QDomNode m_chords_node,m_keys_node,m_key_node;

	//The xml single element
	QDomElement m_chord_element,m_key_element;

	//placeholders for name and sngle key texts
	QString m_nameString,m_keyString;

	//placeholder for the chord semitone
	ChordSemiTone * m_semitone;

	//The single chord
	Chord  * m_chord;

	QString f=file.fileName();
	//Check for file
	if (!file.open(QIODevice::ReadOnly) || !m_doc.setContent(&file))
	{
		return false;
	}

	//Getting the list of chords available in the object
	m_chords_list = m_doc.elementsByTagName("chord");
	//for each row of chords
	for (int i = 0; i < m_chords_list.size(); i++)
	{

		//getting the single chord node
		m_chords_node = m_chords_list.item(i);
		//getting the "name" element
		m_chord_element = m_chords_node.firstChildElement("name");
		m_nameString=m_chord_element.text();
		//and the and the node representing the key sequence of the chord
		m_keys_node = m_chords_node.firstChildElement("keys");
		//getting the keys inside the element as a list
		m_key_list=m_keys_node.childNodes();

		//creating the new chord
		m_chord= new Chord(this, m_nameString);

		//processing the keys
		for (int i = 0; i < m_key_list.size(); i++)
		{
			//getting the single key node
			m_key_node =m_key_list.item(i);
			//and its element
			m_key_element=m_key_node.toElement();
			//and the string of the chord
			m_keyString=m_key_element.text();
			//initializing the single semitone from the key
			m_semitone=new ChordSemiTone(m_chord, m_keyString);
			//pushing it to the semitones vector
			m_chord->push_back(m_semitone);
		}

		//adding it to the chordtable structure
		push_back(m_chord);
	}
	return true;
}


void ChordTable::loadFactoryPreset()
{
	ConfigManager* confMgr = ConfigManager::inst();
	//Path to the presets file
	QString m_path= confMgr->factoryPresetsDir()+"ChordTable/ChordTable.ctd";

	DataFile dataFile( m_path );
	QDomElement content= dataFile.content();

	//already emits the data changed signal
	loadSettings( dataFile.content() );
}

void ChordTable::cloneChord(int i)
{
	Chord *cst;
	if (i==-1 || i>size())
	{ //adding a new Chord
		cst=new Chord(this);
		cst->m_name=tr("New Chord");
		cst->addSemiTone();
	} else
	{ //cloning the Chord
		cst=new Chord(at(i),at(i)->m_name+" " +tr("copy"));
	}
	push_back(cst);
	//emits the data changed signal
	emit chordTableChanged();
	//emits the chordTable names are changed
	emit chordNameChanged();
}

void ChordTable::removeChord(int i)
{
	//removes chord from the vector
	if (i<size())
	{
		remove(i);
		//emits the data changed signal
		emit chordTableChanged();
		//emits the chordTable names are changed
		emit chordNameChanged();
	}
}

const Chord &ChordTable::getByName(const QString &name, bool is_scale) const
{
	for( int i = 0; i < size(); i++ )
	{
		if( at( i )->getName() == name && is_scale == at( i )->isScale() )
			return *at( i );
	}

	static Chord empty(NULL);
	return empty;
}


//Initializes the ChordTable instance as null
ChordTable *ChordTable::instance=NULL;

ChordTable *ChordTable::getInstance(Model *_parent)
{
	if (instance==NULL){
		instance=new ChordTable(_parent);
	}
	return instance;
}

//loads the factory preset
void ChordTable::reset()
{
	//already emits the signal
//	clear();
//	readXML();
//	emit dataChanged();
	loadFactoryPreset();

}
