/*
 * PluginIssue.h - PluginIssue class
 *
 * Copyright (c) 2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "PluginIssue.h"

#include <QDebug>

const char* PluginIssue::msgFor(const PluginIssueType& it) {
	switch (it) {
	case unknownPortFlow: return "unknown port flow for mandatory port";
	case unknownPortType: return "unknown port type for mandatory port";
	case tooManyInputChannels: return "too many audio input channels";
	case tooManyOutputChannels: return "too many audio output channels";
	case tooManyMidiInputChannels: return "too many MIDI input channels";
	case tooManyMidiOutputChannels: return "too many MIDI output channels";
	case noOutputChannel: return "no audio output channel";
	case portHasNoDef: return "port is missing default value";
	case portHasNoMin: return "port is missing min value";
	case portHasNoMax: return "port is missing max value";
	case minGreaterMax: return "port minimum is greater than maximum";
	case defaultValueNotInRange: return "default value is not in range [min, max]";
	case logScaleMinMissing: return "logscale requires minimum value";
	case logScaleMaxMissing: return "logscale requires maximum value";
	case logScaleMinMaxDifferentSigns: return "logscale with min < 0 < max";
	case featureNotSupported: return "required feature not supported";
	case badPortType: return "unsupported port type";
	case blacklisted: return "blacklisted plugin";
	case noIssue: return nullptr;
	}
	return nullptr;
}

bool PluginIssue::operator==(const PluginIssue& other) const {
	return (m_issueType == other.m_issueType) && (m_info == other.m_info);
}

bool PluginIssue::operator<(const PluginIssue& other) const {
	return (m_issueType != other.m_issueType) ? m_issueType < other.m_issueType : m_info < other.m_info;
}

QDebug operator<<(QDebug stream, const PluginIssue& iss) {
	stream << PluginIssue::msgFor(iss.m_issueType);
	if (iss.m_info.length()) { stream.nospace() << ": " << iss.m_info.c_str(); }
	return stream;
}
