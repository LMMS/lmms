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

#include <QByteArray>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>

class OscMsgListener
{
	public:
		virtual ~OscMsgListener();
		// call to asyncronously push a message into the received queue
		void queue(const char *msg, std::size_t length);
		// Continually wait for new messages and process them as they arrive.
		void listenerLoop();
		// spawn a new thread and have it call listenerLoop()
		void listenInNewThread();
	protected:
		// called whenever a message is received
		virtual void processMessage(const QByteArray &msg) = 0;
	private:
		// queue of unprocessed received messages
		QQueue<QByteArray> m_rxQueue;
		QMutex m_rxMutex;
		QWaitCondition m_rxConditionVar;
};