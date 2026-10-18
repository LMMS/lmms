/*
 * SfzTrigger.cpp - Custom class for representing MIDI events relevant to the SFZ player
 *
 * Copyright (c) 2026 Keratin
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

#include "SfzTrigger.h"

namespace lmms
{

const SfzTrigger SfzTrigger::noteOnEvent(const int frameOffset, const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOn;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	trigger.m_frameOffset = frameOffset;
	return trigger;
}

const SfzTrigger SfzTrigger::noteOffEvent(const int frameOffset, const int key, const int velocity)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::NoteOff;
	trigger.m_key = key;
	trigger.m_velocity = velocity;
	trigger.m_frameOffset = frameOffset;
	return trigger;
}

const SfzTrigger SfzTrigger::controlChangeEvent(const int frameOffset, const int controlNumber, const int value)
{
	SfzTrigger trigger = SfzTrigger();
	trigger.m_type = Type::ControlChange;
	trigger.m_controlChangeNumber = controlNumber;
	trigger.m_controlChangeValue = value;
	trigger.m_frameOffset = frameOffset; // Frame offset is not currently for CC events, since making them sample-exact would be inconvenient
	return trigger;
}

} // namespace lmms
