/*
 * LocalZynAddSubFx.cpp - local implementation of ZynAddSubFx plugin
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <lmmsconfig.h>

#include "LocalZynAddSubFx.h"

#include "src/Input/MidiIn.h"
#include "src/Misc/Master.h"
#include "src/Misc/Dump.h"


int LocalZynAddSubFx::s_instanceCount = 0;


LocalZynAddSubFx::LocalZynAddSubFx()
{
	for( int i = 0; i < NumKeys; ++i )
	{
		m_runningNotes[i] = 0;
	}

	if( s_instanceCount == 0 )
	{
#ifdef LMMS_BUILD_WIN32
		// (non-portable) initialization of statically linked pthread library
		pthread_win32_process_attach_np();
		pthread_win32_thread_attach_np();
#endif

		config.init();
		OSCIL_SIZE = config.cfg.OscilSize;

		config.cfg.GzipCompression = 0;

		srand( time( NULL ) );
		denormalkillbuf = new REALTYPE[SOUND_BUFFER_SIZE];
		for( int i = 0; i < SOUND_BUFFER_SIZE; ++i )
		{
			denormalkillbuf[i] = (RND-0.5)*1e-16;
		}

		OscilGen::tmpsmps = new REALTYPE[OSCIL_SIZE];
		newFFTFREQS( &OscilGen::outoscilFFTfreqs, OSCIL_SIZE/2 );
	}
	++s_instanceCount;

	m_master = new Master();
	m_master->swaplr = 0;
}




LocalZynAddSubFx::~LocalZynAddSubFx()
{
	delete m_master;

	if( --s_instanceCount == 0 )
	{
		delete[] denormalkillbuf;
		delete[] OscilGen::tmpsmps;
		deleteFFTFREQS( &OscilGen::outoscilFFTfreqs );
	}
}




void LocalZynAddSubFx::setSampleRate( int _sampleRate )
{
	SAMPLE_RATE = _sampleRate;
}




void LocalZynAddSubFx::setBufferSize( int _bufferSize )
{
	SOUND_BUFFER_SIZE = _bufferSize;
}




void LocalZynAddSubFx::saveXML( const std::string & _filename )
{
	char * name = strdup( _filename.c_str() );
	m_master->saveXML( name );
	free( name );
}




void LocalZynAddSubFx::loadXML( const std::string & _filename )
{
	char * f = strdup( _filename.c_str() );

	pthread_mutex_lock( &m_master->mutex );
	m_master->defaults();
	m_master->loadXML( f );
	pthread_mutex_unlock( &m_master->mutex );

	m_master->applyparameters();

	unlink( f );
	free( f );
}




void LocalZynAddSubFx::loadPreset( const std::string & _filename, int _part )
{
	char * f = strdup( _filename.c_str() );

	pthread_mutex_lock( &m_master->mutex );
	m_master->part[_part]->defaultsinstrument();
	m_master->part[_part]->loadXMLinstrument( f );
	pthread_mutex_unlock( &m_master->mutex );

	m_master->applyparameters();

	free( f );
}




void LocalZynAddSubFx::setPresetDir( const std::string & _dir )
{
	m_presetsDir = _dir;
	for( int i = 0; i < MAX_BANK_ROOT_DIRS; ++i )
	{
		if( config.cfg.bankRootDirList[i] == NULL )
		{
			config.cfg.bankRootDirList[i] = new char[MAX_STRING_SIZE];
			strcpy( config.cfg.bankRootDirList[i], m_presetsDir.c_str() );
			break;
		}
		else if( strcmp( config.cfg.bankRootDirList[i],
							m_presetsDir.c_str() ) == 0 )
		{
			break;
		}
	}
}




void LocalZynAddSubFx::processMidiEvent( const midiEvent & _e )
{
	// all functions are called while m_master->mutex is held
	static MidiIn midiIn;

	switch( _e.m_type )
	{
		case MidiNoteOn:
			if( _e.velocity() > 0 )
			{
				if( _e.key() <= 0 || _e.key() >= 128 )
				{
					break;
				}
				if( m_runningNotes[_e.key()] > 0 )
				{
					m_master->NoteOff( 0, _e.key() );
				}
				++m_runningNotes[_e.key()];
				m_master->NoteOn( 0, _e.key(), _e.velocity() );
				break;
			}
		case MidiNoteOff:
			if( _e.key() <= 0 || _e.key() >= 128 )
			{
				break;
			}
			if( --m_runningNotes[_e.key()] <= 0 )
			{
				m_master->NoteOff( 0, _e.key() );
			}
			break;
		case MidiPitchBend:
			m_master->SetController( 0, C_pitchwheel,
						_e.m_data.m_param[0] +
							_e.m_data.m_param[1]*128-8192 );
			break;
		case MidiControlChange:
			m_master->SetController( 0,
						midiIn.getcontroller( _e.m_data.m_param[0] ),
						_e.m_data.m_param[1] );
			break;
		default:
			break;
	}
}




void LocalZynAddSubFx::processAudio( sampleFrame * _out )
{
	REALTYPE outputl[SOUND_BUFFER_SIZE];
	REALTYPE outputr[SOUND_BUFFER_SIZE];

	m_master->AudioOut( outputl, outputr );

	for( int f = 0; f < SOUND_BUFFER_SIZE; ++f )
	{
		_out[f][0] = outputl[f];
		_out[f][1] = outputr[f];
	}
}


