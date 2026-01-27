/*
 * SfzTrigger.h - Custom class for representing MIDI events relevant to the SFZ player
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

#ifndef LMMS_SFZ_TRIGGER_H
#define LMMS_SFZ_TRIGGER_H

#include <optional>

namespace lmms
{


class SfzTrigger
{
public:
	//! Helper functions for creating different types of events
	static const SfzTrigger noteOnEvent(const int frameOffset, const int key, const int vel);
	static const SfzTrigger noteOffEvent(const int frameOffset, const int key, const int vel);
	static const SfzTrigger controlChangeEvent(const int frameOffset, const int controlNumber, const int value);

	enum class Type
	{
		NoteOn,
		NoteOff,
		ControlChange,
	};

	const auto& type() const { return m_type; }
	const auto& key() const { return m_key; }
	const auto& velocity() const { return m_velocity; }
	const auto& controlChangeNumber() const { return m_controlChangeNumber; }
	const auto& controlChangeValue() const { return m_controlChangeValue; }
	const int frameOffset() const { return m_frameOffset; }

private:
	Type m_type;
	// TODO should these be optionals or not?
	std::optional<int> m_key = std::nullopt;
	std::optional<int> m_velocity = std::nullopt;
	std::optional<int> m_controlChangeNumber = std::nullopt;
	std::optional<int> m_controlChangeValue = std::nullopt;
	//! The event probably occurred partway through a buffer, so this variable keeps track of when exactly it occurred relative to the start of the buffer
	int m_frameOffset = 0;
};


} // namespace lmms


#endif // LMMS_SFZ_TRIGGER_H