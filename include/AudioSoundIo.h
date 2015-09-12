/*
 * AudioSoundIo.h - device-class that performs PCM-output via libsoundio
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

#ifndef AUDIO_SOUNDIO_H
#define AUDIO_SOUNDIO_H

#include <QtCore/QObject>

#include "lmmsconfig.h"
#include "ComboBoxModel.h"

#ifdef LMMS_HAVE_SOUNDIO

#include <soundio/soundio.h>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

class ComboBox;
class LcdSpinBox;

// Exists only to work around "Error: Meta object features not supported for nested classes"
class AudioSoundIoSetupUtil : public QObject
{
    Q_OBJECT
public:
    void *m_setupWidget;
public slots:
    void updateDevices();
    void reconnectSoundIo();
};

class AudioSoundIo : public AudioDevice
{
public:
    AudioSoundIo( bool & _success_ful, Mixer* mixer );
    virtual ~AudioSoundIo();

    inline static QString name()
    {
        return QT_TRANSLATE_NOOP( "setupWidget", "soundio" );
    }


    int process_callback( const float *_inputBuffer,
        float * _outputBuffer,
        unsigned long _framesPerBuffer );


    class setupWidget : public AudioDeviceSetupWidget
    {
    public:
        setupWidget( QWidget * _parent );
        virtual ~setupWidget();

        virtual void saveSettings();

        void updateDevices();
        void reconnectSoundIo();

        AudioSoundIoSetupUtil m_setupUtil;
        ComboBox * m_backend;
        ComboBox * m_device;

        ComboBoxModel m_backendModel;
        ComboBoxModel m_deviceModel;

        SoundIo * m_soundio;

        struct DeviceId {
            QString id;
            bool is_raw;
        };
        QList<DeviceId> m_device_list;

        int m_default_out_index;
        bool m_is_first;

    } ;

private:
    virtual void startProcessing();
    virtual void stopProcessing();

    SoundIo *m_soundio;
    SoundIoOutStream *m_outstream;

    surroundSampleFrame * m_outBuf;
    int m_outBufSize;
    fpp_t m_outBufFramesTotal;
    fpp_t m_outBufFrameIndex;

    int m_disconnect_err;
    void on_backend_disconnect(int err);

    void write_callback(int frame_count_min, int frame_count_max);
    void error_callback(int err);
    void underflow_callback();

    static void static_write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
        return ((AudioSoundIo *)outstream->userdata)->write_callback(frame_count_min, frame_count_max);
    }
    static void static_error_callback(SoundIoOutStream *outstream, int err) {
        return ((AudioSoundIo *)outstream->userdata)->error_callback(err);
    }
    static void static_underflow_callback(SoundIoOutStream *outstream) {
        return ((AudioSoundIo *)outstream->userdata)->underflow_callback();
    }
    static void static_on_backend_disconnect(SoundIo *soundio, int err) {
        return ((AudioSoundIo *)soundio->userdata)->on_backend_disconnect(err);
    }

};

#endif

#endif
