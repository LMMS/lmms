/*
 * SpaInstrument.h - implementation of SPA instrument
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef SPA_INSTRUMENT_H
#define SPA_INSTRUMENT_H

#include <QMap>
#include <QString>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"
#include "SpaControlBase.h"
#include "SpaViewBase.h"

// whether to use MIDI vs playHandle
// currently only MIDI works
#define SPA_INSTRUMENT_USE_MIDI

class QPushButton;

class SpaInstrument : public Instrument, public SpaControlBase
{
	Q_OBJECT

	void setNameFromFile(const QString &name) override;

public:
	SpaInstrument(InstrumentTrack *instrumentTrackArg,
		 Descriptor::SubPluginFeatures::Key* key);
	~SpaInstrument() override;

	void saveSettings(QDomDocument &doc, QDomElement &that) override;
	void loadSettings(const QDomElement &that) override;
	void loadFile(const QString &file) override {
		SpaControlBase::loadFile(file); }

#ifdef SPA_INSTRUMENT_USE_MIDI
	bool handleMidiEvent(const MidiEvent &event,
		const MidiTime &time = MidiTime(), f_cnt_t offset = 0) override;
#else
	void playNote(NotePlayHandle *nph, sampleFrame *) override;
#endif
	void play(sampleFrame *buf) override;

	Flags flags() const override
	{
#ifdef SPA_INSTRUMENT_USE_MIDI
		return IsSingleStreamed | IsMidiBased;
#else
		return IsSingleStreamed;
#endif
	}

	PluginView *instantiateView(QWidget *parent) override;

	unsigned netPort(std::size_t) const override;
	class AutomatableModel* modelAtPort(const QString& dest) override;

private slots:
	void updatePitchRange();
	void reloadPlugin() { SpaControlBase::reloadPlugin(); }

private:
#ifdef SPA_INSTRUMENT_USE_MIDI
	int m_runningNotes[NumKeys];
#endif
	friend class SpaInsView;
	QString nodeName() const override;
};

class SpaInsView : public InstrumentView, public SpaViewBase
{
	Q_OBJECT
public:
	SpaInsView(SpaInstrument *_instrument, QWidget *_parent);
	virtual ~SpaInsView();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *_dee);
	virtual void dropEvent(QDropEvent *_de);

private:
	void modelChanged();

private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif // SPA_INSTRUMENT_H
