/*
 * ClapTimerSupport.h - Implements CLAP timer support extension
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

#ifndef LMMS_CLAP_TIMER_SUPPORT_H
#define LMMS_CLAP_TIMER_SUPPORT_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <QObject>

#include <clap/ext/timer-support.h>

namespace lmms
{

class ClapTimerSupport : public QObject
{
	Q_OBJECT
public:
	friend class ClapInstance;

	auto supported() const -> bool { return m_ext != nullptr; }

private:

	using QObject::QObject;

	auto init(const clap_plugin* plugin) -> bool;

	/**
	 * clap_host_timer_support implementation
	 */
	static auto clapRegisterTimer(const clap_host* host, std::uint32_t periodMilliseconds, clap_id* timerId) -> bool;
	static auto clapUnregisterTimer(const clap_host* host, clap_id timerId) -> bool;

	void timerEvent(QTimerEvent* event) override;

	const clap_plugin* m_plugin = nullptr;
	const clap_plugin_timer_support* m_ext = nullptr;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_TIMER_SUPPORT_H
