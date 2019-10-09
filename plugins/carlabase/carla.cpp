/*
 * carla.cpp - Carla for LMMS
 *
 * Copyright (C) 2014-2018 Filipe Coelho <falktx@falktx.com>
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

#include "Engine.h"
#include "GuiApplication.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Song.h"
#include "gui_templates.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMdiArea>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>
#include <QTimerEvent>
#include <QVBoxLayout>

#include <cstring>

#include "embed.h"

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

CarlaInstrument::CarlaInstrument(InstrumentTrack* const instrumentTrack, const Descriptor* const descriptor, const bool isPatchbay)
    : Instrument(instrumentTrack, descriptor),
      kIsPatchbay(isPatchbay),
      fHandle(NULL),
      fDescriptor(isPatchbay ? carla_get_native_patchbay_plugin() : carla_get_native_rack_plugin()),
      fMidiEventCount(0),
      m_subWindow(NULL)
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
    // parse prefix from dll filename
    QDir path = QFileInfo(dllName).dir();
    resourcesPath = path.absolutePath() + "/resources";
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

    // text filter completion
    m_completerModel = new QStringListModel(this);
    m_paramsCompleter = new QCompleter(m_completerModel, this);
    m_paramsCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_paramsCompleter->setCompletionMode(QCompleter::PopupCompletion);

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

    if (p_subWindow != NULL) {
        delete p_subWindow;
        p_subWindow = NULL;
    }

    if (m_paramModels.isEmpty() == false) {
        m_paramModels.clear();
    }
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

intptr_t CarlaInstrument::handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t, const intptr_t, void* const, const float)
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

    for (uint32_t i=0; i < m_paramModels.count(); ++i)
    {
        QString idStr = CARLA_SETTING_PREFIX + QString::number(i);
        m_paramModels[i]->saveSettings(doc, parent, idStr);
    }
}

void CarlaInstrument::refreshParams(bool valuesOnly = false, bool init = false)
{
	if (fDescriptor->get_parameter_count != nullptr &&
		fDescriptor->get_parameter_info  != nullptr &&
		fDescriptor->get_parameter_value != nullptr &&
		fDescriptor->set_parameter_value != nullptr)
	{
		uint32_t param_count = fDescriptor->get_parameter_count(fHandle);

		if (!param_count) {
			clearKnobModels();
			return;
		}

		if (!valuesOnly)
		{
			clearKnobModels();
			m_paramModels.reserve(param_count);
		}
		else if (m_paramModels.empty())
		{
			return;
		}

		QList<QString> completerData;

		for (uint32_t i=0; i < param_count; ++i)
		{
			// https://github.com/falkTX/Carla/tree/master/source/native-plugins source/native-plugins/resources/carla-plugin
			float param_value = fDescriptor->get_parameter_value(fHandle, i);

			if (valuesOnly)
			{
				m_paramModels[i]->setValue(param_value);
				continue;
			}

			const NativeParameter* paramInfo(fDescriptor->get_parameter_info(fHandle, i));

			// Get parameter name
			QString name = "_NO_NAME_";
			if (paramInfo->name != nullptr){
				name = paramInfo->name;
			}

			completerData.push_back(name);

			// current_value, min, max, steps
			m_paramModels.push_back(new FloatModel(param_value, paramInfo->ranges.min,
					paramInfo->ranges.max, paramInfo->ranges.step, this, name));

			// Load settings into model.
			if (init)
			{
				QString idStr = CARLA_SETTING_PREFIX + QString::number(i);
				m_paramModels[i]->loadSettings(m_settingsElem, idStr);
			}

			connect(m_paramModels[i], &FloatModel::dataChanged, this, [=]() {knobModelChanged(i);}, Qt::DirectConnection);
		}
		// Set completer data
		m_completerModel->setStringList(completerData);
	}
}

void CarlaInstrument::clearKnobModels(){
	//Delete the models, this also disconnects all connections (automation and controller connections)
	for (uint32_t i=0; i < m_paramModels.count(); ++i)
	{
		delete m_paramModels[i];
	}

	//Clear the list
	m_paramModels.clear();
}

void CarlaInstrument::knobModelChanged(uint32_t index)
{
	if (fDescriptor->set_parameter_value != nullptr){
		fDescriptor->set_parameter_value(fHandle, index, m_paramModels[index]->value());
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
    m_settingsElem = const_cast<QDomElement&>(elem);
    refreshParams(false, true);
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
// Disable plugin focus per https://bugreports.qt.io/browse/QTBUG-30181
#ifndef CARLA_OS_MAC
    if (QWidget* const window = parent->window())
        fHost.uiParentId = window->winId();
    else
#endif
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
      m_carlaInstrument(instrument),
      p_parent(parent)
{
    setAutoFillBackground(true);

    QPalette pal;
    pal.setBrush(backgroundRole(), instrument->kIsPatchbay ? PLUGIN_NAME::getIconPixmap("artwork-patchbay") : PLUGIN_NAME::getIconPixmap("artwork-rack"));
    setPalette(pal);

    QHBoxLayout * l = new QHBoxLayout(this);
    l->setContentsMargins(20, 180, 10, 10);
    l->setSpacing(3);
    l->setAlignment(Qt::AlignTop);

    // Show GUI button
    m_toggleUIButton = new QPushButton(tr("Show GUI"), this);
    m_toggleUIButton->setCheckable(true);
    m_toggleUIButton->setChecked(false);
    m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
    m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));

    m_toggleUIButton->setToolTip(
            tr("Click here to show or hide the graphical user interface (GUI) of Carla."));

    // Open params sub window button
    m_toggleParamsWindowButton = new QPushButton(tr("Params"), this);
    m_toggleParamsWindowButton->setIcon(embed::getIconPixmap("controller"));
    m_toggleParamsWindowButton->setCheckable(true);
    m_toggleParamsWindowButton->setFont(pointSize<8>(m_toggleParamsWindowButton->font()));

    // Add widgets to layout
    l->addWidget(m_toggleUIButton);
    l->addWidget(m_toggleParamsWindowButton);

    // Connect signals
    connect(m_toggleUIButton, SIGNAL(clicked(bool)), this, SLOT(toggleUI(bool)));
    connect(m_toggleParamsWindowButton, SIGNAL(clicked(bool)), this, SLOT(toggleParamsWindow()));
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

void CarlaInstrumentView::toggleParamsWindow()
{
	if (m_carlaInstrument->m_subWindow == NULL)
	{
		m_carlaInstrument->p_subWindow = new CarlaParamsView(m_carlaInstrument, p_parent);
		connect(m_carlaInstrument->m_subWindow, SIGNAL(uiClosed()), this, SLOT(paramsUiClosed()));
	} else {
		if (m_carlaInstrument->m_subWindow->isVisible()) {
			m_carlaInstrument->m_subWindow->hide();
		} else {
			m_carlaInstrument->m_subWindow->show();
		}
	}
}

void CarlaInstrumentView::paramsUiClosed()
{
	m_toggleParamsWindowButton->setChecked(false);
}

// -------------------------------------------------------------------

CarlaParamsView::CarlaParamsView(CarlaInstrument* const instrument, QWidget* const parent)
	: InstrumentView(instrument, parent),
	m_carlaInstrument(instrument),
	m_maxColumns(6),
	m_curColumn(0),
	m_curRow(0)
{
	// Create central widget
	/*  ___ centralWidget _______________	QWidget
	 * |  __ verticalLayout _____________	QVBoxLayout
	 * | |  __ m_toolBarLayout __________	QHBoxLayout
	 * | | |
	 * | | | option_0 | option_1 ..
	 * | | |_____________________________
	 * | |
	 * | |  __ m_scrollArea _____________	QScrollArea
	 * | | |  __ m_scrollAreaWidgetContent	QWidget
	 * | | | |  __ m_scrollAreaLayout ___	QGridLayout
	 * | | | | |
	 * | | | | | knob | knob | knob
	 * | | | | | knob | knob | knob
	 * | | | | | knob | knob | knob
	 * | | | | |_________________________
	 * | | | |___________________________
	 * | | |_____________________________
	 * */
	QWidget* centralWidget = new QWidget(this);
	QVBoxLayout* verticalLayout = new QVBoxLayout(centralWidget);


	// Toolbar
	m_toolBarLayout = new QHBoxLayout();

	// Toolbar widgets
	// Refresh params button
	m_refreshParamsButton = new QPushButton(tr(""), this);
	m_refreshParamsButton->setIcon(embed::getIconPixmap("reload"));
	m_refreshParamsButton->setToolTip(
				tr("Click here to reload the Carla parameter knobs."));
	QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_refreshParamsButton->sizePolicy().hasHeightForWidth());
	m_refreshParamsButton->setSizePolicy(sizePolicy);
	m_refreshParamsButton->setCheckable(false);

	// Refresh param values button
	m_refreshParamValuesButton = new QPushButton(tr("Values"), this);
	m_refreshParamValuesButton->setIcon(embed::getIconPixmap("reload"));
	m_refreshParamValuesButton->setToolTip(
				tr("Click here to update the Carla parameter knob "
					"values as they are in the remote Carla instance."));
	sizePolicy.setHeightForWidth(m_refreshParamValuesButton->sizePolicy().hasHeightForWidth());
	m_refreshParamValuesButton->setSizePolicy(sizePolicy);

	// Params filter line edit
	m_paramsFilterLineEdit = new QLineEdit(this);
	m_paramsFilterLineEdit->setPlaceholderText(tr("Search.."));
	m_paramsFilterLineEdit->setCompleter(m_carlaInstrument->m_paramsCompleter);

	// Clear filter line edit button
	m_clearFilterButton = new QPushButton(tr(""), this);
	m_clearFilterButton->setIcon(embed::getIconPixmap("edit_erase"));
	m_clearFilterButton->setToolTip(tr("Clear filter text"));
	sizePolicy.setHeightForWidth(m_clearFilterButton->sizePolicy().hasHeightForWidth());
	m_clearFilterButton->setSizePolicy(sizePolicy);

	// Show automated only button
	m_automatedOnlyButton = new QPushButton(tr(""), this);
	m_automatedOnlyButton->setIcon(embed::getIconPixmap("automation"));
	m_automatedOnlyButton->setToolTip(
				tr("Only show knobs with a connection."));
	m_automatedOnlyButton->setCheckable(true);
	sizePolicy.setHeightForWidth(m_automatedOnlyButton->sizePolicy().hasHeightForWidth());
	m_automatedOnlyButton->setSizePolicy(sizePolicy);

	// Add stuff to toolbar
	m_toolBarLayout->addWidget(m_refreshParamsButton);
	m_toolBarLayout->addWidget(m_refreshParamValuesButton);
	m_toolBarLayout->addWidget(m_paramsFilterLineEdit);
	m_toolBarLayout->addWidget(m_clearFilterButton);
	m_toolBarLayout->addWidget(m_automatedOnlyButton);


	// Create scroll area for the knobs
	m_scrollArea = new QScrollArea(this);
	m_scrollAreaWidgetContent = new QWidget();
	m_scrollAreaLayout = new QGridLayout(m_scrollAreaWidgetContent);

	m_scrollAreaWidgetContent->setLayout(m_scrollAreaLayout);

	m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_scrollArea->setWidget(m_scrollAreaWidgetContent);
	m_scrollArea->setWidgetResizable(true);

	m_scrollAreaLayout->setContentsMargins(3, 3, 3, 3);
	m_scrollAreaLayout->setVerticalSpacing(12);
	m_scrollAreaLayout->setHorizontalSpacing(6);
	m_scrollAreaLayout->setColumnStretch(m_maxColumns, 1);


	// Add m_toolBarLayout and m_scrollArea to the verticalLayout.
	verticalLayout->addLayout(m_toolBarLayout);
	verticalLayout->addWidget(m_scrollArea);


	// Sub window
	CarlaParamsSubWindow* win = new CarlaParamsSubWindow(gui->mainWindow()->workspace()->viewport(), Qt::SubWindow |
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	m_carlaInstrument->m_subWindow = gui->mainWindow()->workspace()->addSubWindow(win);
	m_carlaInstrument->m_subWindow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
	m_carlaInstrument->m_subWindow->setFixedWidth(800);
	m_carlaInstrument->m_subWindow->setMinimumHeight(200);
	m_carlaInstrument->m_subWindow->setWidget(centralWidget);
	centralWidget->setWindowTitle(m_carlaInstrument->instrumentTrack()->name() + tr(" - Parameters"));

	// Connect signals
	connect(m_refreshParamsButton, SIGNAL(clicked(bool)), this, SLOT(onRefreshButton()));
	connect(m_refreshParamValuesButton, SIGNAL(clicked(bool)), this, SLOT(onRefreshValuesButton()));
	connect(m_paramsFilterLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(filterKnobs()));
	connect(m_clearFilterButton, SIGNAL(clicked(bool)), this, SLOT(clearFilterText()));
	connect(m_automatedOnlyButton, SIGNAL(toggled(bool)), this, SLOT(filterKnobs()));


	refreshKnobs(); // Add buttons if there are any already.
	m_carlaInstrument->m_subWindow->show(); // Show the subwindow
}

