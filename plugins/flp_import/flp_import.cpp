/*
 * flp_import.cpp - support for importing FLP-files
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <Qt/QtXml>
#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>
#include <QtCore/QDir>
#include <QtCore/QBuffer>

#include "flp_import.h"
#include "basic_filters.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "combobox.h"
#include "config_mgr.h"
#include "debug.h"
#include "engine.h"
#include "group_box.h"
#include "instrument.h"
#include "instrument_track.h"
#include "envelope_and_lfo_parameters.h"
#include "knob.h"
#include "oscillator.h"
#include "pattern.h"
#include "piano.h"
#include "project_journal.h"
#include "project_notes.h"
#include "song.h"
#include "tempo_sync_knob.h"
#include "track_container.h"
#include "embed.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#define makeID(_c0, _c1, _c2, _c3) \
	( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) )


extern "C"
{

plugin::descriptor PLUGIN_EXPORT flpimport_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"FLP Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Filter for importing FL Studio projects into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	plugin::ImportFilter,
	NULL,
	NULL
} ;



// unrtf-stuff
#include "defs.h"
#include "main.h"
#include "html.h"
#include "word.h"
#include "hash.h"
#include "convert.h"
#include "attr.h"


OutputPersonality * op = NULL;
int lineno = 0;
#define inline_mode 0
#define debug_mode 0
#define nopict_mode 1
#define verbose_mode 0
#define simple_mode 0

extern QString outstring;

}


static void dump_mem( const void * buffer, uint n_bytes )
{
	uchar * cp = (uchar *) buffer;
	for( uint k = 0; k < n_bytes; ++k )
	{
		printf( "%02x ", (unsigned int)cp[k] );//( cp[k] > 31 || cp[k] < 7 ) ? cp[k] : '.' );
	}
	printf( "\n" );
}



flpImport::flpImport( const QString & _file ) :
	importFilter( _file, &flpimport_plugin_descriptor )
{
}




flpImport::~flpImport()
{
}




bool flpImport::tryImport( trackContainer * _tc )
{
	if( openFile() == FALSE )
	{
		return( FALSE );
	}

	if( readID() != makeID( 'F', 'L', 'h', 'd' ) )
	{
		printf( "flpImport::tryImport(): not a valid FL project\n" );
		return( FALSE );
	}

	const int header_len = read32LE();
	if( header_len != 6 )
	{
		printf( "flpImport::tryImport(): invalid file format\n" );
		return( FALSE );
	}

	const int type = read16LE();
	if( type != 0 )
	{
		printf( "flpImport::tryImport(): type %d format is not "
							"supported\n", type );
		return( FALSE );
	}

	const int num_channels = read16LE();
	if( num_channels < 1 || num_channels > 1000 )
	{
		printf( "flpImport::tryImport(): invalid number of channels "
						"(%d)\n", num_channels );
		return( FALSE );
	}

	const int ppq = read16LE();
	if( ppq < 0 )
	{
		printf( "flpImport::tryImport(): invalid ppq\n" );
		return( FALSE );
	}

	QProgressDialog pd( trackContainer::tr( "Importing FLP-file..." ),
			trackContainer::tr( "Cancel" ), 0, num_channels );
	pd.setWindowTitle( trackContainer::tr( "Please wait..." ) );
	pd.show();

	bool valid = FALSE;

	// search for FLdt chunk
	while( 1 )
	{
		Sint32 id = readID();
		const int len = read32LE();
		if( file().atEnd() )
		{
			printf( "flpImport::tryImport(): unexpected "
					"end of file\n" );
			return( FALSE );
		}
		if( len < 0 || len >= 0x10000000 )
		{
			printf( "flpImport::tryImport(): invalid "
					"chunk length %d\n", len );
			return( FALSE );
		}
		if( id == makeID( 'F', 'L', 'd', 't' ) )
		{
			valid = TRUE;
			break;
		}
		skip( len );
	}

	if( valid == FALSE )
	{
		return( FALSE );
	}

	printf( "channels: %d\n", num_channels );

	instrumentTrack * it = NULL;
	instrument * it_inst = NULL;
	flPlugins cur_plugin = FL_Plugin_Undef;

	int current_pattern = 0;
	char * text = NULL;
	Uint32 text_len = 0;
	QList<instrumentTrack *> i_tracks;
	int project_cur_pat = 0;

	int step_pattern = 0;
	int last_step_pos = 0;
	
	int env_lfo_target = 0;

	int ev_cnt = 0;

	engine::getSong()->clearProject();
	const bool is_journ = engine::getProjectJournal()->isJournalling();
	engine::getProjectJournal()->setJournalling( FALSE );


	while( file().atEnd() == FALSE )
	{
		if( ++ev_cnt > 100 )
		{
			ev_cnt = 0;
			qApp->processEvents( QEventLoop::AllEvents, 100 );
			pd.setValue( i_tracks.size() );

			if( pd.wasCanceled() )
			{
				return( FALSE );
			}
		}

		flpEvents ev = static_cast<flpEvents>( readByte() );
		Uint32 data = readByte();

		if( ev >= FLP_Word && ev < FLP_Text )
		{
			data = data | ( readByte() << 8 );
		}

		if( ev >= FLP_Int && ev < FLP_Text )
		{
			data = data | ( readByte() << 16 );
			data = data | ( readByte() << 24 );
		}


		if( ev >= FLP_Text )
		{
			text_len = data & 0x7F;
			Uint8 shift = 0;
			while( data & 0x80 )
			{
				data = readByte();
				text_len = text_len | ( ( data & 0x7F ) <<
							( shift += 7 ) );
			}

			delete[] text;
			text = new char[text_len+1];
			if( readBlock( text, text_len ) <= 0 )
			{
				printf( "could not read string (len: %d)\n",
						text_len );
			}

			text[text_len] = 0;
		}

		switch( ev )
		{
			// BYTE EVENTS
			case FLP_Byte:
				printf( "undefined byte %d\n", data );
				break;

			case FLP_NoteOn:
				printf( "note on: %d\n", data );
				// data = pos   how to handle?
				break;

			case FLP_LoopActive:
				printf( "active loop: %d\n", data );
				break;

			case FLP_ShowInfo:
				printf( "show info: %d\n", data );
				break;

			case FLP_Shuffle:
				printf( "shuffle: %d\n", data );
				break;

			case FLP_MainVol:
				printf( "main-volume: %d\n", data );
				engine::getSong()->setMasterVolume(
					static_cast<volume>( data * 100 /
									128 ) );
				break;

			case FLP_PatLength:
				printf( "pattern-length: %d\n", data );
				break;

			case FLP_BlockLength:
				printf( "block length: %d\n", data );
				break;

			case FLP_LoopType:
				printf( "loop type: %d\n", data );
				break;

			case FLP_ChanType:
				printf( "channel type: %d\n", data );
				break;

			case FLP_MixSliceNum:
				printf( "mix slice num: %d\n", data );
				break;


			// WORD EVENTS
			case FLP_NewChan:
				printf( "new channel\n" );

				it = dynamic_cast<instrumentTrack *>(
	track::create( track::InstrumentTrack, engine::getBBTrackContainer() ) );
				assert( it != NULL );
				i_tracks.push_back( it );
				it_inst = it->loadInstrument(
							"audiofileprocessor" );
//				it->toggledInstrumentTrackButton( FALSE );

				// reset some values
				step_pattern = 0;
				last_step_pos = 0;
				env_lfo_target = 0;

				break;

			case FLP_NewPat:
				current_pattern = data - 1;
				break;

			case FLP_Tempo:
				printf( "tempo: %d\n", data );
				engine::getSong()->setTempo( data );
				break;

			case FLP_CurrentPatNum:
				project_cur_pat = data;
				break;

			case FLP_FX:
				printf( "FX-channel for cur channel: %d\n",
									data );
				break;

			case FLP_Fade_Stereo:
				printf( "fade stereo: %d\n", data );
				break;

			case FLP_CutOff:
				printf( "cutoff (sample): %d\n", data );
				break;

			case FLP_PreAmp:
				printf( "pre-amp (sample): %d\n", data );
				break;

			case FLP_Decay:
				printf( "decay (sample): %d\n", data );
				break;

			case FLP_Attack:
				printf( "attack (sample): %d\n", data );
				break;

			case FLP_MainPitch:
				engine::getSong()->setMasterPitch( data );
				break;

			case FLP_Resonance:
				printf( "reso (sample): %d\n", data );
				break;

			case FLP_LoopBar:
				printf( "loop bar: %d\n", data );
				break;

			case FLP_StDel:
				printf( "stdel (delay?): %d\n", data );
				break;

			case FLP_FX3:
				printf( "FX 3: %d\n", data );
				break;

			case FLP_ShiftDelay:
				printf( "shift delay: %d\n", data );
				break;

			case FLP_Dot:
			{
				printf( "dot/step at: %d\n", data );
				const int pos = data & 0xff;
				if( last_step_pos > pos )
				{
					++step_pattern;
				}
				last_step_pos = pos;
				m_steps.push_back(
					( ( i_tracks.size() - 1 ) << 16 ) |
					( pos + 16 * step_pattern ) );
				break;
			}

			// DWORD EVENTS
			case FLP_Color:
				printf( "color  r:%d  g:%d  b:%d\n",
						qRed( data ),
						qGreen( data ),
						qBlue( data ) );
				break;

			case FLP_PlayListItem:
			{
				m_plItems.push_back( data );
				break;
			}

			case FLP_FXSine:
				printf( "fx sine: %d\n", data );
				break;

			case FLP_CutCutBy:
				printf( "cut cut by: %d\n", data );
				break;

			case FLP_MiddleNote:
				data += 8;
				it->baseNoteModel()->setValue( data );
				break;

			case FLP_DelayReso:
				printf( "delay resonance: %d\n", data );
				break;

			case FLP_Reverb:
				printf( "reverb (sample): %d\n", data );
				break;

			case FLP_IntStretch:
				printf( "int stretch (sample): %d\n", data );
				break;

			// TEXT EVENTS
			case FLP_Text_ChanName:
				if( it == NULL )
				{
					printf( "!! tried to set channel name "
						"but no channel was created so "
						"far\n" );
					break;
				}
				it->setName( text );
				break;

			case FLP_Text_CommentRTF:
			{
				QByteArray ba( text, text_len );
				QBuffer buf( &ba );
				buf.open( QBuffer::ReadOnly );
				lineno = 0;
				attr_clear_all();
				op = html_init();
				hash_init();
				Word * word = word_read( &buf );
				QString out;
				word_print( word, out );
				word_free( word );
				op_free( op );

				engine::getProjectNotes()->setText( out );
				outstring = "";
				break;
			}

			case FLP_Text_Title:
				printf( "project title: %s\n", text );
				break;

			case FLP_Text_SampleFileName:
			{
				if( it_inst == NULL )
				{
					printf( "!! tried to set sample for "
						"instrument but no channel or "
						"instrument was "
						"created so far\n" );
					break;
				}
				QString dir = text;
/*				if( dir.mid( 1, 11 ) == "Instruments" )
				{
					dir = "\\Patches\\Packs" +
								dir.mid( 12 );
				}*/
				dir.replace( '\\', QDir::separator() );
				if( QFileInfo( configManager::inst()->flDir() +
						"/Data/" ).exists() )
				{
					dir = configManager::inst()->flDir() +
								"/Data/" + dir;
				}
				else
				{
					// FL 3 compat
					dir = configManager::inst()->flDir() +
							"/Samples/" + dir;
				}
				it_inst->setParameter( "samplefile", dir );
				break;
			}

			case FLP_Version:
				printf( "FL Version: %s\n", text );
				break;

			case FLP_Text_PluginName:
			{
				instrument * new_inst = NULL;
				if( QString( text ) == "3x Osc" )
				{
					printf( "loading TripleOscillator for "
								"3x Osc\n" );
					new_inst = it->loadInstrument(
							"tripleoscillator" );
					cur_plugin = FL_Plugin_3x_Osc;
				}
				else if( QString( text ) == "Plucked!" )
				{
					printf( "loading Vibed for Plucked!\n"
									);
					new_inst = it->loadInstrument(
							"vibedstrings" );
					cur_plugin = FL_Plugin_Plucked;
				}
				else
				{
					printf( "unsupported plugin: %s!\n",
									text );
				}
				if( new_inst != NULL )
				{
					it_inst = new_inst;
				}
				break;
			}

			case FLP_Delay:
				printf( "delay-data:\n" );
				dump_mem( text, text_len );
				break;

			case FLP_NewPlugin:
				printf( "new plugin: %s\n", text );
				break;

			case FLP_PluginParams:
				if( it_inst != NULL &&
						cur_plugin != FL_Plugin_Undef )
				{
					processPluginParams( cur_plugin,
							(const char *) text,
							text_len, it_inst );
					cur_plugin = FL_Plugin_Undef;
				}
				break;

			case FLP_ChanParams:
			{
				const arpeggiator::ArpDirections
					mappedArpDir[] =
				{
					arpeggiator::ArpDirUp,
					arpeggiator::ArpDirUp,
					arpeggiator::ArpDirDown,
					arpeggiator::ArpDirUpAndDown,
					arpeggiator::ArpDirUpAndDown,
					arpeggiator::ArpDirRandom
				} ;
				const Uint32 * p = (const Uint32 *) text;
				arpeggiator * arp = &it->m_arpeggiator;
				arp->m_arpDirectionModel.setValue(
							mappedArpDir[p[10]] );
				arp->m_arpRangeModel.setValue( p[11] );
				arp->m_arpModel.setValue( p[12] );
				arp->m_arpTimeModel.setValue( p[13] / 8.0f );
							////	100.0f );
				arp->m_arpGateModel.setValue( p[14] * 100.0f /
									48.0f );
				arp->m_arpEnabledModel.setValue( p[10] > 0 );
				printf( "channel params: " );
				dump_mem( text, text_len );
				//printf( "channel params: arpdir: %d  range: %d  time: %d  gate: %d\n", p[10], p[11], p[13], p[14] );
				break;
			}

			case FLP_EnvLfoParams:
			{
				const Uint32 * p = (const Uint32 *) text;
				printf( "envelope and lfo params: " );
				dump_mem( text, text_len );
				instrumentSoundShaping * iss = &it->m_soundShaping;
				envelopeAndLFOParameters * elp = NULL;
				switch( env_lfo_target )
				{
					case 1:
		elp = iss->m_envLFOParameters[instrumentSoundShaping::Volume];
		break;
					case 2:
		elp = iss->m_envLFOParameters[instrumentSoundShaping::Cut];
		break;
					case 3:
		elp = iss->m_envLFOParameters[instrumentSoundShaping::Resonance];
		break;
					default:
						break;
				}
				++env_lfo_target;
				if( elp == NULL )
				{
					break;
				}
				const float scaling = 0.8f / 65536.0f;
				elp->m_predelayModel.setValue( p[2] * scaling );
				elp->m_attackModel.setValue( p[3] * scaling );
				elp->m_holdModel.setValue( p[4] * scaling );
				elp->m_decayModel.setValue( p[5] * scaling );
				elp->m_sustainModel.setValue( p[6] * scaling );
				elp->m_releaseModel.setValue( p[7] * scaling );
				elp->m_amountModel.setValue( p[1] ? 1 : 0 );
				elp->updateSampleVars();
				break;
			}

			case FLP_FilterParams:
			{
				// TODO: Dirty hack!
				// SIMPLE_FLT_CNT equals to old DOUBLE_LOWPASS
				const basicFilters<>::filterTypes
								mappedFilter[] =
				{
					basicFilters<>::LOWPASS,// fast LP
					basicFilters<>::LOWPASS,
					basicFilters<>::BANDPASS_CSG,
					basicFilters<>::HIPASS,
					basicFilters<>::NOTCH,
					basicFilters<>::SIMPLE_FLT_CNT,
					basicFilters<>::LOWPASS,
					basicFilters<>::SIMPLE_FLT_CNT
				} ;
				Uint32 * p = (Uint32 *) text;
				instrumentSoundShaping * iss = &it->m_soundShaping;
/*				printf( "filter values: " );
				for( int i = 0; i < 6; ++i )
				{
					printf( "%d  ", ((Sint32*)text)[i] );
				}
				printf( "\n" );*/
				iss->m_filterModel.setValue(
							mappedFilter[p[5]] );
				iss->m_filterCutModel.setValue( p[3] / 255.0f *
					( iss->m_filterCutModel.maxValue() -
					iss->m_filterCutModel.minValue() ) +
					iss->m_filterCutModel.minValue() );
				iss->m_filterResModel.setValue( p[4] / 1024.0f *
					( iss->m_filterResModel.maxValue() -
					iss->m_filterResModel.minValue() ) +
					iss->m_filterResModel.minValue() );
				iss->m_filterEnabledModel.setValue( p[3] < 256 );
				break;
			}

			case FLP_PatternNotes:
			{
				//dump_mem( text, text_len );
				const int bpn = 20;
				Uint32 imax = ( text_len + bpn - 1 ) / bpn;
				for( Uint32 i = 0; i < imax; ++i )
				{
					int ch = *( text + i*bpn + 6 );
					int pos = *( (int *)( text + i*bpn ) );
					int key = *( text + i*bpn + 12 );
					int len = *( (int*)( text + i*bpn +
									8 ) );
					pos /= 384 / DefaultTicksPerTact;
					len /= 384 / DefaultTicksPerTact;
					note n( len, pos );
					n.setKey( key );
					m_notes.push_back( qMakePair(
				num_channels * current_pattern + ch, n ) );
				
					//printf( "note on channel %d at pos %d with key %d and length %d   ", (int)*((text+i*bpn+6)), *((int*)(text+i*bpc)), (int) *(text+i*bpc+12), (int) *((int*)(text+i*bpc+8)));
					//printf( "note on channel %d at pos %d with key %d and length %d\n", ch, pos, key, len );
					
					//dump_mem( text+i*bpn+4, bpc-4 );
				}
				break;
			}

			case FLP_StepData:
			{
				printf( "step data:\n" );
				dump_mem( text, text_len );
#if 0
				const int bpn = 20;
				for( int i = 0; i*bpn < text_len; ++i )
				{
					int ch = *( text + i*bpn + 6 );
					int pos = *( (int *)( text + i*bpn ) );
					int key = *( text + i*bpn + 12 );
					int len = *( (int*)( text + i*bpn +
									8 ) );
					pos /= 512 / DefaultTicksPerTact;
					len /= 512 / DefaultTicksPerTact;
					/*note n( NULL, len, pos );
					n.setKey( key );
					m_notes.push_back( qMakePair(
				num_channels * current_pattern + ch, n ) );*/
				
					//printf( "note on channel %d at pos %d with key %d and length %d   ", (int)*((text+i*bpn+6)), *((int*)(text+i*bpc)), (int) *(text+i*bpc+12), (int) *((int*)(text+i*bpc+8)));
					//printf( "step-note (?) on channel %d at pos %d with key %d and length %d\n", ch, pos, key, len );
					
					dump_mem( text+i*bpn, bpn );
				}
#endif
				break;
			}

			default:
				if( ev >= FLP_Text )
				{
					printf( "!! unhandled text (ev: %d, "
							"len: %d):\n",
							ev, text_len );
					dump_mem( text, text_len );
				}
				else
				{
					printf( "!! handling of FLP-event %d "
						"not implemented yet "
						"(data=%d).\n", ev, data );
				}
				break;
		}
        }

	// process all steps
	for( stepVector::const_iterator it = m_steps.begin();
						it != m_steps.end(); ++it )
	{
		const int ch = ( *it ) >> 16;
		const int pat = ( ( *it ) & 0xffff ) / 16;
		const int pos = ( ( ( *it ) & 0xffff ) % 16 ) *
						(DefaultTicksPerTact/16);
		while( engine::getBBTrackContainer()->numOfBBs() <= pat )
		{
			track::create( track::BBTrack, engine::getSong() );
		}
		pattern * p = dynamic_cast<pattern *>(
						i_tracks[ch]->getTCO( pat ) );
		if( p == NULL )
		{
			continue;
		}
		p->addNote( note( -DefaultTicksPerTact, pos ), FALSE );
	}

	// now process all notes
	for( patternNoteVector::const_iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		const int where = ( *it ).first;
		const int ch = where % num_channels;
		const int pat = where / num_channels;
		if( pat > 100 )
		{
			continue;
		}
		while( engine::getBBTrackContainer()->numOfBBs() <= pat )
		{
			track::create( track::BBTrack, engine::getSong() );
			qApp->processEvents( QEventLoop::AllEvents, 100 );
		}
		pattern * p = dynamic_cast<pattern *>(
						i_tracks[ch]->getTCO( pat ) );
		if( p != NULL )
		{
			p->addNote( ( *it ).second, FALSE );
		}
	}

	// process all playlist-items
	for( playListItems::const_iterator it = m_plItems.begin();
						it != m_plItems.end(); ++it )
	{
		int pat_num = ( ( *it ) >> 16 ) - 1;
		if( pat_num > 100 )
		{
			continue;
		}
		while( engine::getBBTrackContainer()->numOfBBs() <= pat_num )
		{
			track::create( track::BBTrack, engine::getSong() );
			qApp->processEvents( QEventLoop::AllEvents, 100 );
		}
		
		bbTrack * bbt = bbTrack::findBBTrack( pat_num );
		trackContentObject * tco = bbt->addTCO( bbt->createTCO( 0 ) );
		tco->movePosition( midiTime( ( ( *it ) & 0xffff ) *
							DefaultTicksPerTact ) );
	}

	if( project_cur_pat < engine::getBBTrackContainer()->numOfBBs() )
	{
		engine::getBBTrackContainer()->setCurrentBB( project_cur_pat );
	}

	engine::getProjectJournal()->setJournalling( is_journ );
        return( TRUE );
}




