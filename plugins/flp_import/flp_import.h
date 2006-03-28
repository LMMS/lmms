/*
 * flp_import.h - support for importing FLP-files
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


#ifndef _FLP_IMPORT_H
#define _FLP_IMPORT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QString>
#include <QPair>
#include <QVector>

#else

#include <qstring.h>
#include <qpair.h>
#include <qvaluevector.h>

#endif


#include "midi.h"
#include "import_filter.h"


enum flpEvents
{
	// BYTE EVENTS
	FLP_Byte      	= 0,
	FLP_Enabled   	= 0,
	FLP_NoteOn    	= 1,	//+pos (byte)
	FLP_Vol       	= 2,
	FLP_Pan       	= 3,
	FLP_MIDIChan  	= 4,
	FLP_MIDINote  	= 5,
	FLP_MIDIPatch 	= 6,
	FLP_MIDIBank  	= 7,
	FLP_LoopActive	= 9,
	FLP_ShowInfo  	= 10,
	FLP_Shuffle   	= 11,
	FLP_MainVol   	= 12,
	FLP_Stretch   	= 13,	// old byte version
	FLP_Pitchable 	= 14,
	FLP_Zipped    	= 15,
	FLP_Delay_Flags	= 16,
	FLP_PatLength 	= 17,
	FLP_BlockLength	= 18,
	FLP_UseLoopPoints	= 19,
	FLP_LoopType  	= 20,
	FLP_ChanType  	= 21,
	FLP_MixSliceNum	= 22,

	// WORD EVENTS
	FLP_Word     	= 64,
	FLP_NewChan  	= FLP_Word,
	FLP_NewPat   	= FLP_Word + 1,		//+PatNum (word)
	FLP_Tempo    	= FLP_Word + 2,
	FLP_CurrentPatNum	= FLP_Word + 3,
	FLP_PatData  	= FLP_Word + 4,
	FLP_FX       	= FLP_Word + 5,
	FLP_Fade_Stereo	= FLP_Word + 6,
	FLP_CutOff   	= FLP_Word + 7,
	FLP_DotVol   	= FLP_Word + 8,
	FLP_DotPan   	= FLP_Word + 9,
	FLP_PreAmp   	= FLP_Word + 10,
	FLP_Decay    	= FLP_Word + 11,
	FLP_Attack   	= FLP_Word + 12,
	FLP_DotNote  	= FLP_Word + 13,
	FLP_DotPitch 	= FLP_Word + 14,
	FLP_DotMix   	= FLP_Word + 15,
	FLP_MainPitch	= FLP_Word + 16,
	FLP_RandChan 	= FLP_Word + 17,
	FLP_MixChan  	= FLP_Word + 18,
	FLP_Resonance	= FLP_Word + 19,
	FLP_LoopBar  	= FLP_Word + 20,
	FLP_StDel    	= FLP_Word + 21,
	FLP_FX3      	= FLP_Word + 22,
	FLP_DotReso  	= FLP_Word + 23,
	FLP_DotCutOff	= FLP_Word + 24,
	FLP_ShiftDelay	= FLP_Word + 25,
	FLP_LoopEndBar	= FLP_Word + 26,
	FLP_Dot      	= FLP_Word + 27,
	FLP_DotShift 	= FLP_Word + 28,

	// DWORD EVENTS
	FLP_Int      	= 128,
	FLP_Color    	= FLP_Int,
	FLP_PlayListItem	= FLP_Int + 1,	//+Pos (word) +PatNum (word)
	FLP_Echo     	= FLP_Int + 2,
	FLP_FXSine   	= FLP_Int + 3,
	FLP_CutCutBy 	= FLP_Int + 4,
	FLP_WindowH  	= FLP_Int + 5,
	FLP_MiddleNote	= FLP_Int + 7,
	FLP_Reserved  	= FLP_Int + 8,	// may contain an invalid version info
	FLP_MainResoCutOff	= FLP_Int + 9,
	FLP_DelayReso	= FLP_Int + 10,
	FLP_Reverb   	= FLP_Int + 11,
	FLP_IntStretch	= FLP_Int + 12,
	FLP_SSNote   	= FLP_Int + 13,
	FLP_FineTune 	= FLP_Int + 14,

	// TEXT EVENTS
	FLP_Undef    	= 192,		//+Size (var length)
	FLP_Text     	= FLP_Undef,	//+Size (var length)+Text
					//		(Null Term. String)
	FLP_Text_ChanName  	= FLP_Text,	// name for the current channel
	FLP_Text_PatName   	= FLP_Text + 1,	// name for the current pattern
	FLP_Text_Title     	= FLP_Text + 2,	// title of the loop
	FLP_Text_Comment   	= FLP_Text + 3,	// old comments in text format.
						// Not used anymore
	FLP_Text_SampleFileName	= FLP_Text + 4,	// filename for the sample in
						// the current channel, stored
						// as relative path
	FLP_Text_URL       	= FLP_Text + 5,
	FLP_Text_CommentRTF	= FLP_Text + 6,	// new comments in Rich Text
						// format
	FLP_Version        	= FLP_Text + 7,
	FLP_Text_PluginName	= FLP_Text + 9,	// plugin file name
						// (without path)

	FLP_MIDICtrls      	= FLP_Text + 16,
	FLP_Delay          	= FLP_Text + 17,
	FLP_TS404Params    	= FLP_Text + 18,
	FLP_DelayLine      	= FLP_Text + 19,
	FLP_NewPlugin      	= FLP_Text + 20,
	FLP_PluginParams   	= FLP_Text + 21,
	FLP_ChanParams    	= FLP_Text + 23,// block of various channel
						// params (can grow)

	FLP_CmdCount

} ;





class flpImport : public importFilter
{
public:
	flpImport( const QString & _file );
	virtual ~flpImport();


private:
	virtual bool tryImport( trackContainer * _tc );

	inline int readInt( int _bytes )
	{
		int c, value = 0;
		do
		{
			c = readByte();
			if( c == -1 )
			{
				return( -1 );
			}
			value = ( value << 8 ) | c;
		} while( --_bytes );
		return( value );
	}

	inline Sint32 read32LE( void )
	{
		int value = readByte();
		value |= readByte() << 8;
		value |= readByte() << 16;
		value |= readByte() << 24;
		return( value );
	}
	inline Sint32 read16LE( void )
	{
		int value = readByte();
		value |= readByte() << 8;
		return( value );
	}
/*	inline int readVar( void )
	{
		int c = readByte();
		int value = c & 0x7f;
		if( c & 0x80 )
		{
			c = readByte();
			value = ( value << 7 ) | ( c & 0x7f );
			if( c & 0x80 )
			{
				c = readByte();
				value = ( value << 7 ) | ( c & 0x7f );
				if( c & 0x80 )
				{
					c = readByte();
					value = ( value << 7 ) | c;
					if( c & 0x80 )
					{
						return -1;
					}
				}
			}
	        }
        	return( !file().atEnd() ? value : -1 );
	}*/

	inline Sint32 readID( void )
	{
		return( read32LE() );
	}

	inline void skip( int _bytes )
	{
		while( _bytes > 0 )
		{
			readByte();
			--_bytes;
		}
	}


	typedef vvector<QPair<int, midiEvent> > eventVector;
	eventVector m_events;

} ;


#endif
