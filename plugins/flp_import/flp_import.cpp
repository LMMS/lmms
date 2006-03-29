/*
 * flp_import.cpp - support for importing FLP-files
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "flp_import.h"
#include "project_notes.h"
#include "bb_editor.h"
#include "song_editor.h"
#include "bb_track.h"
#include "track_container.h"
#include "instrument_track.h"
#include "pattern.h"


#ifdef QT4

#include <Qt/QtXml>
#include <QApplication>
#include <QProgressDialog>

#else

#include <qdom.h>
#include <qapplication.h>
#include <qprogressdialog.h>

#define pos at
#define setValue setProgress

#endif

#define makeID(_c0, _c1, _c2, _c3) \
	( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) )


extern "C"
{

plugin::descriptor flpimport_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"FLP Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
			"Filter for importing FL Studio projects into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	plugin::IMPORT_FILTER,
	new QPixmap()
} ;

}


void dump_mem( const void * buffer, uint n_bytes )
{
	uchar * cp = (uchar *) buffer;
	for( uint k = 0; k < n_bytes; ++k )
	{
		printf( "%02x ", ( cp[k] > 31 || cp[k] < 7 ) ? cp[k] : '.' );
	}
	printf( "\n" );
}



flpImport::flpImport( const QString & _file ) :
	importFilter( _file, &flpimport_plugin_descriptor, NULL )
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

	printf( "channels: %d\n", num_channels );

	const int ppq = read16LE();
	if( ppq < 0 )
	{
		printf( "flpImport::tryImport(): invalid ppq\n" );
		return( FALSE );
	}

#ifdef QT4
	QProgressDialog pd( trackContainer::tr( "Importing FLP-file..." ),
			trackContainer::tr( "Cancel" ), 0, num_channels );
#else
	QProgressDialog pd( trackContainer::tr( "Importing FLP-file..." ),
				trackContainer::tr( "Cancel" ), num_channels,
								0, 0, TRUE );
#endif
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

	instrumentTrack * it = NULL;
	pattern * p = NULL;
	char * text = NULL;
	int text_len = 0;
	int it_cnt = 0;

	int ev_cnt = 0;

	_tc->eng()->getSongEditor()->clearProject();

	while( file().atEnd() == FALSE )
	{
		if( ++ev_cnt > 100 )
		{
			ev_cnt = 0;
#ifdef QT4
			qApp->processEvents( QEventLoop::AllEvents, 100 );
#else
			qApp->processEvents( 100 );
#endif
			pd.setValue( it_cnt );

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
			while( data & 0x80 )
			{
				data = readByte();
				text_len = ( text_len << 7 ) | ( data & 0x7F );
			}
			delete[] text;
			text = new char[text_len+1];
			if( readBlock( text, text_len ) <= 0 )
			{
				printf( "could not read string\n" );
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

			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 36:
				printf( "ev: %d data: %d\n", ev, data );
				break;

			// WORD EVENTS
			case FLP_NewChan:
				printf( "new channel\n" );
				++it_cnt;
				m_events.clear();

				it = dynamic_cast<instrumentTrack *>(
	track::create( track::CHANNEL_TRACK, _tc->eng()->getBBEditor() ) );
				assert( it != NULL );
				it->loadInstrument( "tripleoscillator" );
				it->toggledInstrumentTrackButton( FALSE );

				break;

			case FLP_NewPat:
				while( _tc->eng()->getBBEditor()->numOfBBs() <=
								data )
				{
					track::create( track::BB_TRACK,
						_tc->eng()->getSongEditor() );
				}
				p = dynamic_cast<pattern *>(
						it->getTCO( data - 1 ) );
				break;

			case FLP_Tempo:
				printf( "tempo: %d\n", data );
				break;

			case FLP_CurrentPatNum:
				printf( "current pattern: %d\n", data );
				break;

			case FLP_FX:
				printf( "FX-channel for cur channel: %d\n",
									data );
				break;

			case FLP_Fade_Stereo:
				printf( "fade stereo: %d\n", data );
				break;

			case FLP_CutOff:
				printf( "cutoff (for cur channel?): %d\n",
									data );
				break;

			case FLP_PreAmp:
				printf( "pre-amp (for cur channel?): %d\n",
									data );
				break;

			case FLP_Decay:
				printf( "decay (for cur channel?): %d\n",
									data );
				break;

			case FLP_Attack:
				printf( "attack (for cur channel?): %d\n",
									data );
				break;

			case FLP_MainPitch:
				printf( "main-pitch: %d\n", data );
				break;

			case FLP_Resonance:
				printf( "reso (for cur channel?): %d\n", data );
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

			// DWORD EVENTS
			case FLP_Color:
				printf( "color  r:%d  g:%d  b:%d\n",
						qRed( data ),
						qGreen( data ),
						qBlue( data ) );
				break;

			case FLP_PlayListItem:
			{
				unsigned int pat_num = ( data >> 16 ) - 1;
				while( _tc->eng()->getBBEditor()->numOfBBs() <=
								pat_num )
				{
					track::create( track::BB_TRACK,
						_tc->eng()->getSongEditor() );
				}
				
				bbTrack * bbt = bbTrack::findBBTrack( pat_num,
								_tc->eng() );
				trackContentObject * tco = bbt->addTCO(
							bbt->createTCO( 0 ) );
				tco->movePosition( midiTime( ( data & 0xffff ) *
									64 ) );
				break;
			}

			case FLP_FXSine:
				printf( "fx sine: %d\n", data );
				break;

			case FLP_CutCutBy:
				printf( "cut cut by: %d\n", data );
				break;

			case FLP_DelayReso:
				printf( "delay resonance: %d\n", data );
				break;

			case FLP_Reverb:
				printf( "reverb: %d\n", data );
				break;

			case FLP_IntStretch:
				printf( "int stretch: %d\n", data );
				break;

			// TEXT EVENTS
			case FLP_Text_ChanName:
				it->setName( text );
				break;

			case FLP_Text_CommentRTF:
				_tc->eng()->getProjectNotes()->setText( text );
				break;

			case FLP_Text_Title:
				printf( "project title: %s\n", text );
				break;

			case FLP_Text_SampleFileName:
				printf( "sample for current channel: %s\n",
									text );
				break;

			case FLP_Version:
				printf( "FL Version: %s\n", text );
				break;

			case FLP_Text_PluginName:
				if( QString( text ) == "3x Osc" )
				{
					printf( "loading TripleOscillator for "
							"3x Osc\n" );
					it->loadInstrument(
							"tripleoscillator" );
				}
				else
				{
					printf( "unsupported plugin: %s\n",
									text );
				}
				break;

			case FLP_Delay:
				printf( "delay-data:\n" );
				dump_mem( text, text_len );
				break;

			case FLP_NewPlugin:
				printf( "new plugin: %s\n", text );
				break;

			case FLP_PluginParams:
				printf( "plugin params:\n" );
				dump_mem( text, text_len );
				break;

			case FLP_ChanParams:
				printf( "plugin params:\n" );
				dump_mem( text, text_len );
				break;

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
						"not implemented yet.\n", ev );
				}
				break;
		}
/*
		// now process every event
		for( eventVector::const_iterator it = m_events.begin();
						it != m_events.end(); ++it )
		{
			const int tick = it->first;
			const midiEvent & ev = it->second;
			switch( ev.m_type )
			{
				case NOTE_ON:
					if( ev.key() >=
						NOTES_PER_OCTAVE * OCTAVES )
					{
						continue;
					}
					if( ev.velocity() > 0 )
					{
						keys[ev.key()][0] = tick;
						keys[ev.key()][1] =
								ev.velocity();
						break;
					}

				case NOTE_OFF:
					if( ev.key() <
						NOTES_PER_OCTAVE * OCTAVES &&
							keys[ev.key()][0] >= 0 )
					{
			note n( eng(),
				midiTime( ( tick - keys[ev.key()][0] ) / 10 ),
				midiTime( keys[ev.key()][0] / 10 ),
				(tones)( ev.key() % NOTES_PER_OCTAVE ),
				(octaves)( ev.key() / NOTES_PER_OCTAVE ),
				keys[ev.key()][1] * 100 / 128 );
						p->addNote( n );
						keys[ev.key()][0] = -1;
					}
					break;

				default:
//					printf( "Unhandled event: %#x\n",
//								ev.m_type );
					break;
			}
		}*/
        }
        return( TRUE );
}



