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

#ifndef MESSENGER_H
#define MESSENGER_H

#include <cstring>
#include <map>
#include <set>

#include <QReadWriteLock>
#include <QString>

#include <lo/lo.h>


// automatically calls lo_address_free on the managed lo_address upon deletion
class ManagedLoAddress
{
public:
	ManagedLoAddress(lo_address &&addr);
	ManagedLoAddress(ManagedLoAddress &&other);
	~ManagedLoAddress();

	operator const lo_address & () const;
	bool operator< (const ManagedLoAddress &other) const;
private:
	lo_address m_addr;
};


class Messenger
{
public:
	// const strings to specify the Open Sound Control endpoint locations,
	// e.g. "/mixer/ch/n/volume"
	class Endpoints
	{
	public:
		// Endpoints to send general and already-translated warnings / error messages
		static const char *Warning;
		static const char *Error;

		static const char *WaveTableInit;
		static const char *MixerDevInit;
		static const char *MixerProcessingStart;

		static const char *FxMixerPeaks;
	};

	// Send message to indicate that the following components have completed their initialization
	void broadcastWaveTableInit();
	// Mixer has opened its audio devices
	void broadcastMixerDevInit();
	// Indicates that mixer has started its processing thread(s).
	void broadcastMixerProcessing();
	// Broadcast the left/right channel peak values of each Fx channel in the mixer
	void broadcastFxMixerPeaks(std::size_t numFxCh, const float peaks[][2]);

	// whenever the core encounters a warning, it can broadcast it to listeners
	//   rather than explicitly pop a Qt Dialog / log it, etc.
	void broadcastWarning(const QString &brief, const QString &warning);

	// broadcast an error message. The fact that it's an error does *not* imply
	//   that the core/gui should exit.
	// @brief is one-line summary of the error,
	// @msg is the full message
	void broadcastError(const QString &brief, const QString &msg);

	// make it so any messages destined for the given @endpoint will be sent
	// to the host at @address
	// note: This transfers the ownership of @address to Messenger so do not
	// explicitly free it afterwards
	void addListener(const QString &endpoint, lo_address &&address);
private:
	// dispatch an OSC formatted message to all addresses listening to the
	// given endpoint
	void broadcast(const char *endpoint, lo_message msg);
	// broadcast an empty message
	void broadcast(const char *endpoint);

	// map of endpoint path -> list of hosts interested in that type of message
	std::map<QString, std::set<ManagedLoAddress> > m_listeners;
	QReadWriteLock m_listenersLock;
};

#endif