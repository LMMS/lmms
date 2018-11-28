/*
 * spainstrument.cpp - implementation of SPA interface
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include <spa/spa.h>

#include "AutomatableModel.h"
#include "SpaFxControlDialog.h"

#include "SpaEffect.h"

SpaEffect::SpaEffect(const char *libraryName,
	const Plugin::Descriptor * desc, Model* parent) :
	Effect(desc, parent, nullptr),
	m_controls(libraryName, this)
{
}

SpaEffect::~SpaEffect()
{
}

bool SpaEffect::processAudioBuffer(sampleFrame *buf, const fpp_t frames)
{
	if(!isEnabled() || !isRunning())
	{
		return false;
	}

	SpaFxControls& ctrl = m_controls;

	for (std::size_t f = 0; f < ctrl.m_ports.buffersize; ++f)
	{
		ctrl.m_ports.m_lUnprocessed[f] = buf[f][0];
		ctrl.m_ports.m_rUnprocessed[f] = buf[f][1];
	}

	m_controls.copyModelsToPorts();

//	m_pluginMutex.lock();
	ctrl.m_ports.samplecount = static_cast<unsigned>(frames);
	ctrl.m_plugin->run();
//	m_pluginMutex.unlock();
	for (std::size_t f = 0; f < ctrl.m_ports.buffersize; ++f)
	{
		buf[f][0] = ctrl.m_ports.m_lProcessed[f];
		buf[f][1] = ctrl.m_ports.m_rProcessed[f];
	}

	return isRunning();
}

void SpaEffect::writeOsc(const char *dest, const char *args, va_list va)
{
	spaControls()->writeOscInternal(dest, args, va);
}

void SpaEffect::writeOsc(const char *dest, const char *args, ...)
{
	va_list va;
	va_start(va, args);
	writeOsc(dest, args, va);
	va_end(va);
}

unsigned SpaEffect::netPort() const
{
	return spaControls()->m_plugin->net_port();
}

AutomatableModel *SpaEffect::modelAtPort(const QString &dest)
{
	return spaControls()->modelAtPort(dest);
}

