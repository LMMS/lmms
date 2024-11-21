/*
 * MidiApple.cpp - Apple MIDI client
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2015 Maurizio Lo Bosco (rageboge on github)
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

#include "MidiApple.h"

#ifdef LMMS_BUILD_APPLE

#include <QtAlgorithms>
#include <algorithm>

#include "ConfigManager.h"
#include "MidiPort.h"
#include "Note.h"

#include <CoreMIDI/CoreMIDI.h>


namespace lmms
{


const unsigned int SYSEX_LENGTH=1024;

MidiApple::MidiApple() :
	MidiClient(),
	m_inputDevices(),
	m_outputDevices(),
	m_inputSubs(),
	m_outputSubs()
{
	openDevices();
}



MidiApple::~MidiApple()
{
	closeDevices();
}



void MidiApple::processOutEvent( const MidiEvent& event, const TimePos& time, const MidiPort* port )
{
	qDebug("MidiApple:processOutEvent displayName:'%s'",port->displayName().toLatin1().constData());
	
	QStringList outDevs;
	for( SubMap::ConstIterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
	{
		for( MidiPortList::ConstIterator jt = it.value().begin(); jt != it.value().end(); ++jt )
		{
			if( *jt == port )
			{
				outDevs += it.key();
				break;
			}
		}
	}
	
	for( QMap<QString, MIDIEndpointRef>::Iterator it = m_outputDevices.begin(); it != m_outputDevices.end(); ++it )
	{
		if( outDevs.contains( it.key() ) )
		{
			sendMidiOut( it.value(), event );
		}
	}

}



void MidiApple::sendMidiOut( MIDIEndpointRef &endPointRef, const MidiEvent& event )
{
	MIDIPacketList packetList=createMidiPacketList(event);
	MIDIPortRef port = m_sourcePortRef.value(endPointRef);
	MIDISend(port, endPointRef, &packetList);
}



MIDIPacketList MidiApple::createMidiPacketList( const MidiEvent& event )
{
	MIDIPacketList packetList;
	packetList.numPackets = 1;
	MIDIPacket* firstPacket = &packetList.packet[0];
	firstPacket->timeStamp = 0; // send immediately
	firstPacket->length = 3;
	firstPacket->data[0] = ( event.type() + event.channel() );
	firstPacket->data[1] = ( event.param( 0 ) & 0xff );
	firstPacket->data[2] = ( event.param( 1 ) & 0xff );
	return packetList;
}



void MidiApple::applyPortMode( MidiPort* port )
{
	qDebug("applyPortMode displayName:'%s'",port->displayName().toLatin1().constData());
	// make sure no subscriptions exist which are not possible with
	// current port-mode
	if( !port->isInputEnabled() )
	{
		for( SubMap::Iterator it = m_inputSubs.begin(); it != m_inputSubs.end(); ++it )
		{
			it.value().removeAll( port );
		}
	}
	
	if( !port->isOutputEnabled() )
	{
		for( SubMap::Iterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
		{
			it.value().removeAll( port );
		}
	}
}



void MidiApple::removePort( MidiPort* port )
{
	qDebug("removePort displayName:'%s'",port->displayName().toLatin1().constData());
	for( SubMap::Iterator it = m_inputSubs.begin(); it != m_inputSubs.end(); ++it )
	{
		it.value().removeAll( port );
	}
	
	for( SubMap::Iterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
	{
		it.value().removeAll( port );
	}
	
	MidiClient::removePort( port );
}



QString MidiApple::sourcePortName( const MidiEvent& event ) const
{
	qDebug("sourcePortName");
	/*
	if( event.sourcePort() )
	{
		return m_inputDevices.value( *static_cast<const HMIDIIN *>( event.sourcePort() ) );
	}
	*/
	return MidiClient::sourcePortName( event );
}



void MidiApple::subscribeReadablePort( MidiPort* port, const QString& dest, bool subscribe )
{
	qDebug("subscribeReadablePort %s subscribe=%d",dest.toLatin1().constData(),subscribe);
	if( subscribe && port->isInputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!", port->displayName().toLatin1().constData() );
		return;
	}
	
	m_inputSubs[dest].removeAll( port );
	if( subscribe )
	{
		qDebug("Subscribing %s",dest.toLatin1().constData());
		m_inputSubs[dest].push_back( port );
	}
	else
	{
		MidiPortList list = m_inputSubs[dest];
		if(list.empty()){
			m_inputSubs.remove(dest);
		}
	}
}



