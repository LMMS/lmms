/*
 * ClapLog.h - Implements CLAP log extension
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

#ifndef LMMS_CLAP_LOG_H
#define LMMS_CLAP_LOG_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <clap/ext/log.h>

#include "ClapExtension.h"

namespace lmms
{

class ClapLog : public ClapExtension<clap_host_log>
{
public:
	using ClapExtension::ClapExtension;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_LOG; }
	auto hostExt() const -> const clap_host_log* override;

	void log(clap_log_severity severity, std::string_view msg) const;
	void log(clap_log_severity severity, const char* msg) const;

	//! Log without plugin information
	static void globalLog(clap_log_severity severity, std::string_view msg);

	//! Log without any additional information
	static void plainLog(clap_log_severity severity, std::string_view msg);

	static void plainLog(std::string_view msg);

private:
	/**
	 * clap_host_log implementation
	 */
	static void clapLog(const clap_host_t* host, clap_log_severity severity, const char* msg);
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_LOG_H
