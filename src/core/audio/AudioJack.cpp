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
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QStringList>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiJack.h"

#include <cstdio>


namespace
{
static const QString audioJackClass("audiojack");
static const QString clientNameKey("clientname");
static const QString disconnectedRepresentation("-");

QString getOutputKeyByChannel(size_t channel)
{
	return "output" + QString::number(channel + 1);
}

QString getInputKeyByChannel(size_t channel)
{
	return "input" + QString::number(channel + 1);
}

void printJackStatus(jack_status_t status)
{
	std::fprintf(stderr, "Status: 0x%2.0x\n", static_cast<unsigned int>(status));

	if (status & JackFailure)
	{
		std::fprintf(stderr, "Overall operation failed. JACK dependencies might need to be installed.\n");
	}

	if (status & JackServerFailed)
	{
		std::fprintf(stderr, "Could not connect to JACK server.\n");
	}
}

}

namespace lmms
{

static QString buildChannelSuffix(ch_cnt_t ch)
{
	return (ch % 2 ? "R" : "L") + QString::number(ch / 2 + 1);
}

static QString buildOutputName(ch_cnt_t ch)
{
	return QString("master out ") + buildChannelSuffix(ch);
}

static QString buildInputName(ch_cnt_t ch)
{
	return QString("master in ") + buildChannelSuffix(ch);
}

AudioJack::AudioJack(bool& successful, AudioEngine* audioEngineParam)
	: AudioDevice(
		DEFAULT_CHANNELS,
		audioEngineParam)
	, m_client(nullptr)
	, m_active(false)
	, m_midiClient(nullptr)
	, m_tempOutBufs(new jack_default_audio_sample_t*[channels()])
	, m_outBuf(new SampleFrame[audioEngine()->framesPerPeriod()])
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
	QString clientName = ConfigManager::inst()->value(audioJackClass, clientNameKey);
	if (clientName.isEmpty()) { clientName = "lmms"; }

	const char* serverName = nullptr;
	jack_status_t status;
	m_client = jack_client_open(clientName.toLatin1().constData(), JackNullOption, &status, serverName);
	if (m_client == nullptr)
	{
		std::fprintf(stderr, "jack_client_open() failed, ");
		printJackStatus(status);

		return false;
	}
	if (status & JackNameNotUnique)
	{
		std::printf("there's already a client with name '%s', so unique "
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
		const QString name = buildOutputName(ch);
		m_outputPorts.push_back(
			jack_port_register(m_client, name.toLatin1().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));

		const QString input_name = buildInputName(ch);
		m_inputPorts.push_back(jack_port_register(m_client, input_name.toLatin1().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0));

		if (m_outputPorts.back() == nullptr)
		{
			std::fprintf(stderr, "no more JACK-ports available!\n");
			return false;
		}
	}

	return true;
}




void AudioJack::resizeInputBuffer(jack_nframes_t nframes)
{
	m_inputFrameBuffer.resize(nframes);
}

void AudioJack::attemptToConnect(size_t index, const char *lmms_port_type, const char *source_port, const char *destination_port)
{
	std::printf("Attempting to reconnect %s port %u: %s -> %s", lmms_port_type, static_cast<unsigned int>(index), source_port, destination_port);
	if (!jack_connect(m_client, source_port, destination_port))
	{
		std::printf(" - Success!\n");
	}
	else
	{
		std::printf(" - Failure\n");
	}
}

void AudioJack::attemptToReconnectOutput(size_t outputIndex, const QString& targetPort)
{
	if (outputIndex >= m_outputPorts.size()) { return; }

	if (targetPort == disconnectedRepresentation)
	{
		std::fprintf(stderr, "Output port %u is not connected.\n", static_cast<unsigned int>(outputIndex));
		return;
	}

	auto outputName = jack_port_name(m_outputPorts[outputIndex]);
	auto targetName = targetPort.toLatin1();

	attemptToConnect(outputIndex, "output", outputName, targetName.constData());
}

void AudioJack::attemptToReconnectInput(size_t inputIndex, const QString& sourcePort)
{
	if (inputIndex >= m_inputPorts.size()) { return; }

	if (sourcePort == disconnectedRepresentation)
	{
		std::fprintf(stderr, "Input port %u is not connected.\n", static_cast<unsigned int>(inputIndex));
		return;
	}

	auto inputName = jack_port_name(m_inputPorts[inputIndex]);
	auto sourceName = sourcePort.toLatin1();

	attemptToConnect(inputIndex, "input", sourceName.constData(), inputName);
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
		std::fprintf(stderr, "cannot activate client\n");
		return;
	}

