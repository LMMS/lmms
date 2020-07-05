/*
 * carla.h - Carla for LMMS
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

#ifndef CARLA_H
#define CARLA_H

#define CARLA_SETTING_PREFIX    "PARAM_KNOB_"
#define CARLA_MIN_PARAM_VERSION 0x020090
#define CARLA_VERSION_HEX_3 0x30000

// qt
#include <QCloseEvent>
#include <QCompleter>
#include <QGridLayout>
#include <QLineEdit>
#include <QScrollArea>
#include <QStringListModel>
#include <QtCore/QMutex>

// carla/source/includes
#include "carlabase_export.h"
#if CARLA_VERSION_HEX >= 0x010911
    #include "CarlaNativePlugin.h"
#else
    #include "CarlaBackend.h"
    #include "CarlaNative.h"
    #include "CarlaUtils.h"
    CARLA_EXPORT
    const NativePluginDescriptor* carla_get_native_patchbay_plugin();

    CARLA_EXPORT
    const NativePluginDescriptor* carla_get_native_rack_plugin();
#endif

// lmms/include/
#include "EffectControls.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "SubWindow.h"

class QPushButton;
class QComboBox;

class CarlaParamFloatModel : public FloatModel
{
public:
	CarlaParamFloatModel(Model * parent):
		FloatModel(0.0, 0.0, 1.0, 0.001, parent, "Unused"),
		m_isOutput(false),
		m_isEnabled(false)
	{
	}

	// From AutomatableModel.h, it's private there.
	inline static bool mustQuoteName(const QString &name)
	{
		QRegExp reg("^[A-Za-z0-9._-]+$");
		return !reg.exactMatch(name);
	}

	inline virtual void loadSettings(const QDomElement& element, const QString& name = QString("value")) override
	{
		AutomatableModel::loadSettings(element, name);
		bool mustQuote = mustQuoteName(name);
		QDomElement me = element.firstChildElement(mustQuote ? QString("automatablemodel") : name);
		if (!me.isNull()) {
			m_isOutput = (bool)me.attribute("output", "0").toInt();
			m_groupName = QString(me.attribute("groupName", ""));
		}
	}

	inline virtual void saveSettings(QDomDocument& doc, QDomElement& element, const QString& name = QString( "value" )) override
	{
		if (m_isEnabled)
		{
			AutomatableModel::saveSettings(doc, element, name);
			bool mustQuote = mustQuoteName(name);
			QDomElement me = element.firstChildElement(mustQuote ? QString("automatablemodel") : name);
			if (!me.isNull())
			{
				me.setAttribute("output", m_isOutput);
				me.setAttribute("groupName", m_groupName);
			}
		}
	}

	inline const bool enabled()
	{
		return m_isEnabled;
	}

	inline const bool isOutput()
	{
		return m_isOutput;
	}

	inline void setOutput(bool state = true)
	{
		m_isOutput = state;
	}

	inline void setEnabled(bool state = true)
	{
		m_isEnabled = state;
	}

	inline void setGroupName(QString groupName)
	{
		m_groupName = groupName;
	}

	virtual QString groupName() const
	{
		return m_groupName;
	}

private:
	bool m_isOutput;
	bool m_isEnabled;
	QString m_groupName;
};

// -------------------------------------------------------------------

class CarlaParamsSubWindow : public SubWindow
{
	Q_OBJECT

signals:
	void uiClosed();

public:
	CarlaParamsSubWindow(QWidget * _parent, Qt::WindowFlags windowFlags) :
		SubWindow(_parent)
	{
		setAttribute(Qt::WA_DeleteOnClose, false);
		setWindowFlags(windowFlags);
	}

	virtual void closeEvent(QCloseEvent * event) override
	{
		emit uiClosed();
		event->accept();
	}
};

// -------------------------------------------------------------------

class CARLABASE_EXPORT CarlaInstrument : public Instrument
{
    Q_OBJECT

public:
    static const uint32_t kMaxMidiEvents = 512;

    CarlaInstrument(InstrumentTrack* const instrumentTrack, const Descriptor* const descriptor, const bool isPatchbay);
    virtual ~CarlaInstrument();

    // Carla NativeHostDescriptor functions
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
    virtual bool handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset);
    virtual PluginView* instantiateView(QWidget* parent);

signals:
    void uiClosed();
    void paramsUpdated();

private slots:
    void sampleRateChanged();
    void refreshParams(bool init = false);
    void clearKnobModels();
    void knobModelChanged(uint32_t index);
    void updateKnobModel(uint32_t index);

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

    QList<CarlaParamFloatModel*> m_paramModels;
    QDomElement m_settingsElem;
    QMdiSubWindow* m_subWindow;
    QObject* p_subWindow;

    QCompleter* m_paramsCompleter;
    QStringListModel* m_completerModel;

    friend class CarlaInstrumentView;
    friend class CarlaParamsView;
};


// -------------------------------------------------------------------

class CarlaInstrumentView : public InstrumentViewFixedSize
{
    Q_OBJECT

public:
    CarlaInstrumentView(CarlaInstrument* const instrument, QWidget* const parent);
    virtual ~CarlaInstrumentView();

private slots:
    void toggleUI(bool);
    void uiClosed();
    void toggleParamsWindow();
    void paramsUiClosed();

private:
    virtual void modelChanged();
    virtual void timerEvent(QTimerEvent*);

    NativePluginHandle fHandle;
    const NativePluginDescriptor* fDescriptor;
    int fTimerId;

    CarlaInstrument* const m_carlaInstrument;
    QWidget* const p_parent;

    QPushButton* m_toggleUIButton;
    QPushButton* m_toggleParamsWindowButton;
};

// -------------------------------------------------------------------

class CarlaParamsView : public InstrumentView
{
	Q_OBJECT
public:
	CarlaParamsView(CarlaInstrument* const instrument, QWidget* const parent);
	virtual ~CarlaParamsView();

signals:
	void uiClosed();

private slots:
	void refreshKnobs();
	void filterKnobs();
	void clearFilterText();

private:
	virtual void modelChanged();

	void adjustWindowWidth();
	void addKnob(uint32_t index);
	void clearKnobs();

	CarlaInstrument* const m_carlaInstrument;
	QList<Knob*> m_knobs;

	const uint32_t m_maxColumns;
	uint32_t m_curColumn;
	uint32_t m_curRow;

	uint32_t m_curOutColumn;
	uint32_t m_curOutRow;

	QScrollArea* m_inputScrollArea;
	QGridLayout* m_inputScrollAreaLayout;
	QWidget* m_inputScrollAreaWidgetContent;
	QScrollArea* m_outputScrollArea;
	QGridLayout* m_outputScrollAreaLayout;
	QWidget* m_outputScrollAreaWidgetContent;
	QHBoxLayout* m_toolBarLayout;

	QLineEdit* m_paramsFilterLineEdit;
	QPushButton* m_clearFilterButton;
	QPushButton* m_automatedOnlyButton;

	QComboBox* m_groupFilterCombo;
	QStringListModel* m_groupFilterModel;
};

#endif
