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

/*


The FruityLoops
FLP
file format explained

(v 2.7.x)

(BETA)


Introduction:

Here I will try to explain the FLP file format, which is the FruityLoops loop / song format. Visit www.fruityloops.com to get more info about FruityLoops.

First you've got to understand that FruityLoops keeps growing, so does the FLP format. Fortunately it has been designed for it... While I made some mistakes during its evolution, the format still remains updatable.
Normally anything made out of this document will be able to process future loop files, although of course you will miss newly added features.

Not sure what this document could be useful for, but if you ever use it to make something, send it to me!
Also if you'd like me to update this file (it's not complete yet), feel free to mail me.

(2/09/99) Didier Dambrin (gol)
gol@fruityloops.com



How it works:

Don't expect the FLP format to be a big chunk full of ordered parameters, like most trackers file formats. It had to evolute, so I chose the 'events' way.
You should see it a bit like MIDI / AIFF files. It's just a succession of events. Once you've understand how to process the file to retrieve these events, the only thing you'll need is the list of events available!

It's better, since once you've made the piece of code to get the events, you won't bother with the format anymore. Also you will just ignore any event you don't know (yet) about.

Please note that the format *does not* respect the AIFF standard, although I tried to keep the chunks system similar.

Although the program has been coded in Pascal, I'll do my best to use C++ declarations, so everyone will understand. DWORD is 4 bytes, WORD is 2 bytes.



Retrieving the events:

I said it looks like a MIDI file, but it's not a MIDI file.

First, you'll have to get & check the HEADER chunk, to be sure it's a FLP file.
The header is similar to the format of a MIDI file header:

DWORD	ChunkID	4 chars which are the letters 'FLhd' for 'FruityLoops header'
DWORD	Length	The length of this chunk, like in MIDI files. Should be 6 because of the 3 WORDS below...
WORD	Format	Set to 0 for full songs.
WORD	nChannels	The total number of channels (not really used).
WORD	BeatDiv	Pulses per quarter of the song.

Most of this chunk is not used, it's just that I tried (as a start) to respect the proper MIDI header :)


Then you'll encounter the DATA chunk, which is in fact the last chunk, the one containing all the events.

DWORD	ChunkID	4 chars which are the letters 'FLdt' for 'FruityLoops data'
DWORD	Length	The length of this chunk WITHOUT these 2 DWORDS (that is minus 4*2 bytes), like in MIDI files.


The whole data chunk is a succession of EVENTS, which I'm going to explain...
To retrieve an event, first you read a byte (the event ID). According to this byte, the size of the event data varies:
0..63	The data after this byte is a BYTE (signed or unsigned, depending on the ID).
64..127	The data after this byte is a WORD.
128..191	The data after this byte is a DWORD.
192..255	The data after this byte is a variable-length block of data (a text for example).

That makes 64 BYTE events, 64 WORD events, 64 DWORD events & 64 TEXT events. The purpose of this split is of course to keep the file size small.
So you get the event ID & then you read the number of bytes according to this ID. Whether you process the event or not isn't important. What is important is that you can jump correctly to the next event if you skip it.


For TEXT (variable-length) events, you still have to read the size of the event, which is coded in the next byte(s) a bit like in MIDI files (but not stupidly inverted). After the size is the actual data, which you can process or skip.
To get the size of the event, you've got to read bytes until the last one, which has bit 7 off (the purpose of this compression is to reduce the file size again).

Start with a DWORD Size = 0. You're going to reconstruct the size by getting packs of 7 bits:
1.	Get a byte.
2.	Add the first 7 bits of this byte to Size.
3.	Check bit 7 (the last bit) of this byte. If it's on, go back to 1. to process the next byte.

To resume, if Size < 128 then it will occupy only 1 byte, else if Size < 16384 it will occupy only 2 bytes & so on...


So globally, you open the file, check the header, point to the data chunk & retrieve / filter all the events. Easy(?)
Now let's get to the events...


*/



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
	int pat_cnt = 0;


        // read channels