	m_active = true;

	// try to sync JACK's and LMMS's buffer-size
	//	jack_set_buffer_size( m_client, audioEngine()->framesPerPeriod() );

	const auto cm = ConfigManager::inst();

	const auto numberOfChannels = channels();
	for (size_t i = 0; i < numberOfChannels; ++i)
	{
		attemptToReconnectOutput(i, cm->value(audioJackClass, getOutputKeyByChannel(i)));
	}

	for (size_t i = 0; i < numberOfChannels; ++i)
	{
		attemptToReconnectInput(i, cm->value(audioJackClass, getInputKeyByChannel(i)));
	}

	m_stopped = false;
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
	const char* serverName = nullptr;
	jack_status_t status;
	m_client = jack_client_open("LMMS-Setup Dialog", JackNullOption, &status, serverName);
	if (!m_client)
	{
		std::fprintf(stderr, "jack_client_open() failed, ");
		printJackStatus(status);
	}

	QFormLayout * form = new QFormLayout(this);

	// Set the field growth policy to allow fields to expand horizontally
	form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

	const auto cm = ConfigManager::inst();
	QString cn = cm->value(audioJackClass, clientNameKey);
	if (cn.isEmpty()) { cn = "lmms"; }
	m_clientName = new QLineEdit(cn, this);

	form->addRow(tr("Client name"), m_clientName);

	auto buildToolButton = [this](QWidget* parent, const QString& currentSelection, const std::vector<std::string>& names, const QString& filteredLMMSClientName)
	{
		auto toolButton = new QToolButton(parent);
		// Make sure that the tool button will fill out the available space in the form layout
		toolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		toolButton->setPopupMode(QToolButton::InstantPopup);
		toolButton->setText(currentSelection);
		auto menu = AudioJack::setupWidget::buildMenu(toolButton, names, filteredLMMSClientName);
		toolButton->setMenu(menu);

		return toolButton;
	};

	// Outputs
	const auto audioOutputNames = getAudioOutputNames();

	constexpr size_t numberOfOutputChannels = 2;
	for (size_t i = 0; i < numberOfOutputChannels; ++i)
	{
		const auto outputKey = getOutputKeyByChannel(i);
		const auto outputValue = cm->value(audioJackClass, outputKey);
		auto outputDevice = buildToolButton(this, outputValue, audioOutputNames, cn);
		form->addRow(buildOutputName(i) + ":", outputDevice);
		m_outputDevices.push_back(outputDevice);
	}

	// Inputs
	const auto audioInputNames = getAudioInputNames();

	constexpr size_t numberOfInputChannels = 2;
	for (size_t i = 0; i < numberOfInputChannels; ++i)
	{
		const auto inputKey = getInputKeyByChannel(i);
		const auto inputValue = cm->value(audioJackClass, inputKey);
		auto inputDevice = buildToolButton(this, inputValue, audioInputNames, cn);
		form->addRow(buildInputName(i) + ":", inputDevice);
		m_inputDevices.push_back(inputDevice);
	}

	if (m_client != nullptr)
	{
		jack_deactivate(m_client);
		jack_client_close(m_client);
	}
}


void AudioJack::setupWidget::saveSettings()
{
	ConfigManager::inst()->setValue(audioJackClass, clientNameKey, m_clientName->text());

	for (size_t i = 0; i < m_outputDevices.size(); ++i)
	{
		ConfigManager::inst()->setValue(audioJackClass, getOutputKeyByChannel(i), m_outputDevices[i]->text());	
	}

	for (size_t i = 0; i < m_inputDevices.size(); ++i)
	{
		ConfigManager::inst()->setValue(audioJackClass, getInputKeyByChannel(i), m_inputDevices[i]->text());	
	}
}

