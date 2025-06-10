/*
 * AudioJack.cpp - support for JACK transport
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioJack.h"

#ifdef LMMS_HAVE_JACK

#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiJack.h"

namespace lmms
{


AudioJack::AudioJack(bool& successful, AudioEngine* audioEngineParam)
	: AudioDevice(
		DEFAULT_CHANNELS,
		audioEngineParam)
	, m_client(nullptr)
	, m_active(false)
	, m_midiClient(nullptr)
	, m_tempOutBufs(new jack_default_audio_sample_t*[channels()])
	, m_outBuf(new SampleFrame[audioEngine()->framesPerPeriod()])
	, m_inBuf(new SampleFrame[audioEngine()->framesPerPeriod()])
	, m_framesDoneInCurBuf(0)
	, m_framesToDoInCurBuf(0)
{
	m_stopped = true;

	successful = initJackClient();
	if (successful) {
		connect(this, SIGNAL(zombified()), this, SLOT(restartAfterZombified()), Qt::QueuedConnection);
	}

	m_supportsCapture = true;
}




AudioJack::~AudioJack()
{
	AudioJack::stopProcessing();
#ifdef AUDIO_BUS_HANDLE_SUPPORT
	while (m_portMap.size())
	{
		unregisterPort(m_portMap.begin().key());
	}
#endif

	if (m_client != nullptr)
	{
		if (m_active) { jack_deactivate(m_client); }
		jack_client_close(m_client);
	}

	delete[] m_tempOutBufs;

	delete[] m_outBuf;
	delete[] m_inBuf;
}




void AudioJack::restartAfterZombified()
{
	if (initJackClient())
	{
		m_active = false;
		startProcessing();
		QMessageBox::information(gui::getGUI()->mainWindow(), tr("JACK client restarted"),
			tr(	"LMMS was kicked by JACK for some reason. "
				"Therefore the JACK backend of LMMS has been "
				"restarted. You will have to make manual "
				"connections again."));
	}
	else
	{
		QMessageBox::information(gui::getGUI()->mainWindow(), tr("JACK server down"),
			tr(	"The JACK server seems to have been shutdown "
				"and starting a new instance failed. "
				"Therefore LMMS is unable to proceed. "
				"You should save your project and restart "
				"JACK and LMMS."));
	}
}




AudioJack* AudioJack::addMidiClient(MidiJack* midiClient)
{
	if (m_client == nullptr) { return nullptr; }

	m_midiClient = midiClient;

	return this;
}




bool AudioJack::initJackClient()
{
	QString clientName = ConfigManager::inst()->value("audiojack", "clientname");
	if (clientName.isEmpty()) { clientName = "lmms"; }

	const char* serverName = nullptr;
	jack_status_t status;
	m_client = jack_client_open(clientName.toLatin1().constData(), JackNullOption, &status, serverName);
	if (m_client == nullptr)
	{
		printf("jack_client_open() failed, status 0x%2.0x\n", status);
		if (status & JackServerFailed) { printf("Could not connect to JACK server.\n"); }
		return false;
	}
	if (status & JackNameNotUnique)
	{
		printf(	"there's already a client with name '%s', so unique "
				"name '%s' was assigned\n",
				clientName.toLatin1().constData(), jack_get_client_name(m_client));
	}

	resizeInputBuffer(jack_get_buffer_size(m_client));

	// set buffer-size callback
	jack_set_buffer_size_callback(m_client,
		[](jack_nframes_t nframes, void* udata) -> int {
			static_cast<AudioJack*>(udata)->resizeInputBuffer(nframes);
			return 0;
		},
		this);

	// set process-callback
	jack_set_process_callback(m_client, staticProcessCallback, this);

	// set shutdown-callback
	jack_on_shutdown(m_client, shutdownCallback, this);

	if (jack_get_sample_rate(m_client) != sampleRate()) { setSampleRate(jack_get_sample_rate(m_client)); }

	for (ch_cnt_t ch = 0; ch < channels(); ++ch)
	{
		QString name = QString("master out ") + ((ch % 2) ? "R" : "L") + QString::number(ch / 2 + 1);
		m_outputPorts.push_back(
			jack_port_register(m_client, name.toLatin1().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));

		QString input_name = QString("master in ") + ((ch % 2) ? "R" : "L") + QString::number(ch / 2 + 1);
		m_inputPorts.push_back(jack_port_register(m_client, input_name.toLatin1().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0));

		if (m_outputPorts.back() == nullptr)
		{
			printf("no more JACK-ports available!\n");
			return false;
		}
	}

	return true;
}




void AudioJack::resizeInputBuffer(jack_nframes_t nframes)
{
	m_inputFrameBuffer.resize(nframes);
}




void AudioJack::startProcessing()
{
	if (m_active || m_client == nullptr)
	{
		m_stopped = false;
		return;
	}

	if (jack_activate(m_client))
	{
		printf("cannot activate client\n");
		return;
	}

	m_active = true;

	// try to sync JACK's and LMMS's buffer-size
	//	jack_set_buffer_size( m_client, audioEngine()->framesPerPeriod() );

	const char** ports = jack_get_ports(m_client, nullptr, nullptr, JackPortIsPhysical | JackPortIsInput);
	if (ports == nullptr)
	{
		printf("no physical playback ports. you'll have to do "
			   "connections at your own!\n");
	}
	else
	{
		for (ch_cnt_t ch = 0; ch < channels(); ++ch)
		{
			if (jack_connect(m_client, jack_port_name(m_outputPorts[ch]), ports[ch]))
			{
				printf("cannot connect output ports. you'll "
					   "have to do connections at your own!\n");
			}
		}
	}

	m_stopped = false;
	jack_free(ports);
}




void AudioJack::stopProcessing()
{
	m_stopped = true;
}

void AudioJack::registerPort(AudioBusHandle* port)
{
#ifdef AUDIO_BUS_HANDLE_SUPPORT
	// make sure, port is not already registered
	unregisterPort(port);
	const QString name[2] = {port->name() + " L", port->name() + " R"};

	for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
	{
		m_portMap[port].ports[ch] = jack_port_register(
			m_client, name[ch].toLatin1().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	}
#else
	(void)port;
#endif
}




void AudioJack::unregisterPort(AudioBusHandle* port)
{
#ifdef AUDIO_BUS_HANDLE_SUPPORT
	if (m_portMap.contains(port))
	{
		for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
		{
			if (m_portMap[port].ports[ch] != nullptr) { jack_port_unregister(m_client, m_portMap[port].ports[ch]); }
		}
		m_portMap.erase(m_portMap.find(port));
	}
#else
	(void)port;
#endif
}

void AudioJack::renamePort(AudioBusHandle* port)
{
#ifdef AUDIO_BUS_HANDLE_SUPPORT
	if (m_portMap.contains(port))
	{
		const QString name[2] = {port->name() + " L", port->name() + " R"};
		for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
		{
#ifdef LMMS_HAVE_JACK_PRENAME
			jack_port_rename(m_client, m_portMap[port].ports[ch], name[ch].toLatin1().constData());
#else
			jack_port_set_name(m_portMap[port].ports[ch], name[ch].toLatin1().constData());
#endif
		}
	}
#else
	(void)port;
#endif // AUDIO_BUS_HANDLE_SUPPORT
}




int AudioJack::processCallback(jack_nframes_t nframes)
{
	// do midi processing first so that midi input can
	// add to the following sound processing
	if (m_midiClient && nframes > 0)
	{
		m_midiClient.load()->JackMidiRead(nframes);
		m_midiClient.load()->JackMidiWrite(nframes);
	}

	for (int c = 0; c < channels(); ++c)
	{
		m_tempOutBufs[c] = (jack_default_audio_sample_t*)jack_port_get_buffer(m_outputPorts[c], nframes);
	}

#ifdef AUDIO_BUS_HANDLE_SUPPORT
	const int frames = std::min<int>(nframes, audioEngine()->framesPerPeriod());
	for (JackPortMap::iterator it = m_portMap.begin(); it != m_portMap.end(); ++it)
	{
		for (ch_cnt_t ch = 0; ch < channels(); ++ch)
		{
			if (it.value().ports[ch] == nullptr) { continue; }
			jack_default_audio_sample_t* buf
				= (jack_default_audio_sample_t*)jack_port_get_buffer(it.value().ports[ch], nframes);
			for (int frame = 0; frame < frames; ++frame)
			{
				buf[frame] = it.key()->buffer()[frame][ch];
			}
		}
	}
#endif

	jack_nframes_t done = 0;
	while (done < nframes && !m_stopped)
	{
		jack_nframes_t todo = std::min<jack_nframes_t>(nframes - done, m_framesToDoInCurBuf - m_framesDoneInCurBuf);
		for (int c = 0; c < channels(); ++c)
		{
			jack_default_audio_sample_t* o = m_tempOutBufs[c];
			for (jack_nframes_t frame = 0; frame < todo; ++frame)
			{
				o[done + frame] = m_outBuf[m_framesDoneInCurBuf + frame][c];
			}
		}
		done += todo;
		m_framesDoneInCurBuf += todo;
		if (m_framesDoneInCurBuf == m_framesToDoInCurBuf)
		{
			m_framesToDoInCurBuf = getNextBuffer(m_outBuf);
			m_framesDoneInCurBuf = 0;
			if (!m_framesToDoInCurBuf)
			{
				m_stopped = true;
				break;
			}
		}
	}

	if (nframes != done)
	{
		for (int c = 0; c < channels(); ++c)
		{
			jack_default_audio_sample_t* b = m_tempOutBufs[c] + done;
			memset(b, 0, sizeof(*b) * (nframes - done));
		}
	}

	for (int c = 0; c < channels(); ++c)
	{
		jack_default_audio_sample_t* jack_input_buffer = (jack_default_audio_sample_t*) jack_port_get_buffer(m_inputPorts[c], nframes);

		for (jack_nframes_t frame = 0; frame < nframes; frame++)
		{
			m_inputFrameBuffer[frame][c] = static_cast<sample_t>(jack_input_buffer[frame]);
		}
	}
	audioEngine()->pushInputFrames (m_inputFrameBuffer.data(), nframes);
	return 0;
}




int AudioJack::staticProcessCallback(jack_nframes_t nframes, void* udata)
{
	return static_cast<AudioJack*>(udata)->processCallback(nframes);
}




void AudioJack::shutdownCallback(void* udata)
{
	auto thisClass = static_cast<AudioJack*>(udata);
	thisClass->m_client = nullptr;
	emit thisClass->zombified();
}




AudioJack::setupWidget::setupWidget(QWidget* parent)
	: AudioDeviceSetupWidget(AudioJack::name(), parent)
{
	QFormLayout * form = new QFormLayout(this);

	QString cn = ConfigManager::inst()->value("audiojack", "clientname");
	if (cn.isEmpty()) { cn = "lmms"; }
	m_clientName = new QLineEdit(cn, this);

	form->addRow(tr("Client name"), m_clientName);
}




void AudioJack::setupWidget::saveSettings()
{
	ConfigManager::inst()->setValue("audiojack", "clientname", m_clientName->text());
}


} // namespace lmms

#endif // LMMS_HAVE_JACK
