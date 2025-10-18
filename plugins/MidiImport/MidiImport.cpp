/*
 * MidiImport.cpp - support for importing MIDI files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QDir>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>

#include <sstream>
#include <unordered_map>

#include "MidiImport.h"
#include "TrackContainer.h"
#include "InstrumentTrack.h"
#include "AutomationTrack.h"
#include "AutomationClip.h"
#include "ConfigManager.h"
#include "MidiClip.h"
#include "Instrument.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "TimePos.h"
#include "Song.h"

#include "plugin_export.h"

#include "portsmf/allegro.h"

namespace lmms
{


#define makeID(_c0, _c1, _c2, _c3) \
		( 0 | \
		( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) ) )



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiimport_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"MIDI Import",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Filter for importing MIDI-files into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	Plugin::Type::ImportFilter,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
} ;

}


MidiImport::MidiImport( const QString & _file ) :
	ImportFilter( _file, &midiimport_plugin_descriptor ),
	m_events(),
	m_timingDivision( 0 )
{
}




bool MidiImport::tryImport( TrackContainer* tc )
{
	if( openFile() == false )
	{
		return false;
	}

#ifdef LMMS_HAVE_FLUIDSYNTH
	if (gui::getGUI() != nullptr &&
		ConfigManager::inst()->sf2File().isEmpty() )
	{
		QMessageBox::information(gui::getGUI()->mainWindow(),
			tr( "Setup incomplete" ),
			tr( "You have not set up a default soundfont in "
				"the settings dialog (Edit->Settings). "
				"Therefore no sound will be played back after "
				"importing this MIDI file. You should download "
				"a General MIDI soundfont, specify it in "
				"settings dialog and try again." ) );
	}
#else
	if (gui::getGUI() != nullptr)
	{
		QMessageBox::information(gui::getGUI()->mainWindow(),
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
		at( nullptr ),
		ap( nullptr ),
		lastPos( 0 )
	{ }

	AutomationTrack * at;
	AutomationClip * ap;
	TimePos lastPos;

	smfMidiCC & create( TrackContainer* tc, QString tn )
	{
		if( !at )
		{
			// Keep LMMS responsive, for now the import runs
			// in the main thread. This should probably be
			// removed if that ever changes.
			qApp->processEvents();
			at = dynamic_cast<AutomationTrack *>( Track::create( Track::Type::Automation, tc ) );
		}
		if( tn != "") {
			at->setName( tn );
		}
		return *this;
	}


	void clear()
	{
		at = nullptr;
		ap = nullptr;
		lastPos = 0;
	}


	smfMidiCC & putValue( TimePos time, AutomatableModel * objModel, float value )
	{
		if( !ap || time > lastPos + DefaultTicksPerBar )
		{
			TimePos pPos = TimePos( time.getBar(), 0 );
			ap = dynamic_cast<AutomationClip*>(
				at->createClip(pPos));
			ap->addObject( objModel );
		}

		lastPos = time;
		time = time - ap->startPosition();
		ap->putValue( time, value, false );
		ap->changeLength( TimePos( time.getBar() + 1, 0 ) );

		return *this;
	}
};



class smfMidiChannel
{

public:
	smfMidiChannel() :
		it( nullptr ),
		p( nullptr ),
		it_inst( nullptr ),
		isSF2( false ),
		hasNotes( false )
	{ }

	InstrumentTrack * it;
	MidiClip* p;
	Instrument * it_inst;
	bool isSF2;
	bool hasNotes;
	QString trackName;

	smfMidiChannel * create( TrackContainer* tc, QString tn )
	{
		if( !it ) {
			// Keep LMMS responsive
			qApp->processEvents();
			it = dynamic_cast<InstrumentTrack *>( Track::create( Track::Type::Instrument, tc ) );

#ifdef LMMS_HAVE_FLUIDSYNTH
			it_inst = it->loadInstrument( "sf2player" );

			if( it_inst )
			{
				isSF2 = true;
				it_inst->loadFile( ConfigManager::inst()->sf2File() );
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
			trackName = tn;
			if( trackName != "") {
				it->setName( tn );
			}
			// General MIDI default
			it->pitchRangeModel()->setInitValue( 2 );

			// Create a default pattern
			p = dynamic_cast<MidiClip*>(it->createClip(0));
		}
		return this;
	}


	void addNote( Note & n )
	{
		if (!p)
		{
			p = dynamic_cast<MidiClip*>(it->createClip(0));
		}
		p->addNote(n, false);
		hasNotes = true;
	}

	void splitMidiClips()
	{
		MidiClip * newMidiClip = nullptr;
		TimePos lastEnd(0);

		p->rearrangeAllNotes();
		for (auto n : p->notes())
		{
			if (!newMidiClip || n->pos() > lastEnd + DefaultTicksPerBar)
			{
				TimePos pPos = TimePos(n->pos().getBar(), 0);
				newMidiClip = dynamic_cast<MidiClip*>(it->createClip(pPos));
			}
			lastEnd = n->pos() + n->length();

			Note newNote(*n);
			newNote.setPos(n->pos(newMidiClip->startPosition()));
			newMidiClip->addNote(newNote, false);
		}

		delete p;
		p = nullptr;
	}

};


bool MidiImport::readSMF( TrackContainer* tc )
{
	const int MIDI_CC_COUNT = 128 + 1; // 0-127 (128) + pitch bend
	const int preTrackSteps = 2;
	QProgressDialog pd( TrackContainer::tr( "Importing MIDI-file..." ),
	TrackContainer::tr("Cancel"), 0, preTrackSteps, gui::getGUI()->mainWindow());
	pd.setWindowTitle( TrackContainer::tr( "Please wait..." ) );
	pd.setWindowModality(Qt::WindowModal);
	pd.setMinimumDuration( 0 );

	pd.setValue( 0 );

	std::istringstream stream(readAllData().toStdString());
	auto seq = new Alg_seq(stream, true);
	seq->convert_to_beats();

	pd.setMaximum( seq->tracks()  + preTrackSteps );
	pd.setValue( 1 );

	// 128 CC + Pitch Bend
	auto ccs = std::array<smfMidiCC, MIDI_CC_COUNT>{};

	// channel to CC object for program changes
	std::unordered_map<long, smfMidiCC> pcs;

	// channels can be set out of 256 range
	// using unordered_map should fix most invalid loads and crashes while loading
	std::unordered_map<long, smfMidiChannel> chs;
	// NOTE: unordered_map::operator[] creates a new element if none exists

	MeterModel & timeSigMM = Engine::getSong()->getTimeSigModel();
	auto nt = dynamic_cast<AutomationTrack*>(Track::create(Track::Type::Automation, Engine::getSong()));
	nt->setName(tr("MIDI Time Signature Numerator"));
	auto dt = dynamic_cast<AutomationTrack*>(Track::create(Track::Type::Automation, Engine::getSong()));
	dt->setName(tr("MIDI Time Signature Denominator"));
	auto timeSigNumeratorPat = new AutomationClip(nt);
	timeSigNumeratorPat->setDisplayName(tr("Numerator"));
	timeSigNumeratorPat->addObject(&timeSigMM.numeratorModel());
	auto timeSigDenominatorPat = new AutomationClip(dt);
	timeSigDenominatorPat->setDisplayName(tr("Denominator"));
	timeSigDenominatorPat->addObject(&timeSigMM.denominatorModel());

	// TODO: adjust these to Time.Sig changes
	double beatsPerBar = 4;
	double ticksPerBeat = DefaultTicksPerBar / beatsPerBar;

	// Time-sig changes
	Alg_time_sigs * timeSigs = &seq->time_sig;
	for( int s = 0; s < timeSigs->length(); ++s )
	{
		Alg_time_sig timeSig = (*timeSigs)[s];
		timeSigNumeratorPat->putValue(timeSig.beat * ticksPerBeat, timeSig.num);
		timeSigDenominatorPat->putValue(timeSig.beat * ticksPerBeat, timeSig.den);
	}
	// manually call otherwise the pattern shows being 1 bar
	timeSigNumeratorPat->updateLength();
	timeSigDenominatorPat->updateLength();

	pd.setValue( 2 );

	// Tempo stuff
	auto tt = dynamic_cast<AutomationTrack*>(Track::create(Track::Type::Automation, Engine::getSong()));
	tt->setName(tr("Tempo"));
	auto tap = new AutomationClip(tt);
	tap->setDisplayName(tr("Tempo"));
	tap->addObject(&Engine::getSong()->tempoModel());
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

	// Update the tempo to avoid crash when playing a project imported
	// via the command line
	Engine::updateFramesPerTick();

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
		QString trackName = QString( tr( "Track" ) + " %1" ).arg( t );
		Alg_track_ptr trk = seq->track( t );
		pd.setValue( t + preTrackSteps );

		for (auto& cc : ccs)
		{
			cc.clear();
		}

		// Now look at events
		for( int e = 0; e < trk->length(); ++e )
		{
			Alg_event_ptr evt = (*trk)[e];

			if( evt->chan == -1 )
			{
				bool handled = false;
                if( evt->is_update() )
				{
					QString attr = evt->get_attribute();
					// seqnames is a track0 identifier (see allegro code)
					if (attr == (t == 0 ? "seqnames" : "tracknames")
						&& evt->get_update_type() == 's')
					{
						trackName = evt->get_string_value();
						handled = true;
					}
				}
                if( !handled ) {
                    // Write debug output
                    printf("MISSING GLOBAL HANDLER\n");
                    printf("     Chn: %d, Type Code: %d, Time: %f", (int) evt->chan,
                           evt->get_type_code(), evt->time );
                    if ( evt->is_update() )
                    {
                        printf( ", Update Type: %s", evt->get_attribute() );
                        if ( evt->get_update_type() == 'a' )
                        {
                            printf( ", Atom: %s", evt->get_atom_value() );
                        }
                    }
                    printf( "\n" );
				}
			}
			else if (evt->is_note())
			{
				smfMidiChannel * ch = chs[evt->chan].create( tc, trackName );
				auto noteEvt = dynamic_cast<Alg_note_ptr>(evt);
				int ticks = noteEvt->get_duration() * ticksPerBeat;
				Note n( (ticks < 1 ? 1 : ticks ),
						noteEvt->get_start_time() * ticksPerBeat,
						noteEvt->get_identifier(),
						noteEvt->get_loud() * (200.f / 127.f)); // Map from MIDI velocity to LMMS volume
				ch->addNote( n );

			}

			else if( evt->is_update() )
			{
				smfMidiChannel * ch = chs[evt->chan].create( tc, trackName );

				double time = evt->time*ticksPerBeat;
				QString update( evt->get_attribute() );

				if( update == "programi" )
				{
					long prog = evt->get_integer_value();
					if( ch->isSF2 )
					{
						auto& pc = pcs[evt->chan];
						AutomatableModel* objModel = ch->it_inst->childModel("patch");
						if (pc.at == nullptr) {
							pc.create(tc, trackName + " > " + objModel->displayName());
						}
						pc.putValue(time, objModel, prog);
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
						AutomatableModel * objModel = nullptr;

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
							default:
								//TODO: something useful for other CCs
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
								if( ccs[ccid].at == nullptr ) {
									ccs[ccid].create( tc, trackName + " > " + (
										  objModel != nullptr ?
										  objModel->displayName() :
										  QString("CC %1").arg(ccid) ) );
								}
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


	for( auto& c: chs )
	{
		if (c.second.hasNotes)
		{
			c.second.splitMidiClips();
		}
		else if (c.second.it)
		{
			printf(" Should remove empty track\n");
			// must delete trackView first - but where is it?
			//tc->removeTrack( chs[c].it );
			//it->deleteLater();
		}
		// Set channel 10 to drums as per General MIDI's orders
		if (c.first % 16l == 9 /* channel 10 */
			&& c.second.hasNotes && c.second.it_inst && c.second.isSF2)
		{
			c.second.it_inst->childModel("bank")->setValue(128);
			c.second.it_inst->childModel("patch")->setValue(0);
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
	while( true )
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
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * _data )
{
	return new MidiImport( QString::fromUtf8(
									static_cast<const char *>( _data ) ) );
}


}


} // namespace lmms
