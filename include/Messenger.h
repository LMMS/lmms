/*
 * Messenger: class to abstract the routing of Open Sound Control messages from the core
 *   to another area of the core and/or the gui
 *
 * (c) 2015 Colin Wallace (https://github.com/Wallacoloo)
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This file is released under the MIT License.
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MESSENGER_H
#define MESSENGER_H

#include <cstring>

#include <QReadWriteLock>
#include <QSet>

class QString;

class OscMsgListener;


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
	};

	// Send message to indicate that the following components have completed their initialization
	void broadcastWaveTableInit();
	// Mixer has opened its audio devices
	void broadcastMixerDevInit();
	// Indicates that mixer has started its processing thread(s).
	void broadcastMixerProcessing();

	// whenever the core encounters a warning, it can broadcast it to listeners
	//   rather than explicitly pop a Qt Dialog / log it, etc.
	void broadcastWarning(const QString &brief, const QString &warning);

	// broadcast an error message. The fact that it's an error does *not* imply
	//   that the core/gui should exit.
	// @brief is one-line summary of the error,
	// @msg is the full message
	void broadcastError(const QString &brief, const QString &msg);

	// add a function to listen for OSC messages directed to a gui
	// NOTE: This handler should have hard timing guarantees
	//   preferrably, it will just enqueue the message and then process it later *in a separate thread*
	void addGuiOscListener(OscMsgListener *listener);
private:
	// dispatch an OSC formatted message to ALL attached listeners
	// if length is 0, an error will be logged and no action taken
	// Therefore this can be safely used like so:
	// char buffer[512];
	// broadcast(buffer, rtosc_message(buffer, 512, "/test", "s", "Test broadcast"))
	void broadcast(const char *buffer, std::size_t length);

	// list of handlers that are called whenever we wish to broadcast a message to all GUIs
	QSet<OscMsgListener*> m_guiListeners;
	QReadWriteLock m_guiListenersLock;
};

#endif