CarlaParamsView::~CarlaParamsView()
{
	// Close and delete m_subWindow
	if (m_carlaInstrument->m_subWindow != NULL )
	{
		m_carlaInstrument->m_subWindow->setAttribute(Qt::WA_DeleteOnClose);
		m_carlaInstrument->m_subWindow->close();

		if (m_carlaInstrument->m_subWindow != NULL)
			delete m_carlaInstrument->m_subWindow;
		m_carlaInstrument->m_subWindow = NULL;
	}

	m_carlaInstrument->p_subWindow = NULL;

	// Clear models
	if (m_carlaInstrument->m_paramModels.isEmpty() == false)
	{
		m_carlaInstrument->clearKnobModels();
	}
}

void CarlaParamsView::clearFilterText()
{
	m_paramsFilterLineEdit->setText("");
}

void CarlaParamsView::filterKnobs()
{
	QString text = m_paramsFilterLineEdit->text();
	clearKnobs(); // Remove all knobs from the layout.

	for (uint32_t i=0; i < m_knobs.count(); ++i)
	{
		// Filter on automation only
		if (m_automatedOnlyButton->isChecked())
		{
			if (! m_carlaInstrument->m_paramModels[i]->isAutomatedOrControlled())
			{
				continue;
			}
		}
		
		// Filter on text
		if (text != "")
		{
			if (m_knobs[i]->objectName().contains(text, Qt::CaseInsensitive))
			{	
				addKnob(i);
			}
		} else {
			addKnob(i);
		}
	}
}

