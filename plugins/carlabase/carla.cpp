/*
 * carla.cpp - Carla for LMMS
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

#include "carla.h"

#define REAL_BUILD // FIXME this shouldn't be needed
#include "CarlaHost.h"

#include "engine.h"
#include "song.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"

#include <QApplication>
#include <QFileDialog>
#include <QPushButton>
#include <QTimerEvent>
#include <QVBoxLayout>

#include <cstring>

#include "embed.cpp"

// this doesn't seem to be defined anywhere
static const double ticksPerBeat = 48.0;

/*
 * Current TODO items:
 *  - get plugin instance name (to use in external window title)
 *  - offline mode change callback
 *  - midi output
 *
 * All other items are to be done in Carla itself.
 */

// -----------------------------------------------------------------------

#define handlePtr ((CarlaInstrument*)handle)

static uint32_t host_get_buffer_size(NativeHostHandle handle)
{
    return handlePtr->handleGetBufferSize();
}

static double host_get_sample_rate(NativeHostHandle handle)
{
    return handlePtr->handleGetSampleRate();
}

static bool host_is_offline(NativeHostHandle handle)
{
    return handlePtr->handleIsOffline();
}

static const NativeTimeInfo* host_get_time_info(NativeHostHandle handle)
{
    return handlePtr->handleGetTimeInfo();
}

static bool host_write_midi_event(NativeHostHandle, const NativeMidiEvent*)
{
    return false; // unsupported?
}

static void host_ui_parameter_changed(NativeHostHandle handle, uint32_t index, float value)
{
    handlePtr->handleUiParameterChanged(index, value);
}

static void host_ui_custom_data_changed(NativeHostHandle handle, const char* key, const char* value)
{
    // unused
}

static void host_ui_closed(NativeHostHandle handle)
{
    handlePtr->handleUiClosed();
}

static intptr_t host_dispatcher(NativeHostHandle handle, NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
}

#undef handlePtr

// -----------------------------------------------------------------------

static const char* host_ui_open_file(NativeHostHandle, bool isDir, const char* title, const char* filter)
{
    static QByteArray retStr;
    const QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

    retStr = QFileDialog::getOpenFileName(QApplication::activeWindow(), title, "", filter, NULL, options).toUtf8();

    return retStr.isEmpty() ? NULL : retStr.constData();
}

static const char* host_ui_save_file(NativeHostHandle, bool isDir, const char* title, const char* filter)
{
    static QByteArray retStr;
    const QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

    retStr = QFileDialog::getSaveFileName(QApplication::activeWindow(), title, "", filter, NULL, options).toUtf8();

    return retStr.isEmpty() ? NULL : retStr.constData();
}

// -----------------------------------------------------------------------

CARLA_EXPORT
const NativePluginDescriptor* carla_get_native_patchbay_plugin();

CARLA_EXPORT
const NativePluginDescriptor* carla_get_native_rack_plugin();

// -----------------------------------------------------------------------

