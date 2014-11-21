/*
 * MidiImport.cpp - support for importing MIDI files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomDocument>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressDialog>

#include "MidiImport.h"
#include "TrackContainer.h"
#include "InstrumentTrack.h"
#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "config_mgr.h"
#include "Pattern.h"
#include "Instrument.h"
#include "MainWindow.h"
#include "MidiTime.h"
#include "debug.h"
#include "embed.h"
#include "song.h"

#include "portsmf/allegro.h"

#define makeID(_c0, _c1, _c2, _c3) \
		( 0 | \
		( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) ) )



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiimport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"MIDI Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for importing MIDI-files into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	Plugin::ImportFilter,
	NULL,
	NULL,
	NULL
} ;

}


MidiImport::MidiImport( const QString & _file ) :
	ImportFilter( _file, &midiimport_plugin_descriptor ),
	m_events(),
	m_timingDivision( 0 )
{
}




MidiImport::~MidiImport()
{
}




bool MidiImport::tryImport( TrackContainer* tc )
{
	if( openFile() == false )
	{
		return false;
	}

#ifdef LMMS_HAVE_FLUIDSYNTH
	if( engine::hasGUI() &&
		configManager::inst()->defaultSoundfont().isEmpty() )
	{
		QMessageBox::information( engine::mainWindow(),
			tr( "Setup incomplete" ),
			tr( "You do not have set up a default soundfont in "
				"the settings dialog (Edit->Settings). "
				"Therefore no sound will be played back after "
				"importing this MIDI file. You should download "
				"a General MIDI soundfont, specify it in "
				"settings dialog and try again." ) );
	}
#else
	if( engine::hasGUI() )
	{
		QMessageBox::information( engine::mainWindow(),
			tr( "Setup incomplete" ),
			tr( "You did not compile LMMS with support for "
				"SoundFont2 player, which is used to add default "
				"sound to imported MIDI files. "
				"Therefore no sound will be played back after "
				"importing this MIDI file." ) );
	}
#endif

	switch( readID() )
	{
		case makeID( 'M', 'T', 'h', 'd' ):
			printf( "MidiImport::tryImport(): found MThd\n");
			return readSMF( tc );

		case makeID( 'R', 'I', 'F', 'F' ):
			printf( "MidiImport::tryImport(): found RIFF\n");
			return readRIFF( tc );

		default:
			printf( "MidiImport::tryImport(): not a Standard MIDI "
								"file\n" );
			return false;
	}
}




class smfMidiCC
{

public:
	smfMidiCC() :
		at( NULL ),
		ap( NULL ),
		lastPos( 0 )
	{ }
	
	AutomationTrack * at;
	AutomationPattern * ap;
	MidiTime lastPos;
	
	smfMidiCC & create( TrackContainer* tc )
	{
		if( !at )
		{
			at = dynamic_cast<AutomationTrack *>( track::create( track::AutomationTrack, tc ) );
		}
		return *this;
	}


	void clear()
	{
		at = NULL;
		ap = NULL;
		lastPos = 0;
	}


	smfMidiCC & putValue( MidiTime time, AutomatableModel * objModel, float value )
	{
		if( !ap || time > lastPos + DefaultTicksPerTact )
		{
			MidiTime pPos = MidiTime( time.getTact(), 0 );
			ap = dynamic_cast<AutomationPattern*>(
				at->createTCO(0) );
			ap->movePosition( pPos );
		}
		ap->addObject( objModel );

		lastPos = time;
		time = time - ap->startPosition();
		ap->putValue( time, value, false );
		ap->changeLength( MidiTime( time.getTact() + 1, 0 ) ); 

		return *this;
	}
};



class smfMidiChannel
{

public:
	smfMidiChannel() :
		it( NULL ),
		p( NULL ),
		it_inst( NULL ),
		isSF2( false ),
		hasNotes( false ),
		lastEnd( 0 )
	{ }
	
	InstrumentTrack * it;
	Pattern* p;
	Instrument * it_inst;
	bool isSF2; 
	bool hasNotes;
	MidiTime lastEnd;
	
	smfMidiChannel * create( TrackContainer* tc )
	{
		if( !it ) {
			it = dynamic_cast<InstrumentTrack *>( track::create( track::InstrumentTrack, tc ) );

#ifdef LMMS_HAVE_FLUIDSYNTH
			it_inst = it->loadInstrument( "sf2player" );
		
			if( it_inst )
			{
				isSF2 = true;
				it_inst->loadFile( configManager::inst()->defaultSoundfont() );
				it_inst->childModel( "bank" )->setValue( 0 );
				it_inst->childModel( "patch" )->setValue( 0 );
			}
			else
			{
				it_inst = it->loadInstrument( "patman" );
			}	
#else
			it_inst = it->loadInstrument( "patman" );
#endif
			
			lastEnd = 0;
		}
		return this;
	}


	void addNote( note & n )
	{
		if( !p || n.pos() > lastEnd + DefaultTicksPerTact )
		{
			MidiTime pPos = MidiTime( n.pos().getTact(), 0 );
			p = dynamic_cast<Pattern*>( it->createTCO( 0 ) );
			p->movePosition( pPos );
		}
		hasNotes = true;
		lastEnd = n.pos() + n.length();
		n.setPos( n.pos( p->startPosition() ) );
		p->addNote( n, false );
	}

};


bool MidiImport::readSMF( TrackContainer* tc )
{
	QString filename = file().fileName();
	closeFile();

	const int preTrackSteps = 2;
	QProgressDialog pd( TrackContainer::tr( "Importing MIDI-file..." ),
	TrackContainer::tr( "Cancel" ), 0, preTrackSteps, engine::mainWindow() );
	pd.setWindowTitle( TrackContainer::tr( "Please wait..." ) );
	pd.setWindowModality(Qt::WindowModal);
	pd.setMinimumDuration( 0 );

	pd.setValue( 0 );

	Alg_seq_ptr seq = new Alg_seq(filename.toLocal8Bit(), true);
	seq->convert_to_beats();

	pd.setMaximum( seq->tracks()  + preTrackSteps );
	pd.setValue( 1 );
	
	// 128 CC + Pitch Bend
	smfMidiCC ccs[129];
	smfMidiChannel chs[256];

	MeterModel & timeSigMM = engine::getSong()->getTimeSigModel();
	AutomationPattern * timeSigNumeratorPat = 
		AutomationPattern::globalAutomationPattern( &timeSigMM.numeratorModel() );
	AutomationPattern * timeSigDenominatorPat = 
		AutomationPattern::globalAutomationPattern( &timeSigMM.denominatorModel() );
	
	// TODO: adjust these to Time.Sig changes
	double beatsPerTact = 4; 
	double ticksPerBeat = DefaultTicksPerTact / beatsPerTact;

	// Time-sig changes
	Alg_time_sigs * timeSigs = &seq->time_sig;
	for( int s = 0; s < timeSigs->length(); ++s )
	{
		Alg_time_sig timeSig = (*timeSigs)[s];
		// Initial timeSig, set song-default value
		if(/* timeSig.beat == 0*/ true )
		{
			// TODO set song-global default value
			printf("Another timesig at %f\n", timeSig.beat);
			timeSigNumeratorPat->putValue( timeSig.beat*ticksPerBeat, timeSig.num );
			timeSigDenominatorPat->putValue( timeSig.beat*ticksPerBeat, timeSig.den );
		}
		else
		{
		}

	}

	pd.setValue( 2 );

	// Tempo stuff
	AutomationPattern * tap = tc->tempoAutomationPattern();
	if( tap )
	{
		tap->clear();
		Alg_time_map * timeMap = seq->get_time_map();
		Alg_beats & beats = timeMap->beats;
		for( int i = 0; i < beats.len - 1; i++ )
		{
			Alg_beat_ptr b = &(beats[i]);
			double tempo = ( beats[i + 1].beat - b->beat ) /
						   ( beats[i + 1].time - beats[i].time );
			tap->putValue( b->beat * ticksPerBeat, tempo * 60.0 );
		}
		if( timeMap->last_tempo_flag )
		{
			Alg_beat_ptr b = &( beats[beats.len - 1] );
			tap->putValue( b->beat * ticksPerBeat, timeMap->last_tempo * 60.0 );
		}
	}

	// Song events
	for( int e = 0; e < seq->length(); ++e )
	{
		Alg_event_ptr evt = (*seq)[e];

		if( evt->is_update() )
		{
			printf("Unhandled SONG update: %d %f %s\n", 
					evt->get_type_code(), evt->time, evt->get_attribute() );
		}
	}

	// Tracks
	for( int t = 0; t < seq->tracks(); ++t )
	{
		Alg_track_ptr trk = seq->track( t );
		pd.setValue( t + preTrackSteps );

		for( int c = 0; c < 129; c++ )
		{
			ccs[c].clear();
		}

		// Now look at events
		for( int e = 0; e < trk->length(); ++e )
		{
			Alg_event_ptr evt = (*trk)[e];

			if( evt->chan == -1 )
			{
				printf("MISSING GLOBAL THINGY\n");
				printf("     %d %d %f %s\n", (int) evt->chan, 
					evt->get_type_code(), evt->time,
							evt->get_attribute() );
				// Global stuff
			}
			else if( evt->is_note() && evt->chan < 256 )
			{
				smfMidiChannel * ch = chs[evt->chan].create( tc );
				Alg_note_ptr noteEvt = dynamic_cast<Alg_note_ptr>( evt );

				note n( noteEvt->get_duration() * ticksPerBeat,
						noteEvt->get_start_time() * ticksPerBeat,
						noteEvt->get_identifier() - 12,
						noteEvt->get_loud());
				ch->addNote( n );
				
			}
			
			else if( evt->is_update() )
			{
				smfMidiChannel * ch = chs[evt->chan].create( tc );

				double time = evt->time*ticksPerBeat;
				QString update( evt->get_attribute() );

				if( update == "programi" )
				{
					long prog = evt->get_integer_value();
					if( ch->isSF2 )
					{
						ch->it_inst->childModel( "bank" )->setValue( 0 );
						ch->it_inst->childModel( "patch" )->setValue( prog );
					}
					else {
						const QString num = QString::number( prog );
						const QString filter = QString().fill( '0', 3 - num.length() ) + num + "*.pat";
						const QString dir = "/usr/share/midi/"
								"freepats/Tone_000/";
						const QStringList files = QDir( dir ).
						entryList( QStringList( filter ) );
						if( ch->it_inst && !files.empty() )
						{
							ch->it_inst->loadFile( dir+files.front() );
						}
					}
				}
				else if( update == "tracknames" )
				{
					QString trackName( evt->get_string_value() );
					ch->it->setName( trackName );
					//ch.p->setName( trackName );
				}

				else if( update.startsWith( "control" ) || update == "bendr" )
				{
					int ccid = update.mid( 7, update.length()-8 ).toInt();
					if( update == "bendr" )
					{
						ccid = 128;
					}
					if( ccid <= 128 )
					{
						double cc = evt->get_real_value();
						AutomatableModel * objModel = NULL;

						switch( ccid ) 
						{
							case 0:
								if( ch->isSF2 && ch->it_inst )
								{
									objModel = ch->it_inst->childModel( "bank" );
									printf("BANK SELECT %f %d\n", cc, (int)(cc*127.0));
									cc *= 127.0f;
								}
								break;

							case 7:
								objModel = ch->it->volumeModel();
								cc *= 100.0f;
								break;

							case 10:
								objModel = ch->it->panningModel();
								cc = cc * 200.f - 100.0f;
								break;

							case 128:
								objModel = ch->it->pitchModel();
								cc = cc * 100.0f;
								break;
						}

						if( objModel )
						{
							if( time == 0 && objModel )
							{
								objModel->setInitValue( cc );
							}
							else
							{
								ccs[ccid].create( tc );
								ccs[ccid].putValue( time, objModel, cc );
							}
						}
					}
				}
				else {
					printf("Unhandled update: %d %d %f %s\n", (int) evt->chan, 
							evt->get_type_code(), evt->time, evt->get_attribute() );
				}
			}
		}
	}

	delete seq;
	
	
	for( int c=0; c < 256; ++c )
	{
		if( !chs[c].hasNotes && chs[c].it )
		{
			printf(" Should remove empty track\n");
			// must delete trackView first - but where is it?
			//tc->removeTrack( chs[c].it );
			//it->deleteLater();
		}
	}

	return true;
}




