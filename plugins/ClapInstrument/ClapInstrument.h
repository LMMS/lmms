/*
 * ClapInstrument.h - Implementation of CLAP instrument
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_INSTRUMENT_H
#define LMMS_CLAP_INSTRUMENT_H

#include <QString>
#include <QTimer>

#include "ClapControlBase.h"
#include "ClapViewBase.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"

// whether to use MIDI vs playHandle
// currently only MIDI works
#define CLAP_INSTRUMENT_USE_MIDI

namespace lmms
{

namespace gui
{

class ClapInsView;

} // namespace gui

class ClapInstrument : public Instrument, public ClapControlBase
{
	Q_OBJECT

signals:
	void modelChanged();

public:
	ClapInstrument(InstrumentTrack* track, Descriptor::SubPluginFeatures::Key* key);
	~ClapInstrument() override;

	void reload();
	void onSampleRateChanged();

	//! Must be checked after ctor or reload
	auto isValid() const -> bool;

	/*
	 * Load/Save
	 */
	void saveSettings(QDomDocument& doc, QDomElement& that) override;
	void loadSettings(const QDomElement& that) override;
	void loadFile(const QString& file) override;

	/*
	 * Realtime funcs
	 */
	auto hasNoteInput() const -> bool override { return ClapControlBase::hasNoteInput(); }

#ifdef CLAP_INSTRUMENT_USE_MIDI
	auto handleMidiEvent(const MidiEvent& event, const TimePos& time = TimePos{}, f_cnt_t offset = 0) -> bool override;
#else
	void playNote(NotePlayHandle* nph, sampleFrame*) override;
#endif

	void play(sampleFrame* buf) override;

	/*
	 * Misc
	 */
	auto flags() const -> Flags override
	{
#ifdef CLAP_INSTRUMENT_USE_MIDI
		return Flag::IsSingleStreamed | Flag::IsMidiBased;
#else
		return Flag::IsSingleStreamed;
#endif
	}

	auto instantiateView(QWidget* parent) -> gui::PluginView* override;

private slots:
	void updatePitchRange();

private:
	auto nodeName() const -> QString override;

	void clearRunningNotes();

#ifdef CLAP_INSTRUMENT_USE_MIDI
	std::array<int, NumKeys> m_runningNotes{};
#endif

	QTimer m_idleTimer;

	friend class gui::ClapInsView;
};


namespace gui
{

class ClapInsView : public InstrumentView, public ClapViewBase
{
Q_OBJECT
public:
	ClapInsView(ClapInstrument* instrument, QWidget* parent);

protected:
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void dropEvent(QDropEvent* de) override;

private:
	void modelChanged() override;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_CLAP_INSTRUMENT_H
