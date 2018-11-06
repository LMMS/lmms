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

#ifndef SpaInstrument_H
#define SpaInstrument_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

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
	SpaInstrument(InstrumentTrack *instrumentTrackArg,
		const char *m_libraryName, const Descriptor *pluginDescriptor);
	virtual ~SpaInstrument();

#ifdef SPA_INSTRUMENT_USE_MIDI
	virtual bool handleMidiEvent(const MidiEvent &event,
		const MidiTime &time = MidiTime(), f_cnt_t offset = 0);
#else
	virtual void playNote(NotePlayHandle *nph, sampleFrame *);
#endif
	virtual void play(sampleFrame *buf);

	virtual void saveSettings(QDomDocument &doc, QDomElement &that);
	virtual void loadSettings(const QDomElement &that);

	virtual void loadFile(const QString &file);

	virtual QString nodeName() const;

	virtual Flags flags() const
	{
#ifdef SPA_INSTRUMENT_USE_MIDI
		return IsSingleStreamed | IsMidiBased;
#else
		return IsSingleStreamed;
#endif
	}

	virtual PluginView *instantiateView(QWidget *parent);

	void setLibraryName(const QString &name) { m_libraryName = name; }

	void writeOsc(const char *dest, const char *args, va_list va);
	void writeOsc(const char *dest, const char *args, ...);

	const spa::descriptor *m_descriptor = nullptr;
	spa::plugin *m_plugin = nullptr;

	QMap<QString, AutomatableModel *> m_connectedModels;
	uint64_t m_loadTicket = 0, m_saveTicket = 0, m_restoreTicket = 0;

private slots:
	void reloadPlugin();
	void updatePitchRange();

private:
	struct lmmsPorts
	{
		unsigned samplecount;
		unsigned buffersize;
		long samplerate; // TODO: use const?
		std::vector<float> m_lUnprocessed, m_rUnprocessed, m_lProcessed,
			m_rProcessed;
		std::vector<float> m_unknownControls;
		lmmsPorts(int bufferSize);
		std::unique_ptr<spa::audio::osc_ringbuffer> rb;
	} m_ports;

	friend struct lmmsVisitor;

#ifdef SPA_INSTRUMENT_USE_MIDI
	QMutex m_pluginMutex;
	int m_runningNotes[NumKeys];
#endif

	bool initPlugin();
	void shutdownPlugin();

	bool m_hasGUI;
	//	QMutex m_pluginMutex;

	friend class SpaView;

	bool m_loaded;
#ifdef SPA_INSTRUMENT_USE_QLIBRARY
	class QLibrary *m_lib = nullptr;
#else
	void *m_lib = nullptr; //!< dlopen() handle
#endif
	QString m_libraryName;

	//! load a file in the plugin, but don't do anything in LMMS
	void loadFileInternal(const QString &file);
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

#endif // LMMS_HAVE_SPA

#endif // SpaInstrument_H
