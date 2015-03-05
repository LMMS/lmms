/*
 * MidiJack.h - Jack MIDI client
 *
 * by Shane Ambler
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * Based on jack midi code used by hydrogen drum machine
 * and jack example clients.
 *
 */

#ifndef MIDIJACK_H
#define MIDIJACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>

#include <QtCore/QThread>
#include <QMutex>
#include <QtCore/QFile>

#include "MidiClient.h"

#define	JACK_MIDI_BUFFER_MAX 64 /* events */

class QLineEdit;

class MidiJack : public MidiClientRaw, public QThread
{
public:
	MidiJack();
	virtual ~MidiJack();

	static QString probeDevice();

	inline static QString name()
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
			"Jack-MIDI" ) );
	}

	void JackMidiWrite(jack_nframes_t nframes);
	void JackMidiRead(jack_nframes_t nframes);


	class setupWidget : public MidiClientRaw::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_device;

	} ;


protected:
	virtual void sendByte( const unsigned char c );
	virtual void run();


private:
	jack_client_t * m_jack_client;
	jack_port_t *m_input_port;
	jack_port_t *m_output_port;
	uint8_t m_jack_buffer[JACK_MIDI_BUFFER_MAX * 4];

	void JackMidiOutEvent(uint8_t *buf, uint8_t len);
	void lock();
	void unlock();

	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );

	volatile bool m_quit;

};

#endif // LMMS_HAVE_JACK

#endif // MIDIJACK_H
