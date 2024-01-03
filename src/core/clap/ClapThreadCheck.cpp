/*
 * ClapThreadCheck.cpp - Implements CLAP thread check extension
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapThreadCheck.h"

#ifdef LMMS_HAVE_CLAP

#include <QCoreApplication>
#include <QThread>

namespace lmms
{

auto ClapThreadCheck::hostExt() const -> const clap_host_thread_check*
{
	static clap_host_thread_check ext {
		&clapIsMainThread,
		&clapIsAudioThread
	};
	return &ext;
}

auto ClapThreadCheck::clapIsMainThread([[maybe_unused]] const clap_host* host) -> bool
{
	return QThread::currentThread() == QCoreApplication::instance()->thread();
}

auto ClapThreadCheck::clapIsAudioThread([[maybe_unused]] const clap_host* host) -> bool
{
	// Assume any non-GUI thread is an audio thread
	return QThread::currentThread() != QCoreApplication::instance()->thread();
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
