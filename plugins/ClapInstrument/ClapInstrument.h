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

#include "ClapInstance.h"
#include "ClapViewBase.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"


namespace lmms
{

namespace gui
{

class ClapInsView;

} // namespace gui

class ClapInstrument : public Instrument
{
	Q_OBJECT

public:
	ClapInstrument(InstrumentTrack* track, Descriptor::SubPluginFeatures::Key* key);
	~ClapInstrument() override;

	//! Must be checked after ctor or reload
	auto isValid() const -> bool;

	void reload();
	void onSampleRateChanged(); //!< TODO: This should be a virtual method in Plugin that can be overridden

	/*
	 * Load/Save
	 */
	void saveSettings(QDomDocument& doc, QDomElement& that) override;
	void loadSettings(const QDomElement& that) override;
	auto nodeName() const -> QString override { return ClapInstance::ClapNodeName.data(); }

	void loadFile(const QString& file) override;

	/*
	 * Realtime funcs
	 */
	auto hasNoteInput() const -> bool override;
	auto handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset) -> bool override;
	void play(SampleFrame* buffer) override;

	auto instantiateView(QWidget* parent) -> gui::PluginView* override;

signals:
	void modelChanged();

private slots:
	void updatePitchRange();

private:
	void clearRunningNotes();

	std::unique_ptr<ClapInstance> m_instance;
	QTimer m_idleTimer;

	std::array<int, NumKeys> m_runningNotes{}; // TODO: Move to Instrument class

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