void MidiApple::subscribeWritablePort( MidiPort* port, const QString& dest, bool subscribe )
{
	qDebug("subscribeWritablePort %s", port->displayName().toLatin1().constData());
	
	if( subscribe && port->isOutputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!", port->displayName().toLatin1().constData() );
		return;
	}
	
	m_outputSubs[dest].removeAll( port );
	if( subscribe )
	{
		m_outputSubs[dest].push_back( port );
	}
	else
	{
		MidiPortList list = m_outputSubs[dest];
		if(list.empty()){
			m_outputSubs.remove(dest);
		}
	}
}



void MidiApple::ReadCallback( const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon )
{
	MidiApple *caller = static_cast<MidiApple*>(readProcRefCon);
	if (!caller)
	{
		qDebug("Error: !caller: MidiApple::ReadCallback");
		return;
	}
	caller->HandleReadCallback(pktlist,srcConnRefCon);
}



void MidiApple::HandleReadCallback( const MIDIPacketList *pktlist, void *srcConnRefCon )
{
	const char * refName = (const char *) srcConnRefCon;

	MIDIEndpointRef endPointRef = m_inputDevices.value(refName);
	if( !m_inputSubs.contains( refName ) )
	{
//		qDebug("HandleReadCallback '%s' not subscribed",refName);
//		printQStringKeys("m_inputDevices", m_inputDevices);
		return;
	}
//	qDebug("HandleReadCallback '%s' subscribed",refName);
	bool continueSysEx = false;
	unsigned int nBytes;
	const MIDIPacket *packet = &pktlist->packet[0];
	unsigned char sysExMessage[SYSEX_LENGTH];
	unsigned int sysExLength = 0;
	
	for (uint32_t i=0; i<pktlist->numPackets; ++i)
	{
		nBytes = packet->length;
		// Check if this is the end of a continued SysEx message
		if (continueSysEx) {
			unsigned int lengthToCopy = std::min(nBytes, SYSEX_LENGTH - sysExLength);
			// Copy the message into our SysEx message buffer,
			// making sure not to overrun the buffer
			memcpy(sysExMessage + sysExLength, packet->data, lengthToCopy);
			sysExLength += lengthToCopy;
			// Check if the last byte is SysEx End.
			continueSysEx = (packet->data[nBytes - 1] == 0xF7);
			if (!continueSysEx || sysExLength == SYSEX_LENGTH) {
				// We would process the SysEx message here, as it is we're just ignoring it
				
				sysExLength = 0;
			}
		}
		else
		{
			UInt16 iByte, size;
			
			iByte = 0;
			while (iByte < nBytes)
			{
				size = 0;
				
				// First byte should be status
				unsigned char status = packet->data[iByte];
				if (status < 0xC0) {
					size = 3;
				}
				else if (status < 0xE0)
				{
					size = 2;
				}
				else if (status < 0xF0)
				{
					size = 3;
				}
				else if (status == 0xF0)
				{
					// MIDI SysEx then we copy the rest of the message into the SysEx message buffer
					unsigned int lengthLeftInMessage = nBytes - iByte;
					unsigned int lengthToCopy = std::min(lengthLeftInMessage, SYSEX_LENGTH);
					
					memcpy(sysExMessage + sysExLength, packet->data, lengthToCopy);
					sysExLength += lengthToCopy;
					
					size = 0;
					iByte = nBytes;
					
					// Check whether the message at the end is the end of the SysEx
					continueSysEx = (packet->data[nBytes - 1] != 0xF7);
				}
				else if (status < 0xF3)
				{
					size = 3;
				}
				else if (status == 0xF3)
				{
					size = 2;
				}
				else
				{
					size = 1;
				}
				
				unsigned char messageChannel = status & 0xF;
				const MidiEventTypes cmdtype = static_cast<MidiEventTypes>(status & 0xF0);
				const int par1 = packet->data[iByte + 1];
				const int par2 = packet->data[iByte + 2];

				switch (cmdtype)
				{
					case MidiNoteOff:			//0x80:
					case MidiNoteOn:			//0x90:
					case MidiKeyPressure:		//0xA0:
					case MidiControlChange:		//0xB0:
					case MidiProgramChange:		//0xC0:
					case MidiChannelPressure:	//0xD0:
						notifyMidiPortList(
							m_inputSubs[refName],
							MidiEvent(cmdtype, messageChannel, par1, par2 & 0xff, &endPointRef));
						break;
					case MidiPitchBend:			//0xE0:
						notifyMidiPortList(
							m_inputSubs[refName],
							MidiEvent(cmdtype, messageChannel, par1 + par2 * 128, 0, &endPointRef));
						break;
					case MidiActiveSensing:		//0xF0
					case 0xF0:
						break;
					default:
						qDebug("endPointRef name='%s':Some other message %d", refName, cmdtype);
						break;
				}
				iByte += size;
			}
		}
		packet = MIDIPacketNext(packet);
	}
}



void MidiApple::updateDeviceList()
{
	closeDevices();
	openDevices();
	
	emit readablePortsChanged();
	emit writablePortsChanged();
}



