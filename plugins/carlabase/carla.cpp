/*
 * carla.cpp - Carla for LMMS
 *
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "carla.h"

#define REAL_BUILD // FIXME this shouldn't be needed
#include "CarlaHost.h"

#include "Engine.h"
#include "Song.h"
#include "gui_templates.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "Knob.h"
#include "EffectControls.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QScrollArea>
#include <QtDebug>
#include <QString>

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

    // carla/resources contains PyQt scripts required for launch
    QString dllName(carla_get_library_filename());
    QString resourcesPath;
#if defined(CARLA_OS_LINUX)
    // parse prefix from dll filename
    QDir path = QFileInfo(dllName).dir();
    path.cdUp();
    path.cdUp();
    resourcesPath = path.absolutePath() + "/share/carla/resources";
#elif defined(CARLA_OS_MAC)
    // assume standard install location
    resourcesPath = "/Applications/Carla.app/Contents/MacOS/resources";
#elif defined(CARLA_OS_WIN32) || defined(CARLA_OS_WIN64)
    // not yet supported
#endif
    fHost.resourceDir            = strdup(resourcesPath.toUtf8().constData());
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
    InstrumentPlayHandle * iph = new InstrumentPlayHandle( this, instrumentTrack );
    Engine::mixer()->addPlayHandle( iph );

    connect(Engine::mixer(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

CarlaInstrument::~CarlaInstrument()
{
    Engine::mixer()->removePlayHandlesOfTypes(instrumentTrack(), PlayHandle::TypeNotePlayHandle | PlayHandle::TypeInstrumentPlayHandle);

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
    return Engine::mixer()->framesPerPeriod();
}

double CarlaInstrument::handleGetSampleRate() const
{
    return Engine::mixer()->processingSampleRate();
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
    case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
        handleUiClosed();
        break;
    case NATIVE_HOST_OPCODE_HOST_IDLE:
        qApp->processEvents();
        break;
    default:
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

	for (uint32_t i=0; i < floatModels.count(); ++i)
	{
		QString idStr = CARLA_SETTING_PREFIX + QString::number(i);
		floatModels[i]->saveSettings( doc, parent, idStr );
	}
	
}

void CarlaInstrument::refreshParams()
{
	if (fDescriptor->get_parameter_count != nullptr &&
		fDescriptor->get_parameter_info  != nullptr &&
		fDescriptor->get_parameter_value != nullptr &&
		fDescriptor->set_parameter_value != nullptr)
	{
		uint32_t param_count = fDescriptor->get_parameter_count(fHandle);
		if (param_count > 0)
        {
			clearKnobModels();
			floatModels.reserve(param_count);
            for (uint32_t i=0; i < param_count; ++i)
            {
				// https://github.com/falkTX/Carla/tree/master/source/native-plugins source/native-plugins/resources/carla-plugin
                const NativeParameter* paramInfo(fDescriptor->get_parameter_info(fHandle, i));
                float param_value = fDescriptor->get_parameter_value(fHandle, i);

				// Get parameter name
                QString name = "_NO_NAME_";
				if (paramInfo->name != nullptr){
					name = paramInfo->name;
				}

				// Create new model for the knob.
				floatModels.push_back(new FloatModel(param_value,0.0f,1.0f,0.001f,this,name));

				// Load settings into model.
				QString idStr = CARLA_SETTING_PREFIX + QString::number(i);
				floatModels[i]->loadSettings( settingsElem, idStr );

				// Connect to signal dataChanged to knobChanged function.
				connect( floatModels[i], &FloatModel::dataChanged, [=]() { knobModelChanged(i); });

				// TODO Signal view to update?
            }
        }
	}
}

void CarlaInstrument::clearKnobModels(){ floatModels.clear(); }

void CarlaInstrument::knobModelChanged(uint32_t index)
{
	float value = floatModels[index]->value();
	if (fDescriptor->set_parameter_value != nullptr){
		fDescriptor->set_parameter_value(fHandle, index, value);
	}
}

void CarlaInstrument::loadSettings(const QDomElement& elem)
{
    if (fHandle == NULL || fDescriptor->set_state == NULL)
        return;

    QDomDocument carlaDoc("carla");
    carlaDoc.appendChild(carlaDoc.importNode(elem.firstChildElement(), true ));

    fDescriptor->set_state(fHandle, carlaDoc.toString(0).toUtf8().constData());

	// Store to load parameter knobs settings when added.
	settingsElem = const_cast<QDomElement&>(elem);
	refreshParams();
}

void CarlaInstrument::play(sampleFrame* workingBuffer)
{
    const uint bufsize = Engine::mixer()->framesPerPeriod();

    std::memset(workingBuffer, 0, sizeof(sample_t)*bufsize*DEFAULT_CHANNELS);

    if (fHandle == NULL)
    {
        instrumentTrack()->processAudioBuffer(workingBuffer, bufsize, NULL);
        return;
    }

    // set time info
    Song * const s = Engine::getSong();
    fTimeInfo.playing  = s->isPlaying();
    fTimeInfo.frame    = s->getPlayPos(s->playMode()).frames(Engine::framesPerTick());
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
      fTimerId(fHandle != NULL && fDescriptor->ui_idle != NULL ? startTimer(30) : 0),
      p_instrument(instrument)
{
	lMaxColumns = 4;
	lCurColumn = 0;
	lCurRow = 0;
    setAutoFillBackground(true);

    QPalette pal;
    pal.setBrush(backgroundRole(), instrument->kIsPatchbay ? PLUGIN_NAME::getIconPixmap("artwork-patchbay") : PLUGIN_NAME::getIconPixmap("artwork-rack"));
    setPalette(pal);

    QVBoxLayout * l = new QVBoxLayout( this );
    l->setContentsMargins( 3, 40, 3, 3 );
    l->setSpacing( 3 );
    l->setAlignment( Qt::AlignTop );

	// Horizontal layout for the buttons
    QHBoxLayout * hl = new QHBoxLayout(this);

	// Show GUI button
    m_toggleUIButton = new QPushButton( tr( "Show GUI" ), this );
    m_toggleUIButton->setCheckable( true );
    m_toggleUIButton->setChecked( false );
    m_toggleUIButton->setIcon( embed::getIconPixmap( "zoom" ) );
    m_toggleUIButton->setFont( pointSize<8>( m_toggleUIButton->font() ) );
    connect( m_toggleUIButton, SIGNAL( clicked(bool) ), this, SLOT( toggleUI( bool ) ) );

    m_toggleUIButton->setWhatsThis(
                tr( "Click here to show or hide the graphical user interface (GUI) of Carla." ) );

	// Refresh params button
	m_refreshParamsButton = new QPushButton( tr( "" ), this );
	m_refreshParamsButton->setIcon( embed::getIconPixmap( "reload" ) );
	m_refreshParamsButton->setWhatsThis(
                tr( "Click here to reload the Carla parameter knobs." ) );
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_refreshParamsButton->sizePolicy().hasHeightForWidth());
	m_refreshParamsButton->setSizePolicy(sizePolicy);
	m_refreshParamsButton->setCheckable( false );

	// ScrollArea that will contain the knobs
	m_scrollArea = new QScrollArea(this);
	QWidget * scrollAreaWidget = new QWidget( this );
	m_scrollAreaLayout = new QGridLayout( scrollAreaWidget );
	scrollAreaWidget->setLayout( m_scrollAreaLayout );
	m_scrollArea->setWidget( scrollAreaWidget );
	m_scrollArea->setWidgetResizable( true );

	// Add widgets to layout
	hl->addWidget( m_toggleUIButton );
    hl->addWidget( m_refreshParamsButton );
	l->addLayout( hl );
    l->addWidget( m_scrollArea );

	connect( m_refreshParamsButton, SIGNAL( clicked(bool) ), this, SLOT( onRefreshButton() ) );
    connect(instrument, SIGNAL(uiClosed()), this, SLOT(uiClosed()));

    refreshButtons();
}

CarlaInstrumentView::~CarlaInstrumentView()
{
    if (m_toggleUIButton->isChecked())
        toggleUI(false);
}

void CarlaInstrumentView::addKnob(FloatModel& knobModel){
	Knob* new_knob = new Knob(this);

	QString name = knobModel.displayName();
	new_knob->setHintText(name,"");
	new_knob->setLabel(name);

	// Set the newly created model to the knob.
	new_knob->setModel(&knobModel);

	// Add the new knob to layout
	m_scrollAreaLayout->addWidget(new_knob, lCurRow, lCurColumn);

	if (lCurColumn < lMaxColumns-1){
		lCurColumn++;
	} else {
		lCurColumn=0;
		lCurRow++;
	}
}
void CarlaInstrumentView::clearKnobs(){
	QLayoutItem *item;
	while((item = m_scrollAreaLayout->takeAt(0))) {
		if (item->widget()) {
            delete item->widget();
        }
        delete item;
	}
	lCurColumn = 0;
	lCurRow = 0;
}

void CarlaInstrumentView::onRefreshButton(){
	p_instrument->refreshParams();
	refreshButtons();
}

void CarlaInstrumentView::refreshButtons()
{
	CarlaInstrument * instrument = castModel<CarlaInstrument>();
	if (instrument->floatModels.count()==0){return;}
	if (instrument->floatModels.count() != m_scrollAreaLayout->count()){
		clearKnobs();

		for (uint32_t i=0; i < instrument->floatModels.count(); ++i)
		{
			addKnob(*instrument->floatModels[i]);
		}
	}
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
	refreshButtons();
}

void CarlaInstrumentView::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == fTimerId)
        fDescriptor->ui_idle(fHandle);

    InstrumentView::timerEvent(event);
}
