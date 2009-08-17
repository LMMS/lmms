/*
 * remote_zynaddsubfx.cpp - ZynAddSubFX-embedding plugin
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <queue>

#define BUILD_REMOTE_PLUGIN_CLIENT
#include "engine.h"
#include "instrument_play_handle.h"
#include "note_play_handle.h"
#include "remote_plugin.h"
#include "remote_zynaddsubfx.h"


std::string __presets_dir;

#define main unused_main
#ifdef LMMS_BUILD_LINUX
#define ATOM(x) (x)
#endif
#include "src/main.C"

#undef main

#include <FL/x.H>

static pthread_t __gui_thread_handle;
static pthread_mutex_t __gui_mutex;
static std::queue<remotePluginClient::message> __gui_messages;


class remoteZynAddSubFX : public remotePluginClient
{
public:
	remoteZynAddSubFX( int _shm_in, int _shm_out ) :
		remotePluginClient( _shm_in, _shm_out )
	{
		for( int i = 0; i < NumKeys; ++i )
		{
			m_runningNotes[i] = 0;
		}
		setInputCount( 0 );
		sendMessage( IdInitDone );
		waitForMessage( IdInitDone );
	}

	virtual void updateSampleRate( void )
	{
		SAMPLE_RATE = sampleRate();
	}

	virtual void updateBufferSize( void )
	{
		SOUND_BUFFER_SIZE = bufferSize();
	}

	virtual bool processMessage( const message & _m )
	{
		bool gui_message = false;
		switch( _m.id )
		{
			case IdQuit:
				delete master;
				break;

			case IdShowUI:
			case IdHideUI:
			case IdLoadSettingsFromFile:
			case IdLoadPresetFromFile:
				gui_message = true;
				break;

			case IdSaveSettingsToFile:
			{
				char * name = strdup( _m.getString().c_str() );
				master->saveXML( name );
				free( name );
				sendMessage( IdSaveSettingsToFile );
				return true;
				break;
			}

			case IdZasfPresetDirectory:
				__presets_dir = _m.getString();
	for( int i = 0; i < MAX_BANK_ROOT_DIRS; ++i )
	{
		if( config.cfg.bankRootDirList[i] == NULL )
		{
			config.cfg.bankRootDirList[i] = new char[MAX_STRING_SIZE];
			strcpy(config.cfg.bankRootDirList[i], __presets_dir.c_str() );
			break;
		}
		else if( strcmp( config.cfg.bankRootDirList[i],
						__presets_dir.c_str() ) == 0 )
		{
			break;
		}
	}
				break;

			default:
				return remotePluginClient::processMessage( _m );
		}
		if( gui_message )
		{
			pthread_mutex_lock( &__gui_mutex );
			__gui_messages.push( _m );
			pthread_mutex_unlock( &__gui_mutex );
		}
		return true;
	}

	virtual ~remoteZynAddSubFX()
	{
	}
  
	// all functions are called while master->mutex is held
	virtual void processMidiEvent( const midiEvent & _e,
									const f_cnt_t /* _offset */ )
	{
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
						master->NoteOff( 0, _e.key() );
					}
					++m_runningNotes[_e.key()];
					master->NoteOn( 0, _e.key(),
							_e.velocity() );
					break;
				}
			case MidiNoteOff:
				if( _e.key() <= 0 || _e.key() >= 128 )
				{
					break;
				}
				if( --m_runningNotes[_e.key()] <= 0 )
				{
					master->NoteOff( 0, _e.key() );
				}
				break;
			case MidiPitchBend:
				master->SetController( 0,
					C_pitchwheel,
					_e.m_data.m_param[0] +
						_e.m_data.m_param[1]*128-8192 );
				break;
			case MidiControlChange:
				master->SetController( 0,
					midiIn.getcontroller( _e.m_data.m_param[0] ),
					_e.m_data.m_param[1] );
				break;
			default:
				break;
		}
	}


	virtual void process( const sampleFrame * _in, sampleFrame * _out )
	{
		REALTYPE outputl[SOUND_BUFFER_SIZE];
		REALTYPE outputr[SOUND_BUFFER_SIZE];

		master->AudioOut( outputl, outputr );

		for( fpp_t f = 0; f < SOUND_BUFFER_SIZE; ++f )
		{
			_out[f][0] = outputl[f];
			_out[f][1] = outputr[f];
		}
	}


