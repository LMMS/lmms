/*
 * RemoteZynAddSubFx.cpp - ZynAddSubFx-embedding plugin
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "RemoteZynAddSubFx.h"

#include <lmmsconfig.h>
#ifdef LMMS_BUILD_WIN32
#include <winsock2.h>
#endif

#include <queue>
#include "ThreadShims.h"

#undef CursorShape // is, by mistake, not undefed in FL

#include "RemotePluginClient.h"
#include "LocalZynAddSubFx.h"

#include <Nio/Nio.h>
#include <UI/MasterUI.h>

using namespace lmms;

class RemoteZynAddSubFx : public RemotePluginClient, public LocalZynAddSubFx
{
public:
#ifdef SYNC_WITH_SHM_FIFO
	RemoteZynAddSubFx( const std::string& _shm_in, const std::string& _shm_out ) :
		RemotePluginClient( _shm_in, _shm_out ),
#else
	RemoteZynAddSubFx( const char * socketPath ) :
		RemotePluginClient( socketPath ),
#endif
		LocalZynAddSubFx(),
		m_guiSleepTime( 100 ),
		m_guiExit( false )
	{
		Nio::start();

		setInputOutputCount(0, 2);
		sendMessage( IdInitDone );
		waitForMessage( IdInitDone );

		m_messageThread = std::thread{&RemoteZynAddSubFx::messageLoop, this};
	}

	~RemoteZynAddSubFx() override
	{
		m_messageThread.join();
		Nio::stop();
	}

	void updateSampleRate() override
	{
		LocalZynAddSubFx::setSampleRate( sampleRate() );
	}

	void updateBufferSize() override
	{
		LocalZynAddSubFx::setBufferSize( bufferSize() );
	}

	void messageLoop()
	{
		message m;
		while( ( m = receiveMessage() ).id != IdQuit )
		{
			const auto lock = std::lock_guard{m_master->mutex};
			processMessage( m );
		}
		m_guiExit = true;
	}

	bool processMessage( const message & _m ) override
	{
		switch( _m.id )
		{
			case IdQuit:
				break;

			case IdShowUI:
			case IdHideUI:
			case IdLoadSettingsFromFile:
			case IdLoadPresetFile:
				{
					const auto lock = std::lock_guard{m_guiMutex};
					m_guiMessages.push( _m );
				}
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
	void processMidiEvent( const MidiEvent& event, const f_cnt_t /* _offset */ ) override
	{
		LocalZynAddSubFx::processMidiEvent( event );
	}


	void process(const float* in, float* out) override
	{
		(void)in;
		auto accessBuffer = std::array{out, out + bufferSize()};
		auto output = SplitAudioData<float, 2>{accessBuffer.data(), 2, bufferSize()};
		LocalZynAddSubFx::process(output);
	}

	void guiLoop();

private:
	const int m_guiSleepTime;

	std::thread m_messageThread;
	std::mutex m_guiMutex;
	std::queue<RemotePluginClient::message> m_guiMessages;
	bool m_guiExit;

} ;




void RemoteZynAddSubFx::guiLoop()
{
	int exitProgram = 0;
	MasterUI * ui = nullptr;

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
			const auto lock = std::lock_guard{m_master->mutex};
			sendMessage( IdHideUI );
			exitProgram = 0;
		}
		const auto lock = std::lock_guard{m_guiMutex};
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
					const auto lock = std::lock_guard{m_master->mutex};
					sendMessage( IdLoadSettingsFromFile );
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
					const auto lock = std::lock_guard{m_master->mutex};
					sendMessage( IdLoadPresetFile );
					break;
				}

				default:
					break;
			}
		}
	}
	Fl::flush();

	delete ui;
}




int main( int _argc, char * * _argv )
{
#ifdef SYNC_WITH_SHM_FIFO
	if( _argc < 3 )
#else
	if( _argc < 2 )
#endif
	{
		fprintf( stderr, "not enough arguments\n" );
		return -1;
	}

#ifndef LMMS_BUILD_WIN32
	const auto pollParentThread = PollParentThread{};
#endif

#ifdef SYNC_WITH_SHM_FIFO
	RemoteZynAddSubFx * remoteZASF =
		new RemoteZynAddSubFx( _argv[1], _argv[2] );
#else
	auto remoteZASF = new RemoteZynAddSubFx(_argv[1]);
#endif

	remoteZASF->guiLoop();

	delete remoteZASF;

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