void MidiApple::closeDevices()
{
	m_inputSubs.clear();
	m_outputSubs.clear();

	QMapIterator<QString, MIDIEndpointRef> i( m_inputDevices );
	while( i.hasNext() )
	{
		midiInClose( i.next().value() );
	}
	
	QMapIterator<QString, MIDIEndpointRef> o( m_outputDevices );
	while( o.hasNext() )
	{
		midiInClose( o.next().value() );
	}
	
	m_inputDevices.clear();
	m_outputDevices.clear();
}



void MidiApple::midiInClose( MIDIEndpointRef reference )
{
	qDebug("midiInClose '%s'", getFullName(reference));
	MIDIPortRef portRef = m_sourcePortRef[reference];
	MIDIPortDisconnectSource(portRef, reference);
}



char *getName( const MIDIObjectRef &object )
{
	// Returns the name of a given MIDIObjectRef as char *
	CFStringRef name = nullptr;
	if (noErr != MIDIObjectGetStringProperty(object, kMIDIPropertyName, &name))
		return nullptr;
	int len = CFStringGetLength(name)+1;
	char *value = (char *) malloc(len);
	
	CFStringGetCString(name, value, len, 0);
	CFRelease(name);
	return value;
}



const void printQStringKeys( char const * mapName, const QMap<QString,MIDIEndpointRef> &inputMap )
{
	qDebug("%s:", mapName);
	QMapIterator<QString, MIDIEndpointRef> i( inputMap );
	while( i.hasNext() )
	{
		QString key = i.next().key();
		qDebug("  key='%s'",  key.toLatin1().constData());
	}
}



void MidiApple::openDevices()
{
	qDebug("openDevices");
	m_inputDevices.clear();
	// How many MIDI devices do we have?
	ItemCount deviceCount = MIDIGetNumberOfDevices();
	
	// Iterate through all MIDI devices
	for (ItemCount i = 0 ; i < deviceCount ; ++i)
	{
		// Grab a reference to current device
		MIDIDeviceRef device = MIDIGetDevice(i);
		char * deviceName = getName(device);
		QString qsDeviceName = QString::fromUtf8((char*)(deviceName));
		qDebug("Device name:%s",deviceName);
		
		// Is this device online? (Currently connected?)
		SInt32 isOffline = 0;
		MIDIObjectGetIntegerProperty(device, kMIDIPropertyOffline, &isOffline);
		qDebug(" is online: %s", (isOffline ? "No" : "Yes"));
		// How many entities do we have?
		ItemCount entityCount = MIDIDeviceGetNumberOfEntities(device);
		
		// Iterate through this device's entities
		for (ItemCount j = 0 ; j < entityCount ; ++j)
		{
			// Grab a reference to an entity
			MIDIEntityRef entity = MIDIDeviceGetEntity(device, j);
			qDebug("  Entity: %s", getName(entity));
			
			// Iterate through this device's source endpoints (MIDI In)
			ItemCount sourceCount = MIDIEntityGetNumberOfSources(entity);
			for ( ItemCount k = 0 ; k < sourceCount ; ++k )
			{
				// Grab a reference to a source endpoint
				MIDIEndpointRef source = MIDIEntityGetSource(entity, k);
				char * name = getName(source);
				qDebug("	Source: '%s'", name);
				QString sourceName = qsDeviceName + ":" + QString::fromUtf8((char*)(name));
				qDebug("	Source name: '%s'", sourceName.toLatin1().constData() );
				m_inputDevices.insert(sourceName, source);
				openMidiReference(source,sourceName,true);
			}
			
			// Iterate through this device's destination endpoints (MIDI Out)
			ItemCount destCount = MIDIEntityGetNumberOfDestinations(entity);
			for ( ItemCount k = 0 ; k < destCount ; ++k )
			{
				// Grab a reference to a destination endpoint
				MIDIEndpointRef dest = MIDIEntityGetDestination(entity, k);
				char * name = getName(dest);
				qDebug("	Destination: '%s'", name);
				QString destinationName = qsDeviceName + ":" + QString::fromUtf8((char*)(name));
				qDebug("	Destination name: '%s'", destinationName.toLatin1().constData() );
				m_outputDevices.insert(destinationName, dest);
				openMidiReference(dest,destinationName,false);
			}
		}
		qDebug("------");
	}
	printQStringKeys("m_inputDevices:",m_inputDevices);
	printQStringKeys("m_outputDevices:",m_outputDevices);
}



