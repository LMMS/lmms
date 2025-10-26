/*
 * LocalZynAddSubFx.cpp - local implementation of ZynAddSubFx plugin
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "LocalZynAddSubFx.h"

#include <ctime>

#include "lmmsconfig.h"

#ifdef LMMS_BUILD_WIN32
#	include <wchar.h>
#	include "IoHelper.h"
#else
#	include <unistd.h>
#endif

#include "MidiEvent.h"
#include "SampleFrame.h"

#include <Nio/NulEngine.h>
#include <Misc/Master.h>
#include <Misc/Part.h>
#include <Misc/Util.h>

// Global variable in zynaddsubfx/src/globals.h
SYNTH_T* synth = nullptr;


namespace lmms
{


int LocalZynAddSubFx::s_instanceCount = 0;


LocalZynAddSubFx::LocalZynAddSubFx() :
	m_master( nullptr ),
	m_ioEngine( nullptr )
{
	if( s_instanceCount == 0 )
	{
		initConfig();

		synth = new SYNTH_T;
		synth->oscilsize = config.cfg.OscilSize;
		synth->alias();

		srand( time( nullptr ) );

		denormalkillbuf = new float[synth->buffersize];
		for( int i = 0; i < synth->buffersize; ++i )
		{
			denormalkillbuf[i] = (RND-0.5)*1e-16;
		}
	}

	++s_instanceCount;

	m_ioEngine = new NulEngine;

	m_master = new Master();
	m_master->swaplr = false;
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

	{
		const auto lock = std::lock_guard{m_master->mutex};
		m_master->defaults();
		m_master->loadXML( f );
	}

	m_master->applyparameters();

#ifdef LMMS_BUILD_WIN32
	_wunlink(toWString(_filename).get());
#else
	unlink( f );
#endif

	free( f );
}




void LocalZynAddSubFx::loadPreset( const std::string & _filename, int _part )
{
	char * f = strdup( _filename.c_str() );

	{
		const auto lock = std::lock_guard{m_master->mutex};
		m_master->part[_part]->defaultsinstrument();
		m_master->part[_part]->loadXMLinstrument( f );
	}

	m_master->applyparameters();

	free( f );
}




void LocalZynAddSubFx::setPresetDir( const std::string & _dir )
{
	m_presetsDir = _dir;
	for (auto& bankRootDir : config.cfg.bankRootDirList)
	{
		if (bankRootDir.empty())
		{
			bankRootDir = m_presetsDir;
			break;
		}
		else if (bankRootDir == m_presetsDir)
		{
			break;
		}
	}
}




void LocalZynAddSubFx::setLmmsWorkingDir( const std::string & _dir )
{
	if( config.workingDir != nullptr )
	{
		free( config.workingDir );
	}
	config.workingDir = strdup( _dir.c_str() );

	initConfig();
}



void LocalZynAddSubFx::setPitchWheelBendRange( int semitones )
{
	for (const auto& part : m_master->part)
	{
		part->ctl.setpitchwheelbendrange(semitones * 100);
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




void LocalZynAddSubFx::processAudio( SampleFrame* _out )
{
#ifdef _MSC_VER
	const auto outputl = static_cast<float*>(_alloca(synth->buffersize * sizeof(float)));
	const auto outputr = static_cast<float*>(_alloca(synth->buffersize * sizeof(float)));
#else
	float outputl[synth->buffersize];
	float outputr[synth->buffersize];
#endif

	m_master->GetAudioOutSamples( synth->buffersize, synth->samplerate, outputl, outputr );

	// TODO: move to MixHelpers
	for( int f = 0; f < synth->buffersize; ++f )
	{
		_out[f][0] = outputl[f];
		_out[f][1] = outputr[f];
	}
}


} // namespace lmms
