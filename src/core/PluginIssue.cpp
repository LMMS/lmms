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

#include <QDebug>

#include "PluginIssue.h"

const char *PluginIssue::msgFor(const PluginIssueType &it)
{
	switch (it)
	{
		case unknownPortFlow:
			return "unknown port flow for mandatory port";
		case unknownPortType:
			return "unknown port type for mandatory port";
		case tooManyInputChannels:
			return "too many audio input channels";
		case tooManyOutputChannels:
			return "too many audio output channels";
		case noOutputChannel:
			return "no audio output channel";
		case portHasNoDef:
			return "port is missing default value";
		case portHasNoMin:
			return "port is missing min value";
		case portHasNoMax:
			return "port is missing max value";
		case featureNotSupported:
			return "required feature not supported";
		case badPortType:
			return "unsupported port type";
		case noIssue:
			return nullptr;
	}
	return nullptr;
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


