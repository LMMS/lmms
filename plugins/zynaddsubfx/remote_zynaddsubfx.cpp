/*
 * remote_zynaddsubfx.cpp - ZynAddSubFX-embedding plugin
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "note.h"
#include "RemotePlugin.h"
#include "remote_zynaddsubfx.h"

#include "src/Misc/Master.h"
#include "src/Misc/Util.h"
#include "src/Misc/Dump.h"
#include "src/UI/MasterUI.h"

#include <FL/x.H>


class RemoteZynAddSubFX : public RemotePluginClient
{
public:
	RemoteZynAddSubFX( int _shm_in, int _shm_out ) :
		RemotePluginClient( _shm_in, _shm_out ),
		m_guiSleepTime( 100 ),
		m_guiExit( false )
	{
		for( int i = 0; i < NumKeys; ++i )
		{
			m_runningNotes[i] = 0;
		}
		setInputCount( 0 );
		sendMessage( IdInitDone );
		waitForMessage( IdInitDone );

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

		m_master = new Master();
		m_master->swaplr = 0;

		pthread_mutex_init( &m_guiMutex, NULL );
		pthread_create( &m_guiThreadHandle, NULL, guiThread, this );

	}

	virtual ~RemoteZynAddSubFX()
	{
		m_guiExit = true;
#ifdef LMMS_BUILD_WIN32
		Sleep( m_guiSleepTime * 2 );
#else
		usleep( m_guiSleepTime * 2 * 1000 );
#endif

		delete m_master;

		delete[] denormalkillbuf;
		delete[] OscilGen::tmpsmps;
		deleteFFTFREQS( &OscilGen::outoscilFFTfreqs );
		pthread_mutex_destroy( &m_guiMutex );
	}

	virtual void updateSampleRate()
	{
		SAMPLE_RATE = sampleRate();
	}

	virtual void updateBufferSize()
	{
		SOUND_BUFFER_SIZE = bufferSize();
	}

	void run()
	{
		message m;
		while( ( m = receiveMessage() ).id != IdQuit )
		{
			pthread_mutex_lock( &m_master->mutex );
			processMessage( m );
			pthread_mutex_unlock( &m_master->mutex );
		}
	}

	virtual bool processMessage( const message & _m )
	{
		switch( _m.id )
		{
			case IdQuit:
				break;

			case IdShowUI:
			case IdHideUI:
			case IdLoadSettingsFromFile:
			case IdLoadPresetFromFile:
				pthread_mutex_lock( &m_guiMutex );
				m_guiMessages.push( _m );
				pthread_mutex_unlock( &m_guiMutex );
				break;

			case IdSaveSettingsToFile:
			{
				char * name = strdup( _m.getString().c_str() );
				m_master->saveXML( name );
				free( name );
				sendMessage( IdSaveSettingsToFile );
				break;
			}

			case IdZasfPresetDirectory:
				m_presetsDir = _m.getString();
				for( int i = 0; i < MAX_BANK_ROOT_DIRS; ++i )
				{
					if( config.cfg.bankRootDirList[i] == NULL )
					{
						config.cfg.bankRootDirList[i] =
												new char[MAX_STRING_SIZE];
						strcpy( config.cfg.bankRootDirList[i],
												m_presetsDir.c_str() );
						break;
					}
					else if( strcmp( config.cfg.bankRootDirList[i],
										m_presetsDir.c_str() ) == 0 )
					{
						break;
					}
				}
				break;

			default:
				return RemotePluginClient::processMessage( _m );
		}
		return true;
	}

	// all functions are called while m_master->mutex is held
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


	virtual void process( const sampleFrame * _in, sampleFrame * _out )
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

	static void * guiThread( void * _arg );


private:
	std::string m_presetsDir;

	const int m_guiSleepTime;
	int m_runningNotes[NumKeys];
	Master * m_master;

	pthread_t m_guiThreadHandle;
	pthread_mutex_t m_guiMutex;
	std::queue<RemotePluginClient::message> m_guiMessages;
	bool m_guiExit;

} ;



void * RemoteZynAddSubFX::guiThread( void * _arg )
{
	int e;
	MasterUI * ui = NULL;

	RemoteZynAddSubFX * _this = static_cast<RemoteZynAddSubFX *>( _arg );
	Master * master = _this->m_master;

	while( !_this->m_guiExit )
	{
		if( ui )
		{
			Fl::wait( _this->m_guiSleepTime / 1000.0 );
		}
		else
		{
#ifdef LMMS_BUILD_WIN32
			Sleep( _this->m_guiSleepTime );
#else
			usleep( _this->m_guiSleepTime*1000 );
#endif
		}
		pthread_mutex_lock( &_this->m_guiMutex );
		while( _this->m_guiMessages.size() )
		{
			RemotePluginClient::message m = _this->m_guiMessages.front();
			_this->m_guiMessages.pop();
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
					char * f = strdup( m.getString().c_str() );
					pthread_mutex_lock( &master->mutex );
					master->defaults();
					master->loadXML( f );
					pthread_mutex_unlock( &master->mutex );
					master->applyparameters();
					if( ui ) ui->refresh_master_ui();
					unlink( f );
					free( f );
					pthread_mutex_lock( &master->mutex );
					_this->sendMessage( IdLoadSettingsFromFile );
					pthread_mutex_unlock( &master->mutex );
					break;
				}

				case IdLoadPresetFromFile:
				{
					char * f = strdup( m.getString().c_str() );
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
					_this->sendMessage( IdLoadPresetFromFile );
					pthread_mutex_unlock( &master->mutex );
					break;
				}

				default:
					break;
			}
		}
		pthread_mutex_unlock( &_this->m_guiMutex );
	}
	Fl::flush();

	delete ui;

	return NULL;
}




int main( int _argc, char * * _argv )
{
	if( _argc < 3 )
	{
		fprintf( stderr, "not enough arguments\n" );
		return -1;
	}

#ifdef LMMS_BUILD_WIN32
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif


	RemoteZynAddSubFX * remoteZASF =
		new RemoteZynAddSubFX( atoi( _argv[1] ), atoi( _argv[2] ) );

	remoteZASF->run();

	delete remoteZASF;


#ifdef LMMS_BUILD_WIN32
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif

	return 0;
}


