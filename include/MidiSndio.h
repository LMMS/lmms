#ifndef _MIDI_SNDIO_H
#define _MIDI_SNDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SNDIO

#include <QtCore/QThread>
#include <QtCore/QFile>

#include <sndio.h>

#include "MidiClient.h"

class QLineEdit;


class MidiSndio : public MidiClientRaw, public QThread
{
public:
	MidiSndio( void );
	virtual ~MidiSndio();

	static QString probeDevice( void );

	inline static QString name( void )
	{
		return QT_TRANSLATE_NOOP( "MidiSetupWidget", "sndio MIDI" );
	}

	inline static QString configSection()
	{
		return "MidiSndio";
	}


protected:
	virtual void sendByte( const unsigned char c );
	virtual void run( void );

private:
	struct mio_hdl * m_hdl;
	volatile bool m_quit;
} ;

#endif	/* LMMS_HAVE_SNDIO */

#endif	/* _MIDI_SNDIO_H */
