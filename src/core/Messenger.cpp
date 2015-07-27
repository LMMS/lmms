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

#include "Messenger.h"

#include <QReadLocker>
#include <QString>
#include <QWriteLocker>

#include "rtosc/rtosc.h"

#include "OscMsgListener.h"


// define endpoint names
const char *Messenger::Endpoints::InitMsg = "/status/initmsg";


void Messenger::broadcastInitMsg(const QString &msg)
{
	char oscMsg[512];
	broadcast(oscMsg, rtosc_message(oscMsg, 512, Endpoints::InitMsg, "s", msg.toUtf8().data()));
}

void Messenger::addGuiOscListener(OscMsgListener *listener)
{
	QWriteLocker guiLocker(&m_guiListenersLock);
	m_guiListeners.insert(listener);
}

void Messenger::broadcast(const char* buffer, std::size_t length)
{
	if (length)
	{
		// send this message to all GUI listeners
		QReadLocker guiLocker(&m_guiListenersLock);
		for (QSet<OscMsgListener *>::const_iterator i=m_guiListeners.constBegin(); i != m_guiListeners.constEnd(); ++i)
		{
			(*i)->queue(buffer, length);
		}
	}
}