bool flpImport::processPluginParams( const flPlugins _plugin,
					const char * _data,
					const int _data_len, instrument * _i )
{
	printf( "plugin params for plugin %d (%d bytes): ", _plugin,
								_data_len );
	dump_mem( _data, _data_len );
	switch( _plugin )
	{
		case FL_Plugin_3x_Osc:
		{
			const oscillator::WaveShapes mapped_3xOsc_Shapes[] =
			{
				oscillator::SineWave,
				oscillator::TriangleWave,
				oscillator::SquareWave,
				oscillator::SawWave,
				oscillator::SquareWave,	// square-sin
				oscillator::WhiteNoise,
				oscillator::UserDefinedWave
			} ;

			QDomDocument dd;
			QDomElement de = dd.createElement( _i->nodeName() );
			de.setAttribute( "modalgo1", oscillator::SignalMix );
			de.setAttribute( "modalgo2", oscillator::SignalMix );
			for( Uint8 i = 0; i < 3; ++i )
			{
				const Sint32 * d = (const Sint32 *)( _data +
								i * 28 );
				QString is = QString::number( i );
				de.setAttribute( "vol" + is,
					QString::number( d[0] * 100 / 128 ) );
				de.setAttribute( "pan" + is,
						QString::number( d[1] ) );
				de.setAttribute( "coarse" + is,
						QString::number( d[3] ) );
				de.setAttribute( "finel" + is,
					QString::number( d[4] - d[6] / 2 ) );
				de.setAttribute( "finer" + is,
					QString::number( d[4] + d[6] / 2 ) );
				de.setAttribute( "stphdetun" + is,
						QString::number( d[5] ) );
				de.setAttribute( "wavetype" + is,
					QString::number(
						mapped_3xOsc_Shapes[d[2]] ) );
			}
			de.setAttribute( "vol0", QString::number( 100 ) );
			// now apply the prepared plugin-state
			_i->restoreState( de );
			break;
		}

		case FL_Plugin_Plucked:
		{
			// TODO: setup vibed-instrument
			break;
		}

		default:
			printf( "handling of plugin params not implemented "
					"for current plugin\n" );
			break;
	}
	return( TRUE );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model *, void * _data )
{
	return( new flpImport( static_cast<const char *>( _data ) ) );
}


// include unrtf-source
#include "attr.c"
#include "convert.c"
#include "error.c"
#include "hash.c"
#include "html.c"
#include "malloc.c"
#include "output.c"
#include "parse.c"
#include "util.c"
#include "word.c"

}


#undef setValue
