/* `#pragma once` is used in a few places in this repo, but are always
   accompanied by a header guard in lmms core. Is this just convention, or are
   there other reasons header guards are preferred? (e.g. compiler support) */
// #pragma once
#ifndef LMMS_SYNCHRO_H
#define LMMS_SYNCHRO_H

#include <QString>
#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "Graph.h"
#include "Knob.h"

namespace
{

constexpr auto  SYNCHRO_GRAPH_RESOLUTION = 168;
constexpr QColor SYNCHRO_RED = QColor(246, 91, 117);
constexpr QColor SYNCHRO_CYAN = QColor(13, 204, 218);
constexpr QColor SYNCHRO_YELLOW = QColor(255, 187, 0);

}

namespace lmms
{

namespace gui { class SynchroView; }

struct SynchroOsc
{
	FloatModel drive;
	FloatModel sync;
	FloatModel pulse;

	SynchroOsc(Model *parent, QString name) :
		drive(1, 1, 7, 0.01f, parent, name.append(" drive")),
		sync(1, 1, 16, 0.01f, parent, name.append(" sync")),
		pulse(0, 0, 4, 0.01f, parent, name.append(" pulse"))
	{}

	float driveExact(int offset) { return drive.valueBuffer() ? drive.valueBuffer()->value(offset) : drive.value(); }
	float syncExact(int offset) { return sync.valueBuffer() ? sync.valueBuffer()->value(offset) : sync.value(); }
	float pulseExact(int offset) { return pulse.valueBuffer() ? pulse.valueBuffer()->value(offset) : pulse.value(); }
};

class Synchro : public Instrument
{
	Q_OBJECT
public:
	Synchro(InstrumentTrack *track); 
	gui::PluginView *instantiateView(QWidget *parent) override;
	void playNote(NotePlayHandle *nph, SampleFrame *buf) override;
	void deleteNotePluginData(NotePlayHandle *nph) override;
	QString nodeName() const override;
	void saveSettings(QDomDocument &doc, QDomElement &parent) override;
	void loadSettings(const QDomElement &thisElement) override;
protected slots:
	void effectiveSampleRateChanged();
	void carrierChanged();
	void modulatorChanged();
	void eitherOscChanged();
private:
	float modAmtExact(int offset) { return m_modAmt.valueBuffer() ? m_modAmt.valueBuffer()->value(offset) : m_modAmt.value(); }
	float harmonicsExact(int offset) { return m_harmonics.valueBuffer() ? m_harmonics.valueBuffer()->value(offset) : m_harmonics.value(); }
	//TODO Figure out how to allocate both of these contiguously whenever
	// they are resized (in hope of being friendlier to the cache). Probably
	// should benchmarj the downsampling first to see if it's even worth worrying
	// about.
	// Also, should this be using a non-default allocator?
	std::vector<sample_t> m_buf[2];
	SynchroOsc            m_carrier;
	SynchroOsc            m_modulator;
	FloatModel            m_modAmt;
	FloatModel            m_modScale;
	FloatModel            m_harmonics;
	FloatModel            m_octaveRatio; //TODO use IntModel and an appropriate UI element instead of a knob
	int                   m_oversampling; //TODO use dropdown menu or something to select this
	graphModel            m_carrierWaveform;
	graphModel            m_modulatorWaveform;
	graphModel            m_resultingWaveform;

	friend class gui::SynchroView;
};


class gui::SynchroView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	SynchroView(Instrument *instrument, QWidget *parent);
	QSize sizeHint() const override { return QSize(480, 360); }
protected slots:
	void modelChanged() override;
private:
	Graph *m_carrierWaveform;
	Graph *m_modulatorWaveform;
	Graph *m_resultingWaveform;
	Knob  *m_modAmt;
	Knob  *m_modScale;
	Knob  *m_harmonics;
	Knob  *m_octaveRatio;
	Knob  *m_carrierDrive;
	Knob  *m_carrierSync;
	Knob  *m_carrierPulse;
	Knob  *m_modulatorDrive;
	Knob  *m_modulatorSync;
	Knob  *m_modulatorPulse;
};

}

#endif
