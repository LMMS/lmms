/*
 * RemotePluginBase.cpp - base class providing RPC like mechanisms
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "RemotePluginBase.h"

#ifndef BUILD_REMOTE_PLUGIN_CLIENT
#include <QCoreApplication>
#include <QThread>
#endif


namespace lmms
{


#ifdef SYNC_WITH_SHM_FIFO
RemotePluginBase::RemotePluginBase(shmFifo * _in, shmFifo * _out) :
	m_in(_in),
	m_out(_out)
#else
RemotePluginBase::RemotePluginBase() :
	m_socket(-1),
	m_invalid(false)
#endif
{
#ifdef LMMS_HAVE_LOCALE_H
	// make sure, we're using common ways to print/scan
	// floats to/from strings (',' vs. '.' for decimal point etc.)
	setlocale(LC_NUMERIC, "C");
#endif
#ifndef SYNC_WITH_SHM_FIFO
	pthread_mutex_init(&m_receiveMutex, nullptr);
	pthread_mutex_init(&m_sendMutex, nullptr);
#endif
}




RemotePluginBase::~RemotePluginBase()
{
#ifdef SYNC_WITH_SHM_FIFO
	delete m_in;
	delete m_out;
#else
	pthread_mutex_destroy(&m_receiveMutex);
	pthread_mutex_destroy(&m_sendMutex);
#endif
}




int RemotePluginBase::sendMessage(const message & _m)
{
#ifdef SYNC_WITH_SHM_FIFO
	m_out->lock();
	m_out->writeInt(_m.id);
	m_out->writeInt(_m.data.size());
	int j = 8;
	for (unsigned int i = 0; i < _m.data.size(); ++i)
	{
		m_out->writeString(_m.data[i]);
		j += 4 + _m.data[i].size();
	}
	m_out->unlock();
	m_out->messageSent();
#else
	pthread_mutex_lock(&m_sendMutex);
	writeInt(_m.id);
	writeInt(_m.data.size());
	int j = 8;
	for (const auto& str : _m.data)
	{
		writeString(str);
		j += 4 + str.size();
	}
	pthread_mutex_unlock(&m_sendMutex);
#endif

	return j;
}




RemotePluginBase::message RemotePluginBase::receiveMessage()
{
#ifdef SYNC_WITH_SHM_FIFO
	m_in->waitForMessage();
	m_in->lock();
	message m;
	m.id = m_in->readInt();
	const int s = m_in->readInt();
	for (int i = 0; i < s; ++i)
	{
		m.data.push_back(m_in->readString());
	}
	m_in->unlock();
#else
	pthread_mutex_lock(&m_receiveMutex);
	message m;
	m.id = readInt();
	const int s = readInt();
	for (int i = 0; i < s; ++i)
	{
		m.data.push_back(readString());
	}
	pthread_mutex_unlock(&m_receiveMutex);
#endif
	return m;
}




RemotePluginBase::message RemotePluginBase::waitForMessage(
							const message & _wm,
							bool _busy_waiting)
{
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
	if (_busy_waiting)
	{
		// No point processing events outside of the main thread
		_busy_waiting = QThread::currentThread() ==
					QCoreApplication::instance()->thread();
	}

	struct WaitDepthCounter
	{
		WaitDepthCounter(int & depth, bool busy) :
			m_depth(depth),
			m_busy(busy)
		{
			if (m_busy) { ++m_depth; }
		}

		~WaitDepthCounter()
		{
			if (m_busy) { --m_depth; }
		}

		int & m_depth;
		bool m_busy;
	};

	WaitDepthCounter wdc(waitDepthCounter(), _busy_waiting);
#endif
	while (!isInvalid())
	{
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
		if (_busy_waiting && !messagesLeft())
		{
			QCoreApplication::processEvents(
				QEventLoop::ExcludeUserInputEvents, 50);
			continue;
		}
#endif
		message m = receiveMessage();
		processMessage(m);
		if (m.id == _wm.id)
		{
			return m;
		}
		else if (m.id == IdUndefined)
		{
			return m;
		}
	}

	return message();
}

} // namespace lmms