#if 0
bool FASTCALL flpImport::readTrack( int _track_end )
{
        int tick = 0;
        unsigned char last_cmd = 0;
//        unsigned char port = 0;

	m_events.clear();
        // the current file position is after the track ID and length
        while( (int) file().pos() < _track_end )
	{
		unsigned char cmd;
		int len;

		int delta_ticks = readVar();
		if( delta_ticks < 0 )
		{
			break;
		}
		tick += delta_ticks;

		int c = readByte();
		if( c < 0 )
		{
			break;
		}
		if( c & 0x80 )
		{
			// have command
			cmd = c;
			if( cmd < 0xf0 )
			{
				last_cmd = cmd;
			}
		}
		else
		{
			// running status
			ungetChar( c );
			cmd = last_cmd;
			if( !cmd )
			{
				error();
				return( FALSE );
			}
		}
                switch( cmd & 0xF0 )
		{
			// channel msg with 2 parameter bytes
			case NOTE_OFF:
			case NOTE_ON:
			case KEY_PRESSURE:
			case CONTROL_CHANGE:
			case PITCH_BEND:
			{
				int data1 = readByte() & 0x7F;
				int data2 = readByte() & 0x7F;
				m_events.push_back( qMakePair( tick,
					midiEvent( static_cast<midiEventTypes>(
								cmd & 0xF0 ),
							cmd & 0x0F,
							data1,
							data2 ) ) );
				break;
			}
			// channel msg with 1 parameter byte
			case PROGRAM_CHANGE:
			case CHANNEL_PRESSURE:
				m_events.push_back( qMakePair( tick,
					midiEvent( static_cast<midiEventTypes>(
								cmd & 0xF0 ),
							cmd & 0x0F,
							readByte() & 0x7F ) ) );
				break;

			case MIDI_SYSEX:
				switch( cmd )
				{
					case MIDI_SYSEX:
					case MIDI_EOX:
					{
						len = readVar();
						if( len < 0 )
						{
							error();
							return( FALSE );
						}
						if( cmd == MIDI_SYSEX )
						{
							++len;
						}
						char * data = new char[len];
						if( cmd == MIDI_SYSEX )
						{
							data[0] = MIDI_SYSEX;
						}
						for( ; c < len; ++c )
						{
							data[c] = readByte();
						}
						m_events.push_back(
							qMakePair( tick,
				midiEvent( MIDI_SYSEX, data, len ) ) );
						break;
					}

					case MIDI_META_EVENT:
						c = readByte();
						len = readVar();
/*						if( len < 0 )
						{
							error();
							return( FALSE );
						}*/
						switch( c )
						{
						case 0x21: // port number
							if( len < 1 )
							{
								error();
								return( FALSE );
							}
/*							port = readByte() %
								port_count;
							skip( len - 1 );*/
							skip( len );
							break;

						case 0x2F: // end of track
						//track->end_tick = tick;
							skip( _track_end -
								file().pos() );
							return( TRUE );

						case 0x51: // tempo
							if( len < 3 )
							{
								error();
								return( FALSE );
							}
							if( m_smpteTiming )
							{
								// SMPTE timing
								// doesnt change
								skip( len );
							}
							else
							{
/*			event = new_event(track, 0);
			event->type = SND_SEQ_EVENT_TEMPO;
			event->port = port;
			event->tick = tick;
			event->data.tempo = read_byte() << 16;
			event->data.tempo |= read_byte() << 8;
			event->data.tempo |= read_byte();
			skip( len -3 );*/
								skip( len );
							}
							break;

						default:// ignore all other
							// meta events
							skip( len );
							break;
						}
						break;

					default: // invalid Fx command
						error();
						return( FALSE );
				}
				break;

			default: // cannot happen
                	        error();
				return( FALSE );
                }
        }
	error();
	return( FALSE );
}

#endif



extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new flpImport( static_cast<const char *>( _data ) ) );
}


}


#undef pos
#undef setValue
