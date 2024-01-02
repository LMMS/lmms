/*
 * ClapLog.cpp - Implements CLAP log extension
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapLog.h"

#ifdef LMMS_HAVE_CLAP

#include <iostream>

#include "ClapManager.h"

namespace lmms
{

namespace
{
	auto logHelper(clap_log_severity severity, std::ostream*& ostr) -> std::string_view
	{
		ostr = &std::cerr;
		std::string_view severityStr;
		switch (severity)
		{
		case CLAP_LOG_DEBUG:
			severityStr = "DEBUG";
			ostr = &std::cout;
			break;
		case CLAP_LOG_INFO:
			severityStr = "INFO";
			ostr = &std::cout;
			break;
		case CLAP_LOG_WARNING:
			severityStr = "WARNING";
			ostr = &std::cout;
			break;
		case CLAP_LOG_ERROR:
			severityStr = "ERROR";
			break;
		case CLAP_LOG_FATAL:
			severityStr = "FATAL";
			break;
		case CLAP_LOG_HOST_MISBEHAVING:
			severityStr = "HOST_MISBEHAVING";
			break;
		case CLAP_LOG_PLUGIN_MISBEHAVING:
			severityStr = "PLUGIN_MISBEHAVING";
			break;
		default:
			severityStr = "UNKNOWN";
			break;
		}
		return severityStr;
	}
} // namespace

auto ClapLog::hostExt() const -> const clap_host_log*
{
	static clap_host_log ext {
		&clapLog
	};
	return &ext;
}

void ClapLog::log(clap_log_severity severity, std::string_view msg) const
{
	if (severity == CLAP_LOG_DEBUG && !ClapManager::debugging()) { return; }

	std::ostream* ostr;
	const auto severityStr = logHelper(severity, ostr);

	*ostr << "[" << severityStr << "] [" << instance()->info().descriptor()->id << "] [CLAP] " << msg << "\n";
}

void ClapLog::log(clap_log_severity severity, const char* msg) const
{
	log(severity, std::string_view{msg ? msg : ""});
}

void ClapLog::globalLog(clap_log_severity severity, std::string_view msg)
{
	if (severity == CLAP_LOG_DEBUG && !ClapManager::debugging()) { return; }

	std::ostream* ostr;
	const auto severityStr = logHelper(severity, ostr);

	*ostr << "[" << severityStr << "] [*] [CLAP] " << msg << "\n";
}

void ClapLog::plainLog(clap_log_severity severity, std::string_view msg)
{
	if (severity == CLAP_LOG_DEBUG && !ClapManager::debugging()) { return; }

	std::ostream* ostr;
	logHelper(severity, ostr);

	*ostr << msg << "\n";
}

void ClapLog::plainLog(std::string_view msg)
{
	std::cout << msg << "\n";
}

void ClapLog::clapLog(const clap_host_t* host, clap_log_severity severity, const char* msg)
{
	// Thread-safe
	// TODO: Specify log message origin in the log message?
	auto h = fromHost(host);
	if (!h) { return; }
	h->logger().log(severity, msg);
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