private:
	int m_runningNotes[NumKeys];

} ;

static remoteZynAddSubFX * __remote_zasf = NULL;
static int __exit = 0;


void * guiThread( void * )
{
	int e;
	ui = NULL;

	while( !__exit )
	{
		if( ui )
		{
			Fl::wait( 0.1 );
		}
		else
		{
#ifdef LMMS_BUILD_WIN32
			Sleep( 100 );
#else
			usleep( 100*1000 );
#endif
		}
		pthread_mutex_lock( &__gui_mutex );
		while( __gui_messages.size() )
		{
			remotePluginClient::message m = __gui_messages.front();
			__gui_messages.pop();
			switch( m.id )
			{
				case IdShowUI:
					// we only create GUI
					if( !ui )
					{
						Fl::scheme( "plastic" );
						ui = new MasterUI( master, &e );
					}
					ui->showUI();
					ui->refresh_master_ui();
					break;

				case IdHideUI:
					if( !ui ) break;
					switch( config.cfg.UserInterfaceMode )
					{
						case 0:
				ui->selectuiwindow->hide();
				break;
						case 1:
				ui->masterwindow->hide();
				break;
						case 2:
				ui->simplemasterwindow->hide();
				break;
					}
					break;

				case IdLoadSettingsFromFile:
				{
					char * f = strdup( m.getString().
								c_str() );
					pthread_mutex_lock( &master->mutex );
					master->defaults();
					master->loadXML( f );
					pthread_mutex_unlock( &master->mutex );
					master->applyparameters();
					if( ui ) ui->refresh_master_ui();
					unlink( f );
					free( f );
					pthread_mutex_lock( &master->mutex );
					__remote_zasf->sendMessage(
						IdLoadSettingsFromFile );
					pthread_mutex_unlock( &master->mutex );
					break;
				}

				case IdLoadPresetFromFile:
				{
					char * f = strdup( m.getString().
								c_str() );
					pthread_mutex_lock( &master->mutex );
					const int npart = ui ?
						ui->npartcounter->value()-1 : 0;
					master->part[npart]->defaultsinstrument();
					master->part[npart]->loadXMLinstrument( f );
					pthread_mutex_unlock( &master->mutex );
					master->applyparameters();
					if( ui )
					{
						ui->npartcounter->do_callback();
						ui->updatepanel();
						ui->refresh_master_ui();
					}
					free( f );
					pthread_mutex_lock( &master->mutex );
					__remote_zasf->sendMessage(
							IdLoadPresetFromFile );
					pthread_mutex_unlock( &master->mutex );
					break;
				}

				default:
					break;
			}
		}
		pthread_mutex_unlock( &__gui_mutex );
	}
	Fl::flush();

	return NULL;
}




int main( int _argc, char * * _argv )
{
	if( _argc < 3 )
	{
		fprintf( stderr, "not enough arguments\n" );
		return( -1 );
	}

#ifdef LMMS_BUILD_WIN32
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif

	__remote_zasf = new remoteZynAddSubFX( atoi( _argv[1] ),
							atoi( _argv[2] ) );

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

	pthread_mutex_init( &__gui_mutex, NULL );

	master = new Master();
	master->swaplr = 0;

	pthread_create( &__gui_thread_handle, NULL, guiThread, NULL );

	remotePluginClient::message m;
	while( ( m = __remote_zasf->receiveMessage() ).id != IdQuit )
	{
		pthread_mutex_lock( &master->mutex );
		__remote_zasf->processMessage( m );
		pthread_mutex_unlock( &master->mutex );
	}

	__exit = 1;

	delete denormalkillbuf;
	delete OscilGen::tmpsmps;
	deleteFFTFREQS( &OscilGen::outoscilFFTfreqs );

	pthread_mutex_destroy( &__gui_mutex );

#ifdef LMMS_BUILD_WIN32
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif

	return 0;
}