/*	for( int i = 0; i < num_channels; ++i )
	{
		pd.setValue( i );
#ifdef QT4
		qApp->processEvents( QEventLoop::AllEvents, 100 );
#else
		qApp->processEvents( 100 );
#endif

		if( pd.wasCanceled() )
		{
			return( FALSE );
		}*/
	while( file().atEnd() == FALSE )
	{
		flpEvents ev = static_cast<flpEvents>( readByte() );
		Uint32 data = readByte();

		//printf("ev: %d\n", (int) ev );
		if( ev >= FLP_Word && ev < FLP_Text )
		{
			data = data | ( readByte() << 8 );
		}

		if( ev >= FLP_Int && ev < FLP_Text )
		{
			data = data | ( readByte() << 16 );
			data = data | ( readByte() << 24 );
		}

/*For TEXT (variable-length) events, you still have to read the size of the event, which is coded in the next byte(s) a bit like in MIDI files (but not stupidly inverted). After the size is the actual data, which you can process or skip.
To get the size of the event, you've got to read bytes until the last one, which has bit 7 off (the purpose of this compression is to reduce the file size again).

Start with a DWORD Size = 0. You're going to reconstruct the size by getting packs of 7 bits:
1.	Get a byte.
2.	Add the first 7 bits of this byte to Size.
3.	Check bit 7 (the last bit) of this byte. If it's on, go back to 1. to process the next byte.

To resume, if Size < 128 then it will occupy only 1 byte, else if Size < 16384 it will occupy only 2 bytes & so on...
*/

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
			case FLP_NoteOn:
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

			case FLP_MainPitch:
				printf( "main-pitch: %d\n", data );
				break;

			case FLP_PatLength:
				printf( "pattern-length: %d\n", data );
				break;

			case FLP_BlockLength:
				printf( "block length: %d\n", data );
				break;

			case FLP_ChanType:
				printf( "channel type: %d\n", data );
				break;

			// WORD EVENTS
			case FLP_NewChan:
				printf( "new channel\n" );
				m_events.clear();
				pat_cnt = 0;

				it = dynamic_cast<instrumentTrack *>(
			track::create( track::CHANNEL_TRACK, _tc ) );
				assert( it != NULL );
				it->loadInstrument( "tripleoscillator" );
				it->toggledInstrumentTrackButton( FALSE );

				break;

			case FLP_NewPat:
				p = dynamic_cast<pattern *>( it->createTCO(
								pat_cnt ) );
				assert( p != NULL );
				it->addTCO( p );
				break;

			case FLP_Tempo:
				printf( "tempo: %d\n", data );
				break;

			case FLP_CurrentPatNum:
				printf( "current pattern: %d\n", data );
				break;

			case FLP_FX:
				printf( "FX-channel for cur channel: %d\n", data );
				break;

			case FLP_CutOff:
				printf( "cutoff (for cur channel?): %d\n", data );
				break;

			case FLP_Resonance:
				printf( "reso (for cur channel?): %d\n", data );
				break;

			case FLP_FX3:
				printf( "FX 3: %d\n", data );
				break;

			case FLP_ShiftDelay:
				printf( "shift delay: %d\n", data );
				break;

			// DWORD EVENTS
			case FLP_DelayReso:
				printf( "delay resonance: %d\n", data );
				break;

			case FLP_Reverb:
				printf( "reverb: %d\n", data );
				break;

			case FLP_Version:
				printf( "FL Version: %s\n", text );
				break;

			case FLP_Text_PluginName:
				if( QString( text ) == "3x Osc" )
				{
					it->loadInstrument( "tripleoscillator" );
				}
				else
				{
					printf( "unsupported plugin: %s\n", text );
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

			default:
				if( ev >= FLP_Text )
				{
					printf( "unhandled text (%d): %s\n", ev,
						text );
				}
				else
				{
					printf( "!! handling of FLP-event %d not "
						"implemented yet.\n", ev );
				}
				break;
		}
/*
		// now create new channel-track for reading track
		instrumentTrack * ct = dynamic_cast<instrumentTrack *>(
						track::create(
							track::CHANNEL_TRACK,
							_tc ) );
#ifdef LMMS_DEBUG
		assert( ct != NULL );
#endif
		// TODO: setup program, channel etc.
		ct->loadInstrument( "tripleoscillator" );
		ct->toggledInstrumentTrackButton( FALSE );

		// now create pattern to store notes in
		pattern * p = dynamic_cast<pattern *>( ct->createTCO( 0 ) );
#ifdef LMMS_DEBUG
		assert( p != NULL );
#endif
		ct->addTCO( p );

		// init keys
		int keys[NOTES_PER_OCTAVE * OCTAVES][2];
		for( int j = 0; j < NOTES_PER_OCTAVE * OCTAVES; ++j )
		{
			keys[j][0] = -1;
		}

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

/*


void flpImport::error( void )
{
	printf( "flpImport::readTrack(): invalid MIDI data (offset %#x)\n",
						(unsigned int) file().pos() );
}

*/

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
