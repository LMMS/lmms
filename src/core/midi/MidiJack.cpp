/*
 * MidiJack.cpp - Jack MIDI client
 *
 * by Shane Ambler
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * Based on jack midi code used by hydrogen drum machine
 * and jack example clients.
 *
 */

#include "MidiJack.h"

#ifdef LMMS_HAVE_JACK

#include <QtGui/QCompleter>
#include <QtGui/QDirModel>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QTranslator>

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "ConfigManager.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "Engine.h"
#include "MainWindow.h"

/* callback functions for jack */
static int JackMidiProcessCallback(jack_nframes_t nframes, void *arg)
{
	MidiJack *jmd = (MidiJack *)arg;

	if (nframes <= 0)
		return (0);

	jmd->JackMidiRead(nframes);
	jmd->JackMidiWrite(nframes);

	return (0);
}

static void JackMidiShutdown(void *arg)
{
	// TODO: support translations here
	const QString mess_short = "JACK server down";
	const QString mess_long = "The JACK server seems to have been shutdown.";
	QMessageBox::information( gui->mainWindow(), mess_short, mess_long );
}


MidiJack::MidiJack() :
	MidiClientRaw(),
	m_input_port( NULL ),
	m_output_port( NULL ),
	m_quit( false )
{
	m_jack_client = jack_client_open(probeDevice().toLatin1().data(),
									 JackNoStartServer, NULL);

	if(m_jack_client)
	{
		jack_set_process_callback(m_jack_client,
				JackMidiProcessCallback, this);

		jack_on_shutdown(m_jack_client,
				JackMidiShutdown, 0);

		m_output_port = jack_port_register(
				m_jack_client, "midi_TX", JACK_DEFAULT_MIDI_TYPE,
				JackPortIsOutput, 0);

		m_input_port = jack_port_register(
				m_jack_client, "midi_RX", JACK_DEFAULT_MIDI_TYPE,
				JackPortIsInput, 0);

		if(jack_activate(m_jack_client) == 0 )
		{
			// only start thread, if we are an active jack client.
			start( QThread::LowPriority );
		}
	}
}

MidiJack::~MidiJack()
{
	if(m_jack_client)
	{
		if( jack_port_unregister( m_jack_client, m_input_port) != 0){
			printf("Failed to unregister jack midi input\n");
		}

		if( jack_port_unregister( m_jack_client, m_output_port) != 0){
			printf("Failed to unregister jack midi output\n");
		}

		if( jack_deactivate(m_jack_client) != 0){
			printf("Failed to deactivate jack midi client\n");
		}

		if( jack_client_close(m_jack_client) != 0){
			printf("Failed close jack midi client\n");
		}
	}
	if( isRunning() )
	{
		m_quit = true;
		wait( 1000 );
		terminate();
	}
}

QString MidiJack::probeDevice()
{
	QString jid = ConfigManager::inst()->value( "midijack", "lmms" );
	if( jid.isEmpty() )
	{
		return "lmms";
	}
	return jid;
}

void MidiJack::sendByte( const unsigned char c )
{
	//m_midiDev.putChar( c );
}

// we read data from jack
void MidiJack::JackMidiRead(jack_nframes_t nframes)
{
	unsigned int i,b;
	void* port_buf = jack_port_get_buffer(m_input_port, nframes);
	jack_midi_event_t in_event;
	jack_nframes_t event_index = 0;
	jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

	jack_midi_event_get(&in_event, port_buf, 0);
	for(i=0; i<nframes; i++)
	{
		if((in_event.time == i) && (event_index < event_count))
		{
			// lmms is setup to parse bytes coming from a device
			// parse it byte by byte as it expects
			for(b=0;b<in_event.size;b++)
				parseData( *(in_event.buffer + b) );

			event_index++;
			if(event_index < event_count)
				jack_midi_event_get(&in_event, port_buf, event_index);
		}
	}
}

// we write data to jack
void MidiJack::JackMidiWrite(jack_nframes_t nframes)
{
	// TODO: write midi data to jack port
}

void MidiJack::run()
{
	while( m_quit == false )
	{
		// we sleep the thread to keep it alive
		// midi processing is handled by jack server callbacks
		sleep(1);
	}
}

MidiJack::setupWidget::setupWidget( QWidget * _parent ) :
	MidiClientRaw::setupWidget( MidiJack::name(), _parent )
{
	m_device = new QLineEdit( MidiJack::probeDevice(), this );
	m_device->setGeometry( 10, 20, 180, 20 );
	QDirModel * model = new QDirModel( QStringList(),
			QDir::AllDirs | QDir::System,
			QDir::Name | QDir::DirsFirst,
			this );
	m_device->setCompleter(	new QCompleter( model, this ) );


	QLabel * dev_lbl = new QLabel( tr( "CLIENT-NAME" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 180, 10 );
}

MidiJack::setupWidget::~setupWidget()
{
}

void MidiJack::setupWidget::saveSettings()
{
	ConfigManager::inst()->setValue( "midijack", "lmms",
							m_device->text() );
}

#endif // LMMS_HAVE_JACK