CarlaInstrument::CarlaInstrument(InstrumentTrack* const instrumentTrack, const Descriptor* const descriptor, const bool isPatchbay)
    : Instrument(instrumentTrack, descriptor),
      kIsPatchbay(isPatchbay),
      fHandle(NULL),
      fDescriptor(isPatchbay ? carla_get_native_patchbay_plugin() : carla_get_native_rack_plugin()),
      fMidiEventCount(0)
{
    fHost.handle      = this;
    fHost.uiName      = NULL;
    fHost.uiParentId  = 0;

    // figure out prefix from dll filename
    QString dllName(carla_get_library_filename());

#if defined(CARLA_OS_LINUX)
    fHost.resourceDir = strdup(QString(dllName.split("/lib/carla")[0] + "/share/carla/resources/").toUtf8().constData());
#else
    fHost.resourceDir = NULL;
#endif

    fHost.get_buffer_size        = host_get_buffer_size;
    fHost.get_sample_rate        = host_get_sample_rate;
    fHost.is_offline             = host_is_offline;
    fHost.get_time_info          = host_get_time_info;
    fHost.write_midi_event       = host_write_midi_event;
    fHost.ui_parameter_changed   = host_ui_parameter_changed;
    fHost.ui_custom_data_changed = host_ui_custom_data_changed;
    fHost.ui_closed              = host_ui_closed;
    fHost.ui_open_file           = host_ui_open_file;
    fHost.ui_save_file           = host_ui_save_file;
    fHost.dispatcher             = host_dispatcher;

    std::memset(&fTimeInfo, 0, sizeof(NativeTimeInfo));
    fTimeInfo.bbt.valid = true; // always valid

    fHandle = fDescriptor->instantiate(&fHost);
    Q_ASSERT(fHandle != NULL);

    if (fHandle != NULL && fDescriptor->activate != NULL)
        fDescriptor->activate(fHandle);

    // we need a play-handle which cares for calling play()
    InstrumentPlayHandle * iph = new InstrumentPlayHandle( this );
    engine::mixer()->addPlayHandle( iph );

    connect(engine::mixer(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

CarlaInstrument::~CarlaInstrument()
{
    engine::mixer()->removePlayHandles( instrumentTrack() );

    if (fHost.resourceDir != NULL)
    {
        std::free((char*)fHost.resourceDir);
        fHost.resourceDir = NULL;
    }

    if (fHost.uiName != NULL)
    {
        std::free((char*)fHost.uiName);
        fHost.uiName = NULL;
    }

    if (fHandle == NULL)
        return;

    if (fDescriptor->deactivate != NULL)
        fDescriptor->deactivate(fHandle);

    if (fDescriptor->cleanup != NULL)
        fDescriptor->cleanup(fHandle);

    fHandle = NULL;
}

// -------------------------------------------------------------------

uint32_t CarlaInstrument::handleGetBufferSize() const
{
    return engine::mixer()->framesPerPeriod();
}

double CarlaInstrument::handleGetSampleRate() const
{
    return engine::mixer()->processingSampleRate();
}

bool CarlaInstrument::handleIsOffline() const
{
    return false; // TODO
}

const NativeTimeInfo* CarlaInstrument::handleGetTimeInfo() const
{
    return &fTimeInfo;
}

void CarlaInstrument::handleUiParameterChanged(const uint32_t /*index*/, const float /*value*/) const
{
}

void CarlaInstrument::handleUiClosed()
{
    emit uiClosed();
}

intptr_t CarlaInstrument::handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
{
    intptr_t ret = 0;

    switch (opcode)
    {
    case NATIVE_HOST_OPCODE_NULL:
        break;
    case NATIVE_HOST_OPCODE_UPDATE_PARAMETER:
    case NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM:
    case NATIVE_HOST_OPCODE_RELOAD_PARAMETERS:
    case NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
    case NATIVE_HOST_OPCODE_RELOAD_ALL:
        // nothing
        break;
    case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
        handleUiClosed();
        break;
    }

    return ret;

    // unused for now
    (void)index; (void)value; (void)ptr; (void)opt;
}

// -------------------------------------------------------------------

Instrument::Flags CarlaInstrument::flags() const
{
    return IsSingleStreamed|IsMidiBased|IsNotBendable;
}

QString CarlaInstrument::nodeName() const
{
    return  descriptor()->name;
}

void CarlaInstrument::saveSettings(QDomDocument& doc, QDomElement& parent)
{
    if (fHandle == NULL || fDescriptor->get_state == NULL)
        return;

    char* const state = fDescriptor->get_state(fHandle);

    if (state == NULL)
        return;

    QDomDocument carlaDoc("carla");

    if (carlaDoc.setContent(QString(state)))
    {
        QDomNode n = doc.importNode(carlaDoc.documentElement(), true);
        parent.appendChild(n);
    }

    std::free(state);
}

void CarlaInstrument::loadSettings(const QDomElement& elem)
{
    if (fHandle == NULL || fDescriptor->set_state == NULL)
        return;

    QDomDocument carlaDoc("carla");
    carlaDoc.appendChild(carlaDoc.importNode(elem.firstChildElement(), true ));

    fDescriptor->set_state(fHandle, carlaDoc.toString(0).toUtf8().constData());
}

void CarlaInstrument::play(sampleFrame* workingBuffer)
{
    const uint bufsize = engine::mixer()->framesPerPeriod();

    std::memset(workingBuffer, 0, sizeof(sample_t)*bufsize*DEFAULT_CHANNELS);

    if (fHandle == NULL)
    {
        instrumentTrack()->processAudioBuffer(workingBuffer, bufsize, NULL);
        return;
    }

    // set time info
    song* const s = engine::getSong();
    fTimeInfo.playing  = s->isPlaying();
    fTimeInfo.frame    = s->getPlayPos(s->playMode()).frames(engine::framesPerTick());
    fTimeInfo.usecs    = s->getMilliseconds()*1000;
    fTimeInfo.bbt.bar  = s->getTacts() + 1;
    fTimeInfo.bbt.beat = s->getBeat() + 1;
    fTimeInfo.bbt.tick = s->getBeatTicks();
    fTimeInfo.bbt.barStartTick   = ticksPerBeat*s->getTimeSigModel().getNumerator()*s->getTacts();
    fTimeInfo.bbt.beatsPerBar    = s->getTimeSigModel().getNumerator();
    fTimeInfo.bbt.beatType       = s->getTimeSigModel().getDenominator();
    fTimeInfo.bbt.ticksPerBeat   = ticksPerBeat;
    fTimeInfo.bbt.beatsPerMinute = s->getTempo();

    float buf1[bufsize];
    float buf2[bufsize];
    float* rBuf[] = { buf1, buf2 };
    std::memset(buf1, 0, sizeof(float)*bufsize);
    std::memset(buf2, 0, sizeof(float)*bufsize);

    {
        const QMutexLocker ml(&fMutex);
        fDescriptor->process(fHandle, rBuf, rBuf, bufsize, fMidiEvents, fMidiEventCount);
        fMidiEventCount = 0;
    }

    for (uint i=0; i < bufsize; ++i)
    {
        workingBuffer[i][0] = buf1[i];
        workingBuffer[i][1] = buf2[i];
    }

    instrumentTrack()->processAudioBuffer(workingBuffer, bufsize, NULL);
}

bool CarlaInstrument::handleMidiEvent(const MidiEvent& event, const MidiTime&, f_cnt_t offset)
{
    const QMutexLocker ml(&fMutex);

    if (fMidiEventCount >= kMaxMidiEvents)
        return false;

    NativeMidiEvent& nEvent(fMidiEvents[fMidiEventCount++]);
    std::memset(&nEvent, 0, sizeof(NativeMidiEvent));

    nEvent.port    = 0;
    nEvent.time    = offset;
    nEvent.data[0] = event.type() | (event.channel() & 0x0F);

    switch (event.type())
    {
    case MidiNoteOn:
        if (event.velocity() > 0)
        {
            if (event.key() < 0 || event.key() > MidiMaxKey)
                break;

            nEvent.data[1] = event.key();
            nEvent.data[2] = event.velocity();
            nEvent.size    = 3;
            break;
        }
        else
        {
            nEvent.data[0] = MidiNoteOff | (event.channel() & 0x0F);
            // nobreak
        }

    case MidiNoteOff:
        if (event.key() < 0 || event.key() > MidiMaxKey)
            break;

        nEvent.data[1] = event.key();
        nEvent.data[2] = event.velocity();
        nEvent.size    = 3;
        break;

    case MidiKeyPressure:
        nEvent.data[1] = event.key();
        nEvent.data[2] = event.velocity();
        nEvent.size    = 3;
        break;

    case MidiControlChange:
        nEvent.data[1] = event.controllerNumber();
        nEvent.data[2] = event.controllerValue();
        nEvent.size    = 3;
        break;

    case MidiProgramChange:
        nEvent.data[1] = event.program();
        nEvent.size    = 2;
        break;

    case MidiChannelPressure:
        nEvent.data[1] = event.channelPressure();
        nEvent.size    = 2;
        break;

    case MidiPitchBend:
        nEvent.data[1] = event.pitchBend() & 0x7f;
        nEvent.data[2] = event.pitchBend() >> 7;
        nEvent.size    = 3;
        break;

    default:
        // unhandled
        --fMidiEventCount;
        break;
    }

    return true;
}

PluginView* CarlaInstrument::instantiateView(QWidget* parent)
{
    if (QWidget* const window = parent->window())
        fHost.uiParentId = window->winId();
    else
        fHost.uiParentId = 0;

    std::free((char*)fHost.uiName);

    // TODO - get plugin instance name
    //fHost.uiName = strdup(parent->windowTitle().toUtf8().constData());
    fHost.uiName = strdup(kIsPatchbay ? "CarlaPatchbay-LMMS" : "CarlaRack-LMMS");

    return new CarlaInstrumentView(this, parent);
}

void CarlaInstrument::sampleRateChanged()
{
    fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, handleGetSampleRate());
}

// -------------------------------------------------------------------

CarlaInstrumentView::CarlaInstrumentView(CarlaInstrument* const instrument, QWidget* const parent)
    : InstrumentView(instrument, parent),
      fHandle(instrument->fHandle),
      fDescriptor(instrument->fDescriptor),
      fTimerId(fHandle != NULL && fDescriptor->ui_idle != NULL ? startTimer(30) : 0)
{
    setAutoFillBackground(true);

    QPalette pal;
    pal.setBrush(backgroundRole(), instrument->kIsPatchbay ? PLUGIN_NAME::getIconPixmap("artwork-patchbay") : PLUGIN_NAME::getIconPixmap("artwork-rack"));
    setPalette(pal);

    QVBoxLayout * l = new QVBoxLayout( this );
    l->setContentsMargins( 20, 180, 10, 10 );
    l->setSpacing( 10 );

    m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
    m_toggleUIButton->setCheckable( true );
    m_toggleUIButton->setChecked( false );
    m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
    m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
    connect( m_toggleUIButton, SIGNAL( clicked(bool) ), this, SLOT( toggleUI( bool ) ) );

    m_toggleUIButton->setWhatsThis(
                tr( "Click here to show or hide the graphical user interface (GUI) of Carla." ) );

    l->addWidget( m_toggleUIButton );
    l->addStretch();

    connect(instrument, SIGNAL(uiClosed()), this, SLOT(uiClosed()));
}

CarlaInstrumentView::~CarlaInstrumentView()
{
    if (m_toggleUIButton->isChecked())
        toggleUI(false);
}

void CarlaInstrumentView::toggleUI(bool visible)
{
    if (fHandle != NULL && fDescriptor->ui_show != NULL)
        fDescriptor->ui_show(fHandle, visible);
}

void CarlaInstrumentView::uiClosed()
{
    m_toggleUIButton->setChecked(false);
}

void CarlaInstrumentView::modelChanged()
{
}

void CarlaInstrumentView::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == fTimerId)
        fDescriptor->ui_idle(fHandle);

    InstrumentView::timerEvent(event);
}

// -------------------------------------------------------------------

#include "moc_carla.cxx"
