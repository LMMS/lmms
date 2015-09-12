/*
 * AudioSoundIo.cpp - device-class that performs PCM-output via libsoundio
 *
 * Copyright (c) 2015 Andrew Kelley <superjoe30@gmail.com>
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

#include "AudioSoundIo.h"

#ifdef LMMS_HAVE_SOUNDIO

#include <QLabel>
#include <QLineEdit>

#include "Engine.h"
#include "debug.h"
#include "ConfigManager.h"
#include "gui_templates.h"
#include "templates.h"
#include "ComboBox.h"
#include "LcdSpinBox.h"

AudioSoundIo::AudioSoundIo( bool & out_successful, Mixer * _mixer ) :
    AudioDevice( tLimit<ch_cnt_t>(
        ConfigManager::inst()->value( "audiosoundio", "channels" ).toInt(), DEFAULT_CHANNELS, SURROUND_CHANNELS ),
                                _mixer )
{
    out_successful = false;
    m_soundio = NULL;
    m_outstream = NULL;
    m_disconnect_err = 0;
    m_outBufFrameIndex = 0;
    m_outBufFramesTotal = 0;

    m_soundio = soundio_create();
    if (!m_soundio) {
        fprintf(stderr, "Unable to initialize soundio: out of memory\n");
        return;
    }

    m_soundio->app_name = "LMMS";
    m_soundio->userdata = this;
    m_soundio->on_backend_disconnect = static_on_backend_disconnect;

    const QString& config_backend = ConfigManager::inst()->value( "audiosoundio", "backend" );
    const QString& config_device_id = ConfigManager::inst()->value( "audiosoundio", "out_device_id" );
    const QString& config_device_raw = ConfigManager::inst()->value( "audiosoundio", "out_device_raw" );

    int err;
    int out_device_count = 0;
    int backend_count = soundio_backend_count(m_soundio);
    for (int i = 0; i < backend_count; i += 1) {
        SoundIoBackend backend = soundio_get_backend(m_soundio, i);
        if (config_backend == soundio_backend_name(backend)) {
            if ((err = soundio_connect_backend(m_soundio, backend))) {
                // error occurred, leave out_device_count 0
            } else {
                soundio_flush_events(m_soundio);
                if (m_disconnect_err) {
                    fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(m_disconnect_err));
                    return;
                }
                out_device_count = soundio_output_device_count(m_soundio);
            }
            break;
        }
    }

    if (out_device_count <= 0) {
        // try connecting to the default backend
        if ((err = soundio_connect(m_soundio))) {
            fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(err));
            return;
        }

        soundio_flush_events(m_soundio);
        if (m_disconnect_err) {
            fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(m_disconnect_err));
            return;
        }

        out_device_count = soundio_output_device_count(m_soundio);

        if (out_device_count <= 0) {
            fprintf(stderr, "Unable to initialize soundio: no devices found\n");
            return;
        }
    }

    int selected_device_index = soundio_default_output_device_index(m_soundio);

    bool want_raw = (config_device_raw == "yes");
    for (int i = 0; i < out_device_count; i += 1) {
        SoundIoDevice *device = soundio_get_output_device(m_soundio, i);
        bool is_this_one = (config_device_id == device->id && want_raw == device->is_raw);
        soundio_device_unref(device);
        if (is_this_one) {
            selected_device_index = i;
            break;
        }
    }

    SoundIoDevice *device = soundio_get_output_device(m_soundio, selected_device_index);
    m_outstream = soundio_outstream_create(device);
    soundio_device_unref(device);

    if (!m_outstream) {
        fprintf(stderr, "Unable to initialize soundio: out of memory\n");
        return;
    }

    int current_sample_rate = sampleRate();
    int closest_supported_sample_rate = -1;

    for (int i = 0; i < device->sample_rate_count; i += 1) {
        SoundIoSampleRateRange *range = &device->sample_rates[i];
        if (range->min <= current_sample_rate && current_sample_rate <= range->max) {
            closest_supported_sample_rate = current_sample_rate;
            break;
        }
        if (closest_supported_sample_rate == -1 ||
            abs(range->max - current_sample_rate) < abs(closest_supported_sample_rate - current_sample_rate))
        {
            closest_supported_sample_rate = range->max;
        }
    }

    if (closest_supported_sample_rate != current_sample_rate) {
        setSampleRate(closest_supported_sample_rate);
        current_sample_rate = closest_supported_sample_rate;
    }

    m_outstream->name = "LMMS";
    m_outstream->software_latency = (double)mixer()->framesPerPeriod() / (double)current_sample_rate;
    m_outstream->userdata = this;
    m_outstream->write_callback = static_write_callback;
    m_outstream->error_callback = static_error_callback;
    m_outstream->underflow_callback = static_underflow_callback;
    m_outstream->sample_rate = current_sample_rate;
    m_outstream->layout = *soundio_channel_layout_get_default(channels());
    m_outstream->format = SoundIoFormatFloat32NE;

    if ((err = soundio_outstream_open(m_outstream))) {
        fprintf(stderr, "Unable to initialize soundio: %s\n", soundio_strerror(err));
        return;
    }

    fprintf(stderr, "Output device: '%s' backend: '%s'\n",
            device->name, soundio_backend_name(m_soundio->current_backend));

    out_successful = true;
}

void AudioSoundIo::on_backend_disconnect(int err) {
    m_disconnect_err = err;
}

AudioSoundIo::~AudioSoundIo()
{
    stopProcessing();
    soundio_destroy(m_soundio);
}

void AudioSoundIo::startProcessing()
{
    m_outBufFrameIndex = 0;
    m_outBufFramesTotal = 0;
    m_outBufSize = mixer()->framesPerPeriod();

    m_outBuf = new surroundSampleFrame[m_outBufSize];

    int err;
    if ((err = soundio_outstream_start(m_outstream))) {
        fprintf(stderr, "soundio unable to start stream: %s\n", soundio_strerror(err));
    }
}

void AudioSoundIo::stopProcessing()
{
    soundio_outstream_destroy(m_outstream);
    m_outstream = NULL;

    delete[] m_outBuf;
    m_outBuf = NULL;
}

void AudioSoundIo::error_callback(int err) {
    fprintf(stderr, "soundio: error streaming: %s\n", soundio_strerror(err));
}

void AudioSoundIo::underflow_callback() {
    fprintf(stderr, "soundio: buffer underflow reported\n");
}

void AudioSoundIo::write_callback(int frame_count_min, int frame_count_max) {
    const struct SoundIoChannelLayout *layout = &m_outstream->layout;
    SoundIoChannelArea *areas;
    int bytes_per_sample = m_outstream->bytes_per_sample;
    int err;

    const float gain = mixer()->masterGain();

    int frames_left = frame_count_max;

    while (frames_left > 0) {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(m_outstream, &areas, &frame_count))) {
            error_callback(err);
            return;
        }

        if (!frame_count)
            break;

        for (int frame = 0; frame < frame_count; frame += 1) {
            if (m_outBufFrameIndex >= m_outBufFramesTotal) {
                m_outBufFramesTotal = getNextBuffer(m_outBuf);
                m_outBufFrameIndex = 0;
            }

            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float sample = gain * m_outBuf[m_outBufFrameIndex][channel];
                memcpy(areas[channel].ptr, &sample, bytes_per_sample);
                areas[channel].ptr += areas[channel].step;
            }
            m_outBufFrameIndex += 1;
        }

        if ((err = soundio_outstream_end_write(m_outstream))) {
            error_callback(err);
            return;
        }

        frames_left -= frame_count;
    }
}

void AudioSoundIoSetupUtil::reconnectSoundIo() {
    ((AudioSoundIo::setupWidget *)m_setupWidget)->reconnectSoundIo();
}

void AudioSoundIoSetupUtil::updateDevices() {
    ((AudioSoundIo::setupWidget *)m_setupWidget)->updateDevices();
}

static void setup_widget_on_backend_disconnect(SoundIo *soundio, int err) {
    AudioSoundIo::setupWidget *setup_widget = (AudioSoundIo::setupWidget *)soundio->userdata;
    setup_widget->reconnectSoundIo();
}

static void setup_widget_on_devices_change(SoundIo *soundio) {
    AudioSoundIo::setupWidget *setup_widget = (AudioSoundIo::setupWidget *)soundio->userdata;
    setup_widget->updateDevices();
}

void AudioSoundIo::setupWidget::reconnectSoundIo() {
	const QString& config_backend = m_is_first ?
        ConfigManager::inst()->value( "audiosoundio", "backend" ) : m_backendModel.currentText();
    m_is_first = false;

    soundio_disconnect(m_soundio);

    int err;
    int backend_index = m_backendModel.findText(config_backend);
    if (backend_index < 0) {
        if ((err = soundio_connect(m_soundio))) {
            fprintf(stderr, "soundio: unable to connect backend: %s\n", soundio_strerror(err));
            return;
        }
        backend_index = m_backendModel.findText(soundio_backend_name(m_soundio->current_backend));
        assert(backend_index >= 0);
    } else {
        SoundIoBackend backend = soundio_get_backend(m_soundio, backend_index);
        if ((err = soundio_connect_backend(m_soundio, backend))) {
            fprintf(stderr, "soundio: unable to connect %s backend: %s\n",
                    soundio_backend_name(backend), soundio_strerror(err));
            if ((err = soundio_connect(m_soundio))) {
                fprintf(stderr, "soundio: unable to connect backend: %s\n", soundio_strerror(err));
                return;
            }
            backend_index = m_backendModel.findText(soundio_backend_name(m_soundio->current_backend));
            assert(backend_index >= 0);
        }
    }
    m_backendModel.setValue(backend_index);

    soundio_flush_events(m_soundio);

    const QString& config_device_id = ConfigManager::inst()->value( "audiosoundio", "out_device_id" );
    const QString& config_device_raw = ConfigManager::inst()->value( "audiosoundio", "out_device_raw" );

    int device_index = m_default_out_index;
    bool want_raw = (config_device_raw == "yes");
    for (int i = 0; i < m_device_list.length(); i += 1) {
        const DeviceId *device_id = &m_device_list.at(i);
        if (device_id->id == config_device_id && device_id->is_raw == want_raw) {
            device_index = i;
            break;
        }
    }

    m_deviceModel.setValue(device_index);
}

void AudioSoundIo::setupWidget::updateDevices() {
    m_default_out_index = soundio_default_output_device_index(m_soundio);

    // get devices for selected backend
    m_deviceModel.clear();
    m_device_list.clear();
    int out_device_count = soundio_output_device_count(m_soundio);
    for (int i = 0; i < out_device_count; i += 1) {
        SoundIoDevice *device = soundio_get_output_device(m_soundio, i);

        QString raw_text = device->is_raw ? " (raw)" : "";
        QString default_text = (i == m_default_out_index) ? " (default)" : "";

        m_deviceModel.addItem(device->name + raw_text + default_text);
        m_device_list.append({device->id, device->is_raw});
    }

}

AudioSoundIo::setupWidget::setupWidget( QWidget * _parent ) :
    AudioDeviceSetupWidget( AudioSoundIo::name(), _parent )
{
    m_setupUtil.m_setupWidget = this;

    m_backend = new ComboBox( this, "BACKEND" );
    m_backend->setGeometry( 64, 15, 260, 20 );

    QLabel * backend_lbl = new QLabel( tr( "BACKEND" ), this );
    backend_lbl->setFont( pointSize<7>( backend_lbl->font() ) );
    backend_lbl->move( 8, 18 );

    m_device = new ComboBox( this, "DEVICE" );
    m_device->setGeometry( 64, 35, 260, 20 );

    QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
    dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
    dev_lbl->move( 8, 38 );

    // Setup models
    m_soundio = soundio_create();
    if (!m_soundio) {
        fprintf(stderr, "Unable to initialize soundio: out of memory\n");
        return;
    }
    m_soundio->userdata = this;
    m_soundio->on_backend_disconnect = setup_widget_on_backend_disconnect;
    m_soundio->on_devices_change = setup_widget_on_devices_change;
    m_soundio->app_name = "LMMS";

    int backend_count = soundio_backend_count(m_soundio);
    for (int i = 0; i < backend_count; i += 1) {
        SoundIoBackend backend = soundio_get_backend(m_soundio, i);
        m_backendModel.addItem(soundio_backend_name(backend));
    }

    m_is_first = true;

    reconnectSoundIo();

    bool ok = connect( &m_backendModel, SIGNAL( dataChanged() ), &m_setupUtil, SLOT( reconnectSoundIo() ) );
    assert(ok);

    m_backend->setModel( &m_backendModel );
    m_device->setModel( &m_deviceModel );
}

AudioSoundIo::setupWidget::~setupWidget() {
    bool ok = disconnect( &m_backendModel, SIGNAL( dataChanged() ), &m_setupUtil, SLOT( reconnectSoundIo() ) );
    assert(ok);
    soundio_destroy(m_soundio);
}

void AudioSoundIo::setupWidget::saveSettings() {
    int device_index = m_deviceModel.value();
    const DeviceId *device_id = &m_device_list.at(device_index);

    QString config_device_raw = device_id->is_raw ? "yes" : "no";

    ConfigManager::inst()->setValue( "audiosoundio", "backend", m_backendModel.currentText());
    ConfigManager::inst()->setValue( "audiosoundio", "out_device_id", device_id->id);
    ConfigManager::inst()->setValue( "audiosoundio", "out_device_raw", config_device_raw);
}

#endif
