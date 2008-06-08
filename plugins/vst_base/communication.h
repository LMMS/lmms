/*
 * communication.h - header file defining stuff concerning communication between
 *                   LVSL-server and -client
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string>


template<typename T>
inline T readValue( int _fd = 0 )
{
	T i;
	read( _fd, &i, sizeof( i ) );
	return( i );
}




template<typename T>
inline void writeValue( const T & _i, int _fd = 1 )
{
	write( _fd, &_i, sizeof( _i ) );
}




static inline std::string readString( int _fd = 0 )
{
	Sint16 len = readValue<Sint16>( _fd );
	char * sc = new char[len + 1];
	read( _fd, sc, len );
	sc[len] = '\0';
	std::string s( sc );
	delete[] sc;
	return( s );
}




static inline void writeString( const char * _str, int _fd = 1 )
{
	int len = strlen( _str );
	writeValue<Sint16>( len, _fd );
	write( _fd, _str, len );
}




struct vstParameterDumpItem
{
	Sint32 index;
	char shortLabel[8];
	float value;
} ;




// summarized version of VstParameterProperties-struct - useful because client
// doesn't have to know about the latter one
struct vstParamProperties
{
	char label[64];
	char shortLabel[8];
	char categoryLabel[24];
	float minValue;
	float maxValue;
	float step;
	Sint16 category;
} ;


enum hostLanguages
{
	LanguageEnglish = 1,
	LanguageGerman,
	LanguageFrench,
	LanguageItalian,
	LanguageSpanish,
	LanguageJapanese
} ;



enum vstRemoteCommands
{
	// client -> server
	VST_LOAD_PLUGIN = 0,
	VST_CLOSE_PLUGIN,
	VST_SHOW_EDITOR,
	VST_PROCESS,
	VST_ENQUEUE_MIDI_EVENT,
	VST_SAMPLE_RATE,
	VST_BUFFER_SIZE,
	VST_BPM,
	VST_LANGUAGE,
	VST_GET_PARAMETER_COUNT = 20,
	VST_GET_PARAMETER_DUMP,
	VST_SET_PARAMETER_DUMP,
	VST_GET_PARAMETER_PROPERTIES,

	// server -> client
	VST_INITIALIZATION_DONE = 100,
	VST_FAILED_LOADING_PLUGIN,
	VST_QUIT_ACK,
	VST_SHM_KEY_AND_SIZE,
	VST_INPUT_COUNT,
	VST_OUTPUT_COUNT,
	VST_PLUGIN_XID,
	VST_PLUGIN_EDITOR_GEOMETRY,
	VST_PROCESS_DONE,
	VST_PLUGIN_NAME,
	VST_PLUGIN_VERSION,
	VST_PLUGIN_VENDOR_STRING,
	VST_PLUGIN_PRODUCT_STRING,
	VST_PARAMETER_COUNT,
	VST_PARAMETER_DUMP,
	VST_PARAMETER_PROPERTIES,
	VST_GET_SAMPLE_RATE = 120,
	VST_GET_BUFFER_SIZE,

	VST_DEBUG_MSG = 200,
	VST_UNDEFINED_CMD

} ;




#endif
