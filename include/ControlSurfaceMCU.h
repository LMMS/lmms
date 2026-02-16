/*
 * ControlSurfaceMCU.h - A controller to receive MIDI MCU control
 *
 * Copyright (c) 2025 - altrouge
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

#ifndef LMMS_CONTROL_SURFACE_MCU_H
#define LMMS_CONTROL_SURFACE_MCU_H

#include "ControlSurface.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"
#include "Note.h"

namespace lmms {
//! Implements the Mackie control protocol to control the DAW.
class LMMS_EXPORT ControlSurfaceMCU : public MidiEventProcessor
{
public:
	ControlSurfaceMCU(const QString& device);

	void processInEvent(const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0) override;
	void processOutEvent(const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0) override;

private:
	MidiPort m_midiPort;
	ControlSurface m_controlSurface;
};
} // namespace lmms

#endif
