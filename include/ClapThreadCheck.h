/*
 * ClapThreadCheck.h - Implements CLAP thread check extension
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

#ifndef LMMS_CLAP_THREAD_CHECK_H
#define LMMS_CLAP_THREAD_CHECK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <clap/ext/thread-check.h>

#include "ClapExtension.h"

namespace lmms
{

class ClapThreadCheck : public ClapExtension<clap_host_thread_check>
{
public:
	using ClapExtension::ClapExtension;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_THREAD_CHECK; }
	auto hostExt() const -> const clap_host_thread_check* override;

	static auto isMainThread() -> bool { return clapIsMainThread(nullptr); }
	static auto isAudioThread() -> bool { return clapIsAudioThread(nullptr); }

private:
	/**
	 * clap_host_thread_check implementation
	 */
	static auto clapIsMainThread(const clap_host* host) -> bool;
	static auto clapIsAudioThread(const clap_host* host) -> bool;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_THREAD_CHECK_H
