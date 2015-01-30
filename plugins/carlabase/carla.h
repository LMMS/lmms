/*
 * carla.h - Carla for LMMS
 *
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_H
#define CARLA_H

#include "CarlaNative.h"

#include "Instrument.h"
#include "InstrumentView.h"

class QPushButton;

class PLUGIN_EXPORT CarlaInstrument : public Instrument
{
    Q_OBJECT

public:
    static const uint32_t kMaxMidiEvents = 512;

    CarlaInstrument(InstrumentTrack* const instrumentTrack, const Descriptor* const descriptor, const bool isPatchbay);
    virtual ~CarlaInstrument();

    // CarlaNative functions
    uint32_t handleGetBufferSize() const;
    double handleGetSampleRate() const;
    bool handleIsOffline() const;
    const NativeTimeInfo* handleGetTimeInfo() const;
    void handleUiParameterChanged(const uint32_t index, const float value) const;
    void handleUiClosed();
    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt);

    // LMMS functions
    virtual Flags flags() const;
    virtual QString nodeName() const;
    virtual void saveSettings(QDomDocument& doc, QDomElement& parent);
    virtual void loadSettings(const QDomElement& elem);
    virtual void play(sampleFrame* workingBuffer);
    virtual bool handleMidiEvent(const MidiEvent& event, const MidiTime& time, f_cnt_t offset);
    virtual PluginView* instantiateView(QWidget* parent);

signals:
    void uiClosed();

private slots:
    void sampleRateChanged();

private:
    const bool kIsPatchbay;

    NativePluginHandle fHandle;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* fDescriptor;

    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kMaxMidiEvents];
    NativeTimeInfo  fTimeInfo;

    // this is only needed because note-offs are being sent during play
    QMutex fMutex;

    friend class CarlaInstrumentView;
};

class CarlaInstrumentView : public InstrumentView
{
    Q_OBJECT

public:
    CarlaInstrumentView(CarlaInstrument* const instrument, QWidget* const parent);
    virtual ~CarlaInstrumentView();

private slots:
    void toggleUI(bool);
    void uiClosed();

private:
    virtual void modelChanged();
    virtual void timerEvent(QTimerEvent*);

    NativePluginHandle fHandle;
    const NativePluginDescriptor* fDescriptor;
    int fTimerId;

    QPushButton * m_toggleUIButton;
};

#endif
