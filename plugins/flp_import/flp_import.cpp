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
#include "project_journal.h"


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

	printf( "channels: %d\n", num_channels );

	instrumentTrack * it = NULL;
	int current_pattern = 0;
	char * text = NULL;
	int text_len = 0;
	vlist<instrumentTrack *> i_tracks;

	int ev_cnt = 0;

	_tc->eng()->getSongEditor()->clearProject();
	const bool is_journ = _tc->eng()->getProjectJournal()->isJournalling();
	_tc->eng()->getProjectJournal()->setJournalling( FALSE );


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
			int shift = 0;
			while( data & 0x80 )
			{
				data = readByte();
				//text_len = ( text_len << 7 ) | ( data & 0x7F );
				text_len = text_len | ( ( data & 0x7F ) << ( shift += 7 ) );
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
				_tc->eng()->getSongEditor()->setMasterVolume(
						static_cast<volume>( data ) );
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
/*
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 36:
				printf( "ev: %d data: %d\n", ev, data );
				break;*/

			// WORD EVENTS
			case FLP_NewChan:
				printf( "new channel\n" );

				it = dynamic_cast<instrumentTrack *>(
	track::create( track::CHANNEL_TRACK, _tc->eng()->getBBEditor() ) );
				assert( it != NULL );
				i_tracks.push_back( it );
				it->loadInstrument( "tripleoscillator" );
				it->toggledInstrumentTrackButton( FALSE );

				break;

			case FLP_NewPat:
				current_pattern = data - 1;
				break;

			case FLP_Tempo:
				printf( "tempo: %d\n", data );
				_tc->eng()->getSongEditor()->setTempo( data );
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
				_tc->eng()->getSongEditor()->setMasterPitch(
									data );
				break;

			case FLP_Resonance:
				printf( "reso (for cur channel?): %d\n", data );
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

			case FLP_PatternData:
			{
				printf( "pattern data:\n" );
				//dump_mem( text, text_len );
				const int bpn = 20;
				for( int i = 0; i*bpn < text_len; ++i )
				{
					int ch = *( text + i*bpn + 6 );
					int pos = *( (int *)( text + i*bpn ) );
					int key = *( text + i*bpn + 12 );
					int len = *( (int*)( text + i*bpn +
									8 ) );
					pos /= 6;
					len /= 6;
					note n( NULL, len, pos );
					n.setKey( key );
					m_notes.push_back( qMakePair(
				num_channels * current_pattern + ch, n ) );
				
					//printf( "note on channel %d at pos %d with key %d and length %d   ", (int)*((text+i*bpn+6)), *((int*)(text+i*bpc)), (int) *(text+i*bpc+12), (int) *((int*)(text+i*bpc+8)));
					//printf( "note on channel %d at pos %d with key %d and length %d\n", ch, pos, key, len );
					
					//dump_mem( text+i*bpn+4, bpc-4 );
				}
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
						"not implemented yet.\n", ev );
				}
				break;
		}
        }

	// now process all notes
	for( patternNoteVector::const_iterator it = m_notes.begin();
						it != m_notes.end(); ++it )
	{
		const int where = ( *it ).first;
		const int ch = where % num_channels;
		const csize pat = where / num_channels;
		while( _tc->eng()->getBBEditor()->numOfBBs() <= pat )
		{
			track::create( track::BB_TRACK,
						_tc->eng()->getSongEditor() );
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
		unsigned int pat_num = ( ( *it ) >> 16 ) - 1;
		while( _tc->eng()->getBBEditor()->numOfBBs() <= pat_num )
		{
			track::create( track::BB_TRACK,
						_tc->eng()->getSongEditor() );
		}
		
		bbTrack * bbt = bbTrack::findBBTrack( pat_num, _tc->eng() );
		trackContentObject * tco = bbt->addTCO( bbt->createTCO( 0 ) );
		tco->movePosition( midiTime( ( ( *it ) & 0xffff ) * 64 ) );
	}

	_tc->eng()->getProjectJournal()->setJournalling( is_journ );
        return( TRUE );
}





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
