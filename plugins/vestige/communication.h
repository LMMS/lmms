#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#include <unistd.h>
#include <string.h>
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


enum vstRemoteCommands
{
	// client -> server
	LOAD_VST_PLUGIN = 0,
	CLOSE_VST_PLUGIN,
	PROCESS = 10,
	ENQUEUE_MIDI_EVENT = 11,
	SET_SAMPLE_RATE = 20,
	SET_BUFFER_SIZE,
	GET_SHM_KEY_AND_SIZE,
	GET_VST_VERSION = 30,
	GET_NAME,
	GET_VENDOR_STRING,
	GET_PRODUCT_STRING,

	// server -> client
	SET_SHM_KEY_AND_SIZE = 100,
	SET_INPUT_COUNT,
	SET_OUTPUT_COUNT,
	SET_XID,
	INITIALIZATION_DONE,
	FAILED_LOADING_PLUGIN,
	QUIT_ACK,
	GET_SAMPLE_RATE = 110,
	GET_BUFFER_SIZE,
	GET_BPM,
	SET_VST_VERSION,
	SET_NAME,
	SET_VENDOR_STRING,
	SET_PRODUCT_STRING,
	PROCESS_DONE = 120,

	DEBUG_MSG = 200,
	UNDEFINED_CMD

} ;




static inline std::string readString( int _fd = 0 )
{
	Sint16 len = readValue<Sint16>( _fd );
	char * sc = new char[len];
	read( _fd, sc, len );
	std::string s( sc );
	delete[] sc;
	return( s );
}




static inline void writeString( const char * _str, int _fd = 1 )
{
	int len = strlen( _str ) + 1;
	writeValue<Sint16>( len, _fd );
	write( _fd, _str, len );
}


#endif
