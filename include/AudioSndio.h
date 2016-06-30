#ifndef _AUDIO_SNDIO_H
#define _AUDIO_SNDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SNDIO

#include <QThread>
#include <sndio.h>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

class LcdSpinBox;
class QLineEdit;


class AudioSndio : public AudioDevice, public QThread
{
public:
	AudioSndio( bool & _success_ful, Mixer * _mixer );
	virtual ~AudioSndio();

	inline static QString name( void )
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "sndio" );
	}

	class setupWidget : public AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;
		LcdSpinBox * m_channels;
	} ;

private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );
	virtual void applyQualitySettings( void );
	virtual void run( void );

	struct sio_hdl *m_hdl;
	struct sio_par m_par;
} ;


#endif	/* LMMS_HAVE_SNDIO */

#endif	/* _AUDIO_SNDIO_H */
