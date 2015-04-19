/*
 * Messenger.cpp - class Messenger, a singleton that helps route string-based messages through core <-> gui
 *
 * Copyright (c) 2015 Colin Wallace <wallacoloo/at/gmail.com>
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
 *
 */

#include "Messenger.h"

#include <cstdlib>

QSemaphore Messenger::accessSemaphore(Messenger::maxReaders);
QVector<MessageReceiver*> Messenger::receivers[Message::NUM_MESSAGE_TYPES];


Message::Message(const QString &msg, MessageType type)
  : m_msg(msg),
    m_type(type)
{
}

const QString& Message::getMessage() const 
{
	return m_msg;
}

Message::MessageType Message::getType() const 
{
	return m_type;
}


MessageReceiverFuncPtr::MessageReceiverFuncPtr(void (*receiverFunc)(const Message &msg))
  : m_receiverFunc(receiverFunc)
{
}

void MessageReceiverFuncPtr::messageReceived(const Message &msg)
{
	m_receiverFunc(msg);
}


MessageReceiverHandle::MessageReceiverHandle(MessageReceiver *messageReceiver, Message::MessageType subscriptionType)
  : m_messageReceiver(messageReceiver),
    m_subscriptionType(subscriptionType)
{
}

MessageReceiverHandle::~MessageReceiverHandle()
{
	// remove the corresponding receiver from the Messenger's callback list
	if (m_messageReceiver)
	{
		Messenger::removeReceiver(m_messageReceiver, m_subscriptionType);
	}
}

MessageReceiverHandle Messenger::subscribe(MessageReceiver *receiver, Message::MessageType subscriptionType)
{
	accessSemaphore.acquire(maxReaders);
	receivers[subscriptionType].append(receiver);
	accessSemaphore.release(maxReaders);
	return MessageReceiverHandle(receiver, subscriptionType);
}

void Messenger::broadcast(Message msg)
{
	// send the message to all receivers subscribed to the message's type
	accessSemaphore.acquire();
	QVector<MessageReceiver*>::iterator begin = receivers[msg.getType()].begin();
	QVector<MessageReceiver*>::iterator end = receivers[msg.getType()].end();
	for (QVector<MessageReceiver*>::iterator i=begin; i != end; ++i)
	{
		(*i)->messageReceived(msg);
	}
	accessSemaphore.release();
}

void Messenger::broadcast(const QString &msg, Message::MessageType type)
{
	broadcast(Message(msg, type));
}

void Messenger::removeReceiver(MessageReceiver *receiver, Message::MessageType subscriptionType)
{
	accessSemaphore.acquire(maxReaders);
	int i = receivers[subscriptionType].indexOf(receiver);
	if (i == -1)
	{
		// receiver function is not registered as a callback
		qWarning("Messenger::removeReceiver: attempt to remove a non-existent message receiver");
	}
	else
	{
		// remove the receiver from the associated callback vector
		delete receivers[subscriptionType][i];
		receivers[subscriptionType].remove(i);
	}
	accessSemaphore.release(maxReaders);
}