std::vector<std::string> AudioJack::setupWidget::getAudioPortNames(JackPortFlags portFlags) const
{
	std::vector<std::string> audioPorts;

	// We are using weak_libjack. If JACK is not installed this will result in the client being nullptr.
	// Because jack_get_ports in weak_libjack does not check for nullptr we have to do this here and fail gracefully,
	// i.e. with an empty list of audio ports.
	if (!m_client)
	{
		return audioPorts;
	}

	const char **inputAudioPorts = jack_get_ports(m_client, nullptr, JACK_DEFAULT_AUDIO_TYPE, portFlags);
	if (inputAudioPorts)
	{
		for (int i = 0; inputAudioPorts[i] != nullptr; ++i)
		{
			auto currentPortName = inputAudioPorts[i];

			audioPorts.push_back(currentPortName);
		}
		jack_free(inputAudioPorts); // Remember to free after use
	}

	return audioPorts;
}

std::vector<std::string> AudioJack::setupWidget::getAudioOutputNames() const
{
	return getAudioPortNames(JackPortIsInput);
}

std::vector<std::string> AudioJack::setupWidget::getAudioInputNames() const
{
	return getAudioPortNames(JackPortIsOutput);
}

QMenu* AudioJack::setupWidget::buildMenu(QToolButton* toolButton, const std::vector<std::string>& names, const QString& filteredLMMSClientName)
{
	auto menu = new QMenu(toolButton);
	QMap<QString, QMenu*> clientNameToSubMenuMap;
	QList<QAction*> topLevelActions;
	for (const auto& currentName : names)
	{
		const auto clientNameWithPortName = QString::fromStdString(currentName);

		auto actionLambda = [toolButton, clientNameWithPortName](bool checked)
		{
			toolButton->setText(clientNameWithPortName);
		};

		// Split into individual client name and port name
		const auto list = clientNameWithPortName.split(":");
		if (list.size() == 2)
		{
			const auto& clientName = list[0];
			const auto& portName = list[1];

			if (clientName == filteredLMMSClientName)
			{
				// Prevent loops by not adding port of the LMMS client to the menu
				continue;
			}

			QMenu* clientSubMenu = nullptr;

			auto it = clientNameToSubMenuMap.find(clientName);
			if (it == clientNameToSubMenuMap.end())
			{
				clientSubMenu = new QMenu(menu);
				clientSubMenu->setTitle(clientName);
				clientNameToSubMenuMap.insert(clientName, clientSubMenu);
			}
			else
			{
				clientSubMenu = *it;
			}

			auto action = new QAction(portName, clientSubMenu);
			connect(action, &QAction::triggered, actionLambda);
			clientSubMenu->addAction(action);
		}
		else
		{
			// We cannot split into client and port name. Add the whole thing to the top level menu
			auto action = new QAction(QString::fromStdString(currentName), menu);
			connect(action, &QAction::triggered, actionLambda);
			topLevelActions.append(action);
		}
	}

	// First add the sub menus. By iterating the map they will be sorted automatically
	for (auto it = clientNameToSubMenuMap.begin(); it != clientNameToSubMenuMap.end(); ++it)
	{
		menu->addMenu(it.value());
	}

	// Now add potential top level actions, i.e. the entries which cannot be split at exactly one ":"
	// They must be sorted explicitly
	std::sort(topLevelActions.begin(), topLevelActions.end(), [](QAction* a, QAction* b) { return a->text() < b->text(); });
	menu->addActions(topLevelActions);

	// Add the menu entry which represents the disconnected state at the very end
	auto disconnectedAction = new QAction(disconnectedRepresentation, menu);
	connect(disconnectedAction, &QAction::triggered, [toolButton](bool checked)	{ toolButton->setText(disconnectedRepresentation); });
	menu->addAction(disconnectedAction);

	return menu;
}


} // namespace lmms

#endif // LMMS_HAVE_JACK
