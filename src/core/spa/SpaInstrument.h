/*
 * spainstrument.h - implementation of SPA interface
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

#ifndef SpaInstrument_H
#define SpaInstrument_H

#include <QMap>
#include <QString>
#include <memory>

// whether to use MIDI vs playHandle
#define SPA_INSTRUMENT_USE_MIDI
// whether to use QLibrary vs dlopen()
#define SPA_INSTRUMENT_USE_QLIBRARY

#ifdef SPA_INSTRUMENT_USE_MIDI
#include <QMutex>
#endif

// general LMMS includes
#include "DataFile.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"

// LMMS includes for spa
#include "SpaOscModel.h"

// includes from the spa library
#include <spa/audio_fwd.h>
#include <spa/spa_fwd.h>

class QPushButton;

class SpaInstrument : public Instrument
{
	Q_OBJECT
public:
	SpaInstrument(InstrumentTrack *_instrument_track,
		const char *libraryName, const Descriptor *plugin_descriptor);
	virtual ~SpaInstrument();

#ifdef SPA_INSTRUMENT_USE_MIDI
	virtual bool handleMidiEvent(const MidiEvent &event,
		const MidiTime &time = MidiTime(), f_cnt_t offset = 0);
#else
	virtual void playNote(NotePlayHandle *_n, sampleFrame *);
#endif
	virtual void play(sampleFrame *_working_buffer);

	virtual void saveSettings(QDomDocument &_doc, QDomElement &_parent);
	virtual void loadSettings(const QDomElement &_this);

	virtual void loadFile(const QString &_file);

	virtual QString nodeName() const;

	virtual Flags flags() const
	{
#ifdef SPA_INSTRUMENT_USE_MIDI
		return IsSingleStreamed | IsMidiBased;
#else
		return IsSingleStreamed;
#endif
	}

	virtual PluginView *instantiateView(QWidget *_parent);

	void setLibraryName(const QString &name) { libraryName = name; }

	void writeOsc(const char *dest, const char *args, va_list va);
	void writeOsc(const char *dest, const char *args, ...);

	const spa::descriptor *descriptor = nullptr;
	spa::plugin *plugin = nullptr;

	QMap<QString, AutomatableModel *> connectedModels;
	uint64_t load_ticket = 0, save_ticket = 0, restore_ticket = 0;

private slots:
	void reloadPlugin();
	void updatePitchRange();

private:
	struct lmms_ports
	{
		unsigned samplecount;
		unsigned buffersize;
		long samplerate; // TODO: use const?
		std::vector<float> l_unprocessed, r_unprocessed, l_processed,
			r_processed;
		std::vector<float> unknown_controls;
		lmms_ports(int buffersize);
		std::unique_ptr<spa::audio::osc_ringbuffer> rb;
	} ports;

	friend struct lmms_visitor;

#ifdef SPA_INSTRUMENT_USE_MIDI
	QMutex m_pluginMutex;
	int m_runningNotes[NumKeys];
#endif

	bool initPlugin();
	void shutdownPlugin();

	bool m_hasGUI;
	//	QMutex m_pluginMutex;

	friend class SpaView;

	bool loaded;
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
	class QLibrary *lib = nullptr;
#else
	void *lib = nullptr; //!< dlopen() handle
#endif
	QString libraryName;

	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &_file);
signals:
	void settingsChanged();
};

class SpaView : public InstrumentView
{
	Q_OBJECT
public:
	SpaView(Instrument *_instrument, QWidget *_parent);
	virtual ~SpaView();

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

#endif // SpaInstrument_H