bool MidiImport::readRIFF( TrackContainer* tc )
{
	// skip file length
	skip( 4 );

	// check file type ("RMID" = RIFF MIDI)
	if( readID() != makeID( 'R', 'M', 'I', 'D' ) )
	{
invalid_format:
			qWarning( "MidiImport::readRIFF(): invalid file format" );
			return false;
	}

	// search for "data" chunk
	while( 1 )
	{
		const int id = readID();
		const int len = read32LE();
		if( file().atEnd() )
		{
data_not_found:
				qWarning( "MidiImport::readRIFF(): data chunk not found" );
				return false;
		}
		if( id == makeID( 'd', 'a', 't', 'a' ) )
		{
				break;
		}
		if( len < 0 )
		{
				goto data_not_found;
		}
		skip( ( len + 1 ) & ~1 );
	}

	// the "data" chunk must contain data in SMF format
	if( readID() != makeID( 'M', 'T', 'h', 'd' ) )
	{
		goto invalid_format;
	}
	return readSMF( tc );
}




void MidiImport::error()
{
	printf( "MidiImport::readTrack(): invalid MIDI data (offset %#x)\n",
						(unsigned int) file().pos() );
}



extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new MidiImport( QString::fromUtf8(
									static_cast<const char *>( _data ) ) );
}


}

#include "moc_MidiImport.cxx"
