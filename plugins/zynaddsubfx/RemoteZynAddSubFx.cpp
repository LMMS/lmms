/*
 * RemoteZynAddSubFx.cpp - ZynAddSubFx-embedding plugin
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#ifdef LMMS_BUILD_WIN32
#include <winsock2.h>
#endif

#include <queue>

#define BUILD_REMOTE_PLUGIN_CLIENT
#include "note.h"
#include "RemotePlugin.h"
#include "RemoteZynAddSubFx.h"
#include "LocalZynAddSubFx.h"

#include "zynaddsubfx/src/Nio/Nio.h"
#include "zynaddsubfx/src/UI/MasterUI.h"

#include <FL/x.H>


class RemoteZynAddSubFx : public RemotePluginClient, public LocalZynAddSubFx
{
public:
	RemoteZynAddSubFx( int _shm_in, int _shm_out ) :
		RemotePluginClient( _shm_in, _shm_out ),
		LocalZynAddSubFx(),
		m_guiSleepTime( 100 ),
		m_guiExit( false )
	{
		Nio::start();

		setInputCount( 0 );
		sendMessage( IdInitDone );
		waitForMessage( IdInitDone );

		pthread_mutex_init( &m_guiMutex, NULL );
		pthread_create( &m_guiThreadHandle, NULL, guiThread, this );
	}

	virtual ~RemoteZynAddSubFx()
	{
		m_guiExit = true;
#ifdef LMMS_BUILD_WIN32
		Sleep( m_guiSleepTime * 2 );
#else
		usleep( m_guiSleepTime * 2 * 1000 );
#endif

		Nio::stop();
	}

	virtual void updateSampleRate()
	{
		LocalZynAddSubFx::setSampleRate( sampleRate() );
	}

	virtual void updateBufferSize()
	{
		LocalZynAddSubFx::setBufferSize( bufferSize() );
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
			case IdLoadPresetFile:
				pthread_mutex_lock( &m_guiMutex );
				m_guiMessages.push( _m );
				pthread_mutex_unlock( &m_guiMutex );
				break;

			case IdSaveSettingsToFile:
			{
				LocalZynAddSubFx::saveXML( _m.getString() );
				sendMessage( IdSaveSettingsToFile );
				break;
			}

			case IdZasfPresetDirectory:
				LocalZynAddSubFx::setPresetDir( _m.getString() );
				break;

			case IdZasfLmmsWorkingDirectory:
				LocalZynAddSubFx::setLmmsWorkingDir( _m.getString() );
				break;

			case IdZasfSetPitchWheelBendRange:
				LocalZynAddSubFx::setPitchWheelBendRange( _m.getInt() );
				break;

			default:
				return RemotePluginClient::processMessage( _m );
		}
		return true;
	}

	// all functions are called while m_master->mutex is held
	virtual void processMidiEvent( const MidiEvent& event, const f_cnt_t /* _offset */ )
	{
		LocalZynAddSubFx::processMidiEvent( event );
	}


	virtual void process( const sampleFrame * _in, sampleFrame * _out )
	{
		LocalZynAddSubFx::processAudio( _out );
	}

	static void * guiThread( void * _arg )
	{
		RemoteZynAddSubFx * _this =
					static_cast<RemoteZynAddSubFx *>( _arg );

		_this->guiThread();

		return NULL;
	}


private:
	void guiThread();

	const int m_guiSleepTime;

	pthread_t m_guiThreadHandle;
	pthread_mutex_t m_guiMutex;
	std::queue<RemotePluginClient::message> m_guiMessages;
	bool m_guiExit;

} ;




void RemoteZynAddSubFx::guiThread()
{
	int exitProgram = 0;
	MasterUI * ui = NULL;

	while( !m_guiExit )
	{
		if( ui )
		{
			Fl::wait( m_guiSleepTime / 1000.0 );
		}
		else
		{
#ifdef LMMS_BUILD_WIN32
			Sleep( m_guiSleepTime );
#else
			usleep( m_guiSleepTime*1000 );
#endif
		}
		if( exitProgram == 1 )
		{
			pthread_mutex_lock( &m_master->mutex );
			sendMessage( IdHideUI );
			exitProgram = 0;
			pthread_mutex_unlock( &m_master->mutex );
		}
		pthread_mutex_lock( &m_guiMutex );
		while( m_guiMessages.size() )
		{
			RemotePluginClient::message m = m_guiMessages.front();
			m_guiMessages.pop();
			switch( m.id )
			{
				case IdShowUI:
					// we only create GUI
					if( !ui )
					{
						Fl::scheme( "plastic" );
						ui = new MasterUI( m_master, &exitProgram );
					}
					ui->showUI();
					ui->refresh_master_ui();
					break;

				case IdLoadSettingsFromFile:
				{
					LocalZynAddSubFx::loadXML( m.getString() );
					if( ui )
					{
						ui->refresh_master_ui();
					}
					pthread_mutex_lock( &m_master->mutex );
					sendMessage( IdLoadSettingsFromFile );
					pthread_mutex_unlock( &m_master->mutex );
					break;
				}

				case IdLoadPresetFile:
				{
					LocalZynAddSubFx::loadPreset( m.getString(), ui ?
											ui->npartcounter->value()-1 : 0 );
					if( ui )
					{
						ui->npartcounter->do_callback();
						ui->updatepanel();
						ui->refresh_master_ui();
					}
					pthread_mutex_lock( &m_master->mutex );
					sendMessage( IdLoadPresetFile );
					pthread_mutex_unlock( &m_master->mutex );
					break;
				}

				default:
					break;
			}
		}
		pthread_mutex_unlock( &m_guiMutex );
	}
	Fl::flush();

	delete ui;
}




int main( int _argc, char * * _argv )
{
	if( _argc < 3 )
	{
		fprintf( stderr, "not enough arguments\n" );
		return -1;
	}

#ifdef LMMS_BUILD_WIN32
#ifndef __WINPTHREADS_VERSION
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif
#endif


	RemoteZynAddSubFx * remoteZASF =
		new RemoteZynAddSubFx( atoi( _argv[1] ), atoi( _argv[2] ) );

	remoteZASF->run();

	delete remoteZASF;


#ifdef LMMS_BUILD_WIN32
#ifndef __WINPTHREADS_VERSION
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif
#endif

	return 0;
}


#ifdef NTK_GUI
static Fl_Tiled_Image *module_backdrop;
#endif

void set_module_parameters ( Fl_Widget *o )
{
#ifdef NTK_GUI
	o->box( FL_DOWN_FRAME );
	o->align( o->align() | FL_ALIGN_IMAGE_BACKDROP );
	o->color( FL_BLACK );
	o->image( module_backdrop );
	o->labeltype( FL_SHADOW_LABEL );
#else
	o->box( FL_PLASTIC_UP_BOX );
	o->color( FL_CYAN );
	o->labeltype( FL_EMBOSSED_LABEL );
#endif
}