void CarlaParamsView::onRefreshButton()
{
	if (m_carlaInstrument->m_paramModels.isEmpty() == false)
	{
		if (QMessageBox::warning(NULL,
				tr("Reload knobs"),
				tr("There are already knobs loaded, if any of them "
				"are connected to a controller or automation track "
				"their connection will be lost. Do you want to "
				"continue?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
				{
					return;
				}
	}
	m_carlaInstrument->refreshParams();
	refreshKnobs();
}

void CarlaParamsView::onRefreshValuesButton()
{
	m_carlaInstrument->refreshParams(true);
}

void CarlaParamsView::refreshKnobs()
{
	// Make sure all the knobs are deleted.
	for (uint32_t i=0; i < m_knobs.count(); ++i)
	{
		delete m_knobs[i]; // Delete knob widgets itself.
	}
	m_knobs.clear(); // Clear the pointer list.

	// Clear the layout (posible spacer).
	QLayoutItem *item;
	while ((item = m_scrollAreaLayout->takeAt(0)))
	{
		if (item->widget()) {delete item->widget();}
		delete item;
	}

	// Reset position data.
	m_curColumn = 0;
	m_curRow = 0;

	if (!m_carlaInstrument->m_paramModels.count()) { return; }

	// Make room in QList m_knobs
	m_knobs.reserve(m_carlaInstrument->m_paramModels.count());

	for (uint32_t i=0; i < m_carlaInstrument->m_paramModels.count(); ++i)
	{
		m_knobs.push_back(new Knob(m_scrollAreaWidgetContent));
		QString name = (*m_carlaInstrument->m_paramModels[i]).displayName();
		m_knobs[i]->setHintText(name, "");
		m_knobs[i]->setLabel(name);
		m_knobs[i]->setObjectName(name); // this is being used for filtering the knobs.

		// Set the newly created model to the knob.
		m_knobs[i]->setModel(m_carlaInstrument->m_paramModels[i]);

		// Add knob to layout
		addKnob(i);
	}

	// Add spacer so all knobs go to top
	QSpacerItem* verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_scrollAreaLayout->addItem(verticalSpacer, m_curRow + 1, 0, 1, 1);
}

void CarlaParamsView::addKnob(uint32_t index)
{
	// Add the new knob to layout
	m_scrollAreaLayout->addWidget(m_knobs[index], m_curRow, m_curColumn, Qt::AlignHCenter | Qt::AlignTop);

	// Chances that we did close() on the widget is big, so show it.
	m_knobs[index]->show();

	// Keep track of current column and row index.
	if (m_curColumn < m_maxColumns - 1)
	{
		m_curColumn++;
	} else {
		m_curColumn = 0;
		m_curRow++;
	}
}
void CarlaParamsView::clearKnobs()
{
	// Remove knobs from layout.
	for (uint32_t i=0; i < m_knobs.count(); ++i)
	{
		m_knobs[i]->close();
	}

	// Reset position data.
	m_curColumn = 0;
	m_curRow = 0;
}

void CarlaParamsView::modelChanged()
{
	refreshKnobs();
}