void MidiApple::openMidiReference( MIDIEndpointRef reference, QString refName, bool isIn )
{
	char * registeredName = (char*) malloc(refName.length()+1);
	std::snprintf(registeredName, refName.length() + 1, "%s",refName.toLatin1().constData());
	qDebug("openMidiReference refName '%s'",refName.toLatin1().constData());
	
	MIDIClientRef mClient = getMidiClientRef();
	MIDIPortRef mPort = 0;
	
	CFStringRef inName = CFStringCreateWithCString(0, registeredName, kCFStringEncodingASCII);
	if(isIn)
	{
		MIDIInputPortCreate(mClient, inName, &MidiApple::ReadCallback, this, &mPort);
	}
	else
	{
		MIDIOutputPortCreate(mClient, inName, &mPort);
	}
	MIDIPortConnectSource(mPort, reference, (void *)registeredName);
	m_sourcePortRef.insert(reference, mPort);
	CFRelease(inName);
	qDebug("openMidiReference registeredName '%s'",registeredName);
}



MIDIClientRef MidiApple::getMidiClientRef()
{
	if(mClient==0)
	{
		CFStringRef deviceClientName = CFSTR("MIDI In Device Client");
		MIDIClientCreate(deviceClientName, NotifyCallback, this, &mClient);
		CFRelease(deviceClientName);
	}
	return mClient;
}



void MidiApple::notifyMidiPortList( MidiPortList l, MidiEvent event )
{
	for( MidiPortList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		( *it )->processInEvent( event);
	}
}



void MidiApple::NotifyCallback( const MIDINotification *message, void *refCon )
{
	//refCon is a pointer to MidiApple class
	MidiApple *midiApple = (MidiApple *)refCon;
	qDebug("MidiApple::NotifyCallback '%d'",message->messageID);
	switch (message->messageID)
	{
		case kMIDIMsgObjectAdded:
		{
			MIDIObjectAddRemoveNotification* msg = (MIDIObjectAddRemoveNotification*)message ;
			MIDIEndpointRef endpoint_ref = (MIDIEndpointRef)msg->child ;
			char * fullName = midiApple->getFullName(endpoint_ref);

			if (msg->childType == kMIDIObjectType_Source) {
				qDebug("kMIDIMsgObjectAdded source '%s'",fullName);
				// Here your code to save the new source ref
				// and update your internal "clients" (eg
				// update a popup menu, a list, ...)
			}
			if (msg->childType == kMIDIObjectType_Destination) {
				qDebug("kMIDIMsgObjectAdded destination '%s'",fullName);
				// Here your code to save the new destination ref
				// and update your internal "clients"
			}
			break;
		}
		case kMIDIMsgObjectRemoved:
		{
			MIDIObjectAddRemoveNotification*
			msg = (MIDIObjectAddRemoveNotification*)message ;
			MIDIEndpointRef endpoint_ref = (MIDIEndpointRef)msg->child ;
			char * fullName = midiApple->getFullName(endpoint_ref);
			
			if (msg->childType == kMIDIObjectType_Source) {
				// Here your code to remove the source ref
				// and update your internal "clients"
				qDebug("kMIDIMsgObjectRemoved source '%s'",fullName);
			}
			if (msg->childType == kMIDIObjectType_Destination) {
				// Here your code to remove the destination ref
				// and update your internal "clients"
				qDebug("kMIDIMsgObjectRemoved destination '%s'",fullName);			}
			break;
		}
		case kMIDIMsgPropertyChanged:
			// Currently ignored
			qDebug("kMIDIMsgPropertyChanged");
			break;
		case kMIDIMsgThruConnectionsChanged:
			// Currently ignored
			qDebug("kMIDIMsgThruConnectionsChanged");
			break;
		case kMIDIMsgSerialPortOwnerChanged:
			// Currently ignored
			qDebug("kMIDIMsgSerialPortOwnerChanged");
			break;
		default:
			qDebug("unhandled message type");
			break;
	}
}



char * MidiApple::getFullName(MIDIEndpointRef &endpoint_ref)
{
	MIDIEntityRef entity = 0;
	MIDIEndpointGetEntity(endpoint_ref, &entity); //get the entity
	MIDIDeviceRef device = 0;
	MIDIEntityGetDevice(entity, &device);
	char * deviceName = getName(device);
	char * endPointName = getName(endpoint_ref);
	qDebug("device name='%s' endpoint name='%s'",deviceName,endPointName);
	size_t deviceNameLen = deviceName == nullptr ? 0 : strlen(deviceName);
	size_t endPointNameLen = endPointName == nullptr ? 0 : strlen(endPointName);
	char * fullName = (char *)malloc(deviceNameLen + endPointNameLen + 2);
	std::snprintf(fullName, deviceNameLen + endPointNameLen + 2, "%s:%s", deviceName,endPointName);
	if (deviceName != nullptr) { free(deviceName); }
	if (endPointName != nullptr) { free(endPointName); }
	return fullName;
}


} // namespace lmms

#endif // LMMS_BUILD_APPLE
