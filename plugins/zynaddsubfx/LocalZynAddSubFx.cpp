/*
 * LocalZynAddSubFx.cpp - local implementation of ZynAddSubFx plugin
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <lmmsconfig.h>

#include "zynaddsubfx/src/Misc/Util.h"
#include <unistd.h>
#include <ctime>

#include "LocalZynAddSubFx.h"

#include "zynaddsubfx/src/Nio/NulEngine.h"
#include "zynaddsubfx/src/Misc/Master.h"
#include "zynaddsubfx/src/Misc/Part.h"
#include "zynaddsubfx/src/Misc/Dump.h"


SYNTH_T* synth = NULL;

int LocalZynAddSubFx::s_instanceCount = 0;


LocalZynAddSubFx::LocalZynAddSubFx() :
	m_master( NULL ),
	m_ioEngine( NULL )
{
	for( int i = 0; i < NumKeys; ++i )
	{
		m_runningNotes[i] = 0;
	}

	if( s_instanceCount == 0 )
	{
#ifdef LMMS_BUILD_WIN32
#ifndef __WINPTHREADS_VERSION
		// (non-portable) initialization of statically linked pthread library
		pthread_win32_process_attach_np();
		pthread_win32_thread_attach_np();
#endif
#endif

		initConfig();

		synth = new SYNTH_T;
		synth->oscilsize = config.cfg.OscilSize;
		synth->alias();

		srand( time( NULL ) );

		denormalkillbuf = new float[synth->buffersize];
		for( int i = 0; i < synth->buffersize; ++i )
		{
			denormalkillbuf[i] = (RND-0.5)*1e-16;
		}
	}

	++s_instanceCount;

	m_ioEngine = new NulEngine;

	m_master = new Master();
	m_master->swaplr = 0;
}




LocalZynAddSubFx::~LocalZynAddSubFx()
{
	delete m_master;
	delete m_ioEngine;

	if( --s_instanceCount == 0 )
	{
		delete[] denormalkillbuf;
	}
}




void LocalZynAddSubFx::initConfig()
{
	config.init();

	config.cfg.GzipCompression = 0;
}




void LocalZynAddSubFx::setSampleRate( int sampleRate )
{
	synth->samplerate = sampleRate;
	synth->alias();
}




void LocalZynAddSubFx::setBufferSize( int bufferSize )
{
	synth->buffersize = bufferSize;
	synth->alias();
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
		if( config.cfg.bankRootDirList[i].empty() )
		{
			config.cfg.bankRootDirList[i] = m_presetsDir;
			break;
		}
		else if( config.cfg.bankRootDirList[i] == m_presetsDir )
		{
			break;
		}
	}
}




void LocalZynAddSubFx::setLmmsWorkingDir( const std::string & _dir )
{
	if( config.workingDir != NULL )
	{
		free( config.workingDir );
	}
	config.workingDir = strdup( _dir.c_str() );

	initConfig();
}



void LocalZynAddSubFx::setPitchWheelBendRange( int semitones )
{
	for( int i = 0; i < NUM_MIDI_PARTS; ++i )
	{
		m_master->part[i]->ctl.setpitchwheelbendrange( semitones * 100 );
	}
}



void LocalZynAddSubFx::processMidiEvent( const MidiEvent& event )
{
	switch( event.type() )
	{
		case MidiNoteOn:
			if( event.velocity() > 0 )
			{
				if( event.key() < 0 || event.key() > MidiMaxKey )
				{
					break;
				}
				if( m_runningNotes[event.key()] > 0 )
				{
					m_master->noteOff( event.channel(), event.key() );
				}
				++m_runningNotes[event.key()];
				m_master->noteOn( event.channel(), event.key(), event.velocity() );
				break;
			}
		case MidiNoteOff:
			if( event.key() < 0 || event.key() > MidiMaxKey )
			{
				break;
			}
			if( --m_runningNotes[event.key()] <= 0 )
			{
				m_master->noteOff( event.channel(), event.key() );
			}
			break;
		case MidiPitchBend:
			m_master->setController( event.channel(), C_pitchwheel, event.pitchBend()-8192 );
			break;
		case MidiControlChange:
			m_master->setController( event.channel(), event.controllerNumber(), event.controllerValue() );
			break;
		default:
			break;
	}
}




void LocalZynAddSubFx::processAudio( sampleFrame * _out )
{
	float outputl[synth->buffersize];
	float outputr[synth->buffersize];

	m_master->GetAudioOutSamples( synth->buffersize, synth->samplerate, outputl, outputr );

	// TODO: move to MixHelpers
	for( int f = 0; f < synth->buffersize; ++f )
	{
		_out[f][0] = outputl[f];
		_out[f][1] = outputr[f];
	}
}


