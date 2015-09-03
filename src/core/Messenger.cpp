/*
 * Messenger: class to abstract the routing of Open Sound Control messages from the core
 *   to another area of the core and/or the gui
 *
 * (c) 2015 Colin Wallace (https://github.com/Wallacoloo)
 *
 * This file is part of LMMS - http://lmms.io
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
 */

#include "Messenger.h"

#include <QReadLocker>
#include <QWriteLocker>


// define endpoint names
const char *Messenger::Endpoints::Warning              = "/status/warning";
const char *Messenger::Endpoints::Error                = "/status/error";
const char *Messenger::Endpoints::WaveTableInit        = "/wavetable/init";
const char *Messenger::Endpoints::MixerDevInit         = "/mixer/devices/init";
const char *Messenger::Endpoints::MixerProcessingStart = "/mixer/processing/start";
const char *Messenger::Endpoints::FxMixerPeaks         = "/fxmixer/peaks";


ManagedLoAddress::ManagedLoAddress(lo_address &&addr)
: m_addr(std::move(addr))
{
}
ManagedLoAddress::ManagedLoAddress(ManagedLoAddress &&other)
: m_addr(std::move(other.m_addr))
{
	other.m_addr = NULL;
}

ManagedLoAddress::~ManagedLoAddress()
{
	if (m_addr)
	{
		lo_address_free(m_addr);
	}
}

ManagedLoAddress::operator const lo_address & () const
{
	return m_addr;
}
bool ManagedLoAddress::operator< (const ManagedLoAddress &other) const
{
	return m_addr < other.m_addr;
}



void Messenger::broadcastWaveTableInit()
{
	broadcast(Endpoints::WaveTableInit);
}
void Messenger::broadcastMixerDevInit()
{
	broadcast(Endpoints::MixerDevInit);
}
void Messenger::broadcastMixerProcessing()
{
	broadcast(Endpoints::MixerProcessingStart);
}


void Messenger::broadcastFxMixerPeaks(std::size_t numFxCh, const float peaks[][2])
{
	lo_message oscMsg = lo_message_new();

	for (std::size_t fxNo=0; fxNo<numFxCh; ++fxNo)
	{
		for (int ch=0; ch<2; ++ch)
		{
			lo_message_add_float(oscMsg, peaks[fxNo][ch]);
		}
	}
	broadcast(Endpoints::FxMixerPeaks, oscMsg);
	lo_message_free(oscMsg);
}

void Messenger::broadcastWarning(const QString &brief, const QString &msg)
{
	lo_message oscMsg = lo_message_new();
	lo_message_add_string(oscMsg, brief.toUtf8().data());
	lo_message_add_string(oscMsg, msg.toUtf8().data());
	broadcast(Endpoints::Warning, oscMsg);
	lo_message_free(oscMsg);
}

void Messenger::broadcastError(const QString &brief, const QString &msg)
{
	lo_message oscMsg = lo_message_new();
	lo_message_add_string(oscMsg, brief.toUtf8().data());
	lo_message_add_string(oscMsg, msg.toUtf8().data());
	broadcast(Endpoints::Error, oscMsg);
	lo_message_free(oscMsg);
}

void Messenger::addListener(const QString &endpoint, lo_address &&address)
{
	QWriteLocker listenerLocker(&m_listenersLock);
	m_listeners[endpoint].insert(std::move(ManagedLoAddress(std::move(address))));
}


void Messenger::broadcast(const char *endpoint, lo_message msg)
{
	QReadLocker listenerLocker(&m_listenersLock);
	// send the message to all addresses listening at the given endpoint
	for (const lo_address &addr : m_listeners[endpoint])
	{
		lo_send_message(addr, endpoint, msg);
	}
}

void Messenger::broadcast(const char *endpoint)
{
	lo_message msg = lo_message_new();
	broadcast(endpoint, msg);
	lo_message_free(msg);
}