/*
 * AudioSoundIo.cpp - device-class that performs PCM-output via libsoundio
 *
 * Copyright (c) 2015 Andrew Kelley <superjoe30@gmail.com>
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

#include "AudioSoundIo.h"

#ifdef LMMS_HAVE_SOUNDIO

#include <QFormLayout>
#include <QLineEdit>

#include "Engine.h"
#include "ConfigManager.h"
#include "ComboBox.h"
#include "AudioEngine.h"

namespace lmms
{

AudioSoundIo::AudioSoundIo( bool & outSuccessful, AudioEngine * _audioEngine ) :
	AudioDevice(std::clamp<ch_cnt_t>(
		ConfigManager::inst()->value("audiosoundio", "channels").toInt(),
		DEFAULT_CHANNELS,
		DEFAULT_CHANNELS), _audioEngine)
{
	outSuccessful = false;
	m_soundio = nullptr;
	m_outstream = nullptr;
	m_outBuf = nullptr;
	m_disconnectErr = 0;
	m_outBufFrameIndex = 0;
	m_outBufFramesTotal = 0;
	m_stopped = true;
	m_outstreamStarted = false;

	m_soundio = soundio_create();
	if (!m_soundio)
	{
		fprintf(stderr, "Unable to initialize soundio: out of memory\n");
		return;
	}

	m_soundio->app_name = "LMMS";
	m_soundio->userdata = this;
	m_soundio->on_backend_disconnect = staticOnBackendDisconnect;

	const QString& configBackend = ConfigManager::inst()->value( "audiosoundio", "backend" );
	const QString& configDeviceId = ConfigManager::inst()->value( "audiosoundio", "out_device_id" );
	const QString& configDeviceRaw = ConfigManager::inst()->value( "audiosoundio", "out_device_raw" );

	int outDeviceCount = 0;
	int backendCount = soundio_backend_count(m_soundio);
	for (int i = 0; i < backendCount; i += 1)
	{
		SoundIoBackend backend = soundio_get_backend(m_soundio, i);
		if (configBackend == soundio_backend_name(backend))
		{
			if (!soundio_connect_backend(m_soundio, backend))
			{
				soundio_flush_events(m_soundio);
				if (m_disconnectErr)
				{
					fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(m_disconnectErr));
					return;
				}
				outDeviceCount = soundio_output_device_count(m_soundio);
			}
			break;
		}
	}

	if (outDeviceCount <= 0)
	{
		// try connecting to the default backend
		if (int err = soundio_connect(m_soundio))
		{
			fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(err));
			return;
		}

		soundio_flush_events(m_soundio);
		if (m_disconnectErr)
		{
			fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(m_disconnectErr));
			return;
		}

		outDeviceCount = soundio_output_device_count(m_soundio);

		if (outDeviceCount <= 0)
		{
			fprintf(stderr, "Unable to initialize soundio: no devices found\n");
			return;
		}
	}

	int selected_device_index = soundio_default_output_device_index(m_soundio);

	bool wantRaw = (configDeviceRaw == "yes");
	for (int i = 0; i < outDeviceCount; i += 1)
	{
		SoundIoDevice *device = soundio_get_output_device(m_soundio, i);
		bool isThisOne = (configDeviceId == device->id && wantRaw == device->is_raw);
		soundio_device_unref(device);
		if (isThisOne)
		{
			selected_device_index = i;
			break;
		}
	}

	SoundIoDevice *device = soundio_get_output_device(m_soundio, selected_device_index);
	m_outstream = soundio_outstream_create(device);
	soundio_device_unref(device);

	if (!m_outstream)
	{
		fprintf(stderr, "Unable to initialize soundio: out of memory\n");
		return;
	}

	int currentSampleRate = sampleRate();
	int closestSupportedSampleRate = -1;

	for (int i = 0; i < device->sample_rate_count; i += 1)
	{
		SoundIoSampleRateRange *range = &device->sample_rates[i];
		if (range->min <= currentSampleRate && currentSampleRate <= range->max)
		{
			closestSupportedSampleRate = currentSampleRate;
			break;
		}
		if (closestSupportedSampleRate == -1 ||
			std::abs(range->max - currentSampleRate) < std::abs(closestSupportedSampleRate - currentSampleRate))
		{
			closestSupportedSampleRate = range->max;
		}
	}

	if (closestSupportedSampleRate != currentSampleRate)
	{
		setSampleRate(closestSupportedSampleRate);
		currentSampleRate = closestSupportedSampleRate;
	}

	m_outstream->name = "LMMS";
	m_outstream->software_latency = (double)audioEngine()->framesPerPeriod() / (double)currentSampleRate;
	m_outstream->userdata = this;
	m_outstream->write_callback = staticWriteCallback;
	m_outstream->error_callback = staticErrorCallback;
	m_outstream->underflow_callback = staticUnderflowCallback;
	m_outstream->sample_rate = currentSampleRate;
	m_outstream->layout = *soundio_channel_layout_get_default(channels());
	m_outstream->format = SoundIoFormatFloat32NE;

	if (int err = soundio_outstream_open(m_outstream))
	{
		fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(err));
		return;
	}

	fprintf(stderr, "Output device: '%s' backend: '%s'\n",
			device->name, soundio_backend_name(m_soundio->current_backend));

	outSuccessful = true;
}

void AudioSoundIo::onBackendDisconnect(int err)
{
	m_disconnectErr = err;
}

AudioSoundIo::~AudioSoundIo()
{
	stopProcessing();
	
	if (m_outstream)
	{
		soundio_outstream_destroy(m_outstream);
	}
	
	if (m_soundio)
	{
		soundio_destroy(m_soundio);
		m_soundio = nullptr;
	}
}

void AudioSoundIo::startProcessing()
{
	m_outBufFrameIndex = 0;
	m_outBufFramesTotal = 0;
	m_outBufSize = audioEngine()->framesPerPeriod();

	m_outBuf = new SampleFrame[m_outBufSize];

	if (! m_outstreamStarted)
	{
		if (int err = soundio_outstream_start(m_outstream))
		{
			fprintf(stderr, 
				"AudioSoundIo::startProcessing() :: soundio unable to start stream: %s\n", 
				soundio_strerror(err));
		} else {
			m_outstreamStarted = true;
		}
	}

	m_stopped = false;

	if (int err = soundio_outstream_pause(m_outstream, false))
	{
		m_stopped = true;
		fprintf(stderr, 
			"AudioSoundIo::startProcessing() :: resuming result error: %s\n", 
			soundio_strerror(err));
	}
}

void AudioSoundIo::stopProcessing()
{
	m_stopped = true;
	if (m_outstream)
	{
		if (int err = soundio_outstream_pause(m_outstream, true))
		{
			fprintf(stderr, 
				"AudioSoundIo::stopProcessing() :: pausing result error: %s\n",
				soundio_strerror(err));
		}
	}

	if (m_outBuf)
	{
		delete[] m_outBuf;
		m_outBuf = nullptr;
	}
}

void AudioSoundIo::errorCallback(int err)
{
	fprintf(stderr, "soundio: error streaming: %s\n", soundio_strerror(err));
}

void AudioSoundIo::underflowCallback()
{
	fprintf(stderr, "soundio: buffer underflow reported\n");
}

void AudioSoundIo::writeCallback(int frameCountMin, int frameCountMax)
{
	if (m_stopped) {return;}
	const struct SoundIoChannelLayout *layout = &m_outstream->layout;
	SoundIoChannelArea* areas;
	int bytesPerSample = m_outstream->bytes_per_sample;
	int framesLeft = frameCountMax;

	while (framesLeft > 0)
	{
		int frameCount = framesLeft;
		if (int err = soundio_outstream_begin_write(m_outstream, &areas, &frameCount))
		{
			errorCallback(err);
			return;
		}

		if (!frameCount)
			break;

		
		if (m_stopped)
		{
			for (int channel = 0; channel < layout->channel_count; ++channel)
			{
				memset(areas[channel].ptr, 0, bytesPerSample * frameCount);
				areas[channel].ptr += areas[channel].step * frameCount;
			}
			continue;
		}

		for (int frame = 0; frame < frameCount; frame += 1)
		{
			if (m_outBufFrameIndex >= m_outBufFramesTotal)
			{
				m_outBufFramesTotal = getNextBuffer(m_outBuf);
				if (m_outBufFramesTotal == 0)
				{
					m_stopped = true;
					break;
				}
				m_outBufFrameIndex = 0;
			}

			for (int channel = 0; channel < layout->channel_count; channel += 1)
			{
				float sample = m_outBuf[m_outBufFrameIndex][channel];
				memcpy(areas[channel].ptr, &sample, bytesPerSample);
				areas[channel].ptr += areas[channel].step;
			}
			m_outBufFrameIndex += 1;
		}

		if (int err = soundio_outstream_end_write(m_outstream))
		{
			errorCallback(err);
			return;
		}

		framesLeft -= frameCount;
	}
}

void AudioSoundIoSetupUtil::reconnectSoundIo()
{
	((AudioSoundIo::setupWidget *)m_setupWidget)->reconnectSoundIo();
}

void AudioSoundIoSetupUtil::updateDevices()
{
	((AudioSoundIo::setupWidget *)m_setupWidget)->updateDevices();
}

static void setupWidgetOnBackendDisconnect(SoundIo *soundio, int err)
{
	AudioSoundIo::setupWidget *setupWidget = (AudioSoundIo::setupWidget *)soundio->userdata;
	setupWidget->reconnectSoundIo();
}

static void setup_widget_on_devices_change(SoundIo *soundio)
{
	AudioSoundIo::setupWidget *setupWidget = (AudioSoundIo::setupWidget *)soundio->userdata;
	setupWidget->updateDevices();
}

void AudioSoundIo::setupWidget::reconnectSoundIo()
{
	const QString& configBackend = m_isFirst ?
		ConfigManager::inst()->value( "audiosoundio", "backend" ) : m_backendModel.currentText();
	m_isFirst = false;

	soundio_disconnect(m_soundio);

	int backend_index = m_backendModel.findText(configBackend);
	if (backend_index < 0)
	{
		if (int err = soundio_connect(m_soundio))
		{
			fprintf(stderr, "soundio: unable to connect backend: %s\n", soundio_strerror(err));
			return;
		}
		backend_index = m_backendModel.findText(soundio_backend_name(m_soundio->current_backend));
		assert(backend_index >= 0);
	}
	else
	{
		SoundIoBackend backend = soundio_get_backend(m_soundio, backend_index);
		if (int err = soundio_connect_backend(m_soundio, backend))
		{
			fprintf(stderr, "soundio: unable to connect %s backend: %s\n",
					soundio_backend_name(backend), soundio_strerror(err));
			if (int err = soundio_connect(m_soundio))
			{
				fprintf(stderr, "soundio: unable to connect backend: %s\n", soundio_strerror(err));
				return;
			}
			backend_index = m_backendModel.findText(soundio_backend_name(m_soundio->current_backend));
			assert(backend_index >= 0);
		}
	}
	m_backendModel.setValue(backend_index);

	soundio_flush_events(m_soundio);

	const QString& configDeviceId = ConfigManager::inst()->value( "audiosoundio", "out_device_id" );
	const QString& configDeviceRaw = ConfigManager::inst()->value( "audiosoundio", "out_device_raw" );

	int deviceIndex = m_defaultOutIndex;
	bool wantRaw = (configDeviceRaw == "yes");
	for (int i = 0; i < m_deviceList.length(); i += 1)
	{
		const DeviceId *deviceId = &m_deviceList.at(i);
		if (deviceId->id == configDeviceId && deviceId->is_raw == wantRaw)
		{
			deviceIndex = i;
			break;
		}
	}

	m_deviceModel.setValue(deviceIndex);
}

void AudioSoundIo::setupWidget::updateDevices()
{
	m_defaultOutIndex = soundio_default_output_device_index(m_soundio);

	// get devices for selected backend
	m_deviceModel.clear();
	m_deviceList.clear();
	int outDeviceCount = soundio_output_device_count(m_soundio);
	for (int i = 0; i < outDeviceCount; i += 1)
	{
		SoundIoDevice *device = soundio_get_output_device(m_soundio, i);

		QString raw_text = device->is_raw ? " (raw)" : "";
		QString default_text = (i == m_defaultOutIndex) ? " (default)" : "";

		m_deviceModel.addItem(device->name + raw_text + default_text);
		m_deviceList.append({device->id, device->is_raw});
	}

}

AudioSoundIo::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioSoundIo::name(), _parent )
{
	m_setupUtil.m_setupWidget = this;

	QFormLayout * form = new QFormLayout(this);

	m_backend = new gui::ComboBox( this, "BACKEND" );
	form->addRow(tr("Backend"), m_backend);

	m_device = new gui::ComboBox( this, "DEVICE" );
	form->addRow(tr("Device"), m_device);

	// Setup models
	m_soundio = soundio_create();
	if (!m_soundio)
	{
		fprintf(stderr, "Unable to initialize soundio: out of memory\n");
		return;
	}
	m_soundio->userdata = this;
	m_soundio->on_backend_disconnect = setupWidgetOnBackendDisconnect;
	m_soundio->on_devices_change = setup_widget_on_devices_change;
	m_soundio->app_name = "LMMS";

	int backendCount = soundio_backend_count(m_soundio);
	for (int i = 0; i < backendCount; i += 1)
	{
		SoundIoBackend backend = soundio_get_backend(m_soundio, i);
		m_backendModel.addItem(soundio_backend_name(backend));
	}

	m_isFirst = true;

	reconnectSoundIo();

	[[maybe_unused]] bool ok = connect(&m_backendModel, &ComboBoxModel::dataChanged,
		&m_setupUtil, &AudioSoundIoSetupUtil::reconnectSoundIo);
	assert(ok);

	m_backend->setModel( &m_backendModel );
	m_device->setModel( &m_deviceModel );
}

AudioSoundIo::setupWidget::~setupWidget()
{
	[[maybe_unused]] bool ok = disconnect(&m_backendModel, &ComboBoxModel::dataChanged,
		&m_setupUtil, &AudioSoundIoSetupUtil::reconnectSoundIo);
	assert(ok);
	if (m_soundio)
	{
		soundio_destroy(m_soundio);
		m_soundio = nullptr;
	}
}

void AudioSoundIo::setupWidget::saveSettings()
{
	int deviceIndex = m_deviceModel.value();
	const DeviceId *deviceId = &m_deviceList.at(deviceIndex);

	QString configDeviceRaw = deviceId->is_raw ? "yes" : "no";

	ConfigManager::inst()->setValue( "audiosoundio", "backend", m_backendModel.currentText());
	ConfigManager::inst()->setValue( "audiosoundio", "out_device_id", deviceId->id);
	ConfigManager::inst()->setValue( "audiosoundio", "out_device_raw", configDeviceRaw);
}


} // namespace lmms

#endif // LMMS_HAVE_SOUNDIO
