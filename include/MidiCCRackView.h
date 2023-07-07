/*
 * MidiCCRackView.h - declaration of the MIDI CC rack widget
 *
 * Copyright (c) 2020 Ian Caio <iancaio_dev/at/hotmail.com>
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

#ifndef LMMS_GUI_MIDI_CC_RACK_VIEW_H
#define LMMS_GUI_MIDI_CC_RACK_VIEW_H

#include <QWidget>

#include "Midi.h"
#include "SerializingObject.h"

namespace lmms
{

class InstrumentTrack;

namespace gui
{

class Knob;
class GroupBox;

class MidiCCRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	MidiCCRackView(InstrumentTrack * track);
	~MidiCCRackView() override;

	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement &) override;

	inline QString nodeName() const override
	{
		return "MidiCCRackView";
	}

private slots:
	void renameWindow();

private:
	InstrumentTrack *m_track;

	GroupBox *m_midiCCGroupBox; // MIDI CC GroupBox (used to enable disable MIDI CC)

	Knob *m_controllerKnob[MidiControllerCount]; // Holds the knob widgets for each controller

};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_MIDI_CC_RACK_VIEW_H
