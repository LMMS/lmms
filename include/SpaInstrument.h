/*
 * SpaInstrument.h - implementation of SPA interface
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include <QMap>
#include <QString>
#include <memory>

// general LMMS includes
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"

// LMMS includes for spa
#include "SpaControlBase.h"
#include "SpaPluginBase.h"

// whether to use MIDI vs playHandle
// currently only MIDI works
#define SPA_INSTRUMENT_USE_MIDI

class QPushButton;

class SpaInstrument : public Instrument,
	public SpaControlBase, public SpaPluginBase
{
	Q_OBJECT

	DataFile::Types settingsType() override;
	void setNameFromFile(const QString &name) override;
	SpaPluginBase& getPluginBase() override { return *this; }

public:
	SpaInstrument(InstrumentTrack *instrumentTrackArg,
		const char *m_libraryName, const Descriptor *pluginDescriptor);
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

	void writeOsc(const char *dest, const char *args, va_list va) override;
	void writeOsc(const char *dest, const char *args, ...) override;
	unsigned netPort() const override;
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

class SpaInsView : public InstrumentView
{
	Q_OBJECT
public:
	SpaInsView(Instrument *_instrument, QWidget *_parent);
	virtual ~SpaInsView();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *_dee);
	virtual void dropEvent(QDropEvent *_de);

private:
	void modelChanged();

	QPushButton *m_toggleUIButton;
	QPushButton *m_reloadPluginButton;

private slots:
	void toggleUI();
	void reloadPlugin();
};

#endif // LMMS_HAVE_SPA

#endif // SPA_INSTRUMENT_H
