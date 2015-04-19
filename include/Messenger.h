/*
 * Messenger.h - class Messenger, a singleton that helps route string-based messages through core <-> gui
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

#ifndef MESSENGER_H
#define MESSENGER_H

#include <QVector>
#include <QString>
#include <QSemaphore>

class Message
{
	// This object is passed to MessageReceivers that are set to listen to the relevant message type.
public:
	enum MessageType
	{
		INIT_STATUS, 
		NUM_MESSAGE_TYPES // KEEP THIS AT THE END OF THE LIST. It's used to determine the number of possible message types.
	};

	Message(const QString &msg, MessageType type);

	const QString& getMessage() const;
	MessageType getType() const;
private:

	QString m_msg;
	MessageType m_type;
};


class MessageReceiver
{
	// This represents a Messenger callback in a way 
	//   such that static functions, member functions, and non-member functions can all receive messages
	friend class Messenger;
	virtual void messageReceived(const Message &msg) = 0;
protected:
	virtual ~MessageReceiver() {};
};


class MessageReceiverFuncPtr : public MessageReceiver
{
	// MessageReceiver implementation for C function pointer callback functions
public:
	MessageReceiverFuncPtr(void (*receiverFunc)(const Message &msg));
private:
	void (*m_receiverFunc)(const Message &msg);
	void messageReceived(const Message &msg);
};


template <typename T> class MessageReceiverMemberFunc : public MessageReceiver
{
	// MessageReceiver implementation for member callback functions
public:
	MessageReceiverMemberFunc(void (T::*memberFunction)(const Message &msg), T *this_)
	: m_this(this_),
	  m_memberFunction(memberFunction)
	{
	}
private:
	T *m_this;
	void (T::*m_memberFunction)(const Message &msg);
	void messageReceived(const Message &msg)
	{
		(m_this->*m_memberFunction)(msg);
	}
};


class MessageReceiverHandle
{
	// This is used for automatically removing a message handler from the callback list
	//   when it's owner goes out-of-scope.
	// e.g. store the MessageReceiverHandle returned from Messenger::registerHandler as a member variable 
	//   to have the handler removed when the instance is destroyed.
	MessageReceiver *m_messageReceiver;
	Message::MessageType m_subscriptionType;
public:
	MessageReceiverHandle(MessageReceiver *messageReceiver, Message::MessageType subscriptionType);
	~MessageReceiverHandle();
};


class Messenger
{
	friend class MessageReceiverHandle;
public:
	static void broadcast(Message msg);
	static void broadcast(const QString &msg, Message::MessageType type);

	static MessageReceiverHandle subscribe(MessageReceiver *receiver, Message::MessageType subscriptionType);

	// subscribe overload for C function pointers
	inline static MessageReceiverHandle subscribe(void (*receiverFunc)(const Message &msg), Message::MessageType subscriptionType)
	{
		return subscribe(new MessageReceiverFuncPtr(receiverFunc), subscriptionType);
	}
	// subscribe overload for member function pointers
	template <typename T> static MessageReceiverHandle subscribe(void (T::*memberFunction)(const Message &msg), T *this_,
   																 Message::MessageType subscriptionType)
	{
		return subscribe(new MessageReceiverMemberFunc<T>(memberFunction, this_), subscriptionType);
	}
	
private:
	static void removeReceiver(MessageReceiver *receiver, Message::MessageType subscriptionType);

	// use a semaphore in order to allow many threads to iterate through the message handlers,
	// but a call to subscribe must block ALL threads
	static QSemaphore accessSemaphore;
	static const int maxReaders = 32; // 32 is a mostly arbitrary choice. Any large number should do.

	// store callback pointers in an array of vectors. 
	// All callbacks listening for a Message::MessageType `type' are contained in receivers[type]
	static QVector<MessageReceiver*> receivers[Message::NUM_MESSAGE_TYPES];
};

#endif