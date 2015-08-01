/*
 * OscMsgListener: virtual base class to listen for Open Sound Control messages sent from core/gui
 *   and handle them appropriately.
 *   If you have a class that wants to listen in on OSC messages, just derive it from this,
 *   implement the appropriate callbacks & register it with some OSC broadcaster (e.g. Messenger)
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

#include "OscMsgListener.h"

#include <QMutexLocker>
#include <QThread>

// Proper use of the OscMsgListener is to listen for messages in a different thread that the one that receives them.
// This class merely allows one to easily spawn a new thread and call listenerLoop() from that thread
class OscMsgListenerThread : public QThread
{
public:
	OscMsgListenerThread(OscMsgListener *self) : m_self(self)
	{
	}
	// override of QThread::run.
	// When the creating thread calls OscMsgListenerThread.start(),
	// that will trigger run() to be called from a new thread.
	void run()
	{
		m_self->listenerLoop();
	}
private:
	OscMsgListener *m_self;
};



OscMsgListener::~OscMsgListener()
{
}

void OscMsgListener::listenerLoop()
{
	do
	{
		QByteArray received;
		{
			QMutexLocker rxLock(&m_rxMutex);
			// wait for new data
			while (m_rxQueue.isEmpty())
			{
				m_rxConditionVar.wait(&m_rxMutex);
			}
			received = m_rxQueue.dequeue();
		}

		processMessage(received);
	} while (true);
}

void OscMsgListener::listenInNewThread()
{
	OscMsgListenerThread *listener = new OscMsgListenerThread(this);
	listener->start();
}

void OscMsgListener::queue(const char *msg, std::size_t length)
{
	// copy the message into the queue, and then wake any listeners
	{
		QMutexLocker rxLock(&m_rxMutex);
		m_rxQueue.enqueue(QByteArray(msg, length));
	}
	m_rxConditionVar.wakeOne();
}