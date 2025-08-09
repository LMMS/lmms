/*
 * PluginIssue.cpp - PluginIssue class implementation
 *
 * Copyright (c) 2019-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include <QDebug>

#include "PluginIssue.h"

namespace lmms
{


const char *PluginIssue::msgFor(const PluginIssueType &it)
{
	switch (it)
	{
		case PluginIssueType::UnknownPortFlow:
			return "unknown port flow for mandatory port";
		case PluginIssueType::UnknownPortType:
			return "unknown port type for mandatory port";
		case PluginIssueType::TooManyInputChannels:
			return "too many audio input channels";
		case PluginIssueType::TooManyOutputChannels:
			return "too many audio output channels";
		case PluginIssueType::TooManyMidiInputChannels:
			return "too many MIDI input channels";
		case PluginIssueType::TooManyMidiOutputChannels:
			return "too many MIDI output channels";
		case PluginIssueType::NoOutputChannel:
			return "no audio output channel";
		case PluginIssueType::PortHasNoDef:
			return "port is missing default value";
		case PluginIssueType::PortHasNoMin:
			return "port is missing min value";
		case PluginIssueType::PortHasNoMax:
			return "port is missing max value";
		case PluginIssueType::MinGreaterMax:
			return "port minimum is greater than maximum";
		case PluginIssueType::DefaultValueNotInRange:
			return "default value is not in range [min, max]";
		case PluginIssueType::LogScaleMinMissing:
			return "logscale requires minimum value";
		case PluginIssueType::LogScaleMaxMissing:
			return "logscale requires maximum value";
		case PluginIssueType::LogScaleMinMaxDifferentSigns:
			return "logscale with min < 0 < max";
		case PluginIssueType::FeatureNotSupported:
			return "required feature not supported";
		case PluginIssueType::BadPortType:
			return "unsupported port type";
		case PluginIssueType::Blocked:
			return "blocked plugin";
		case PluginIssueType::NoIssue:
			return nullptr;
	}
	return nullptr;
}




bool PluginIssue::operator==(const PluginIssue &other) const
{
	return (m_issueType == other.m_issueType) && (m_info == other.m_info);
}




bool PluginIssue::operator<(const PluginIssue &other) const
{
	return (m_issueType != other.m_issueType)
			? m_issueType < other.m_issueType
			: m_info < other.m_info;
}


QDebug operator<<(QDebug stream, const PluginIssue &iss)
{
	stream << PluginIssue::msgFor(iss.m_issueType);
	if (iss.m_info.length())
	{
		stream.nospace() << ": " << iss.m_info.c_str();
	}
	return stream;
}

} // namespace lmms



