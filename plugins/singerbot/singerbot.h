/*
 * singerbot.h - declaration of class singerBot, a singing bot instrument plugin
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _SINGERBOT_H
#define _SINGERBOT_H

#include <QtCore/QThread>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef USE_3RDPARTY_LIBSRC
#include <samplerate.h>
#else
#include "src/3rdparty/samplerate/samplerate.h"
#endif

#include "instrument.h"
#include "mixer.h"


class EST_Wave;
class QTextEdit;
class sampleBuffer;


class singerBot : public instrument
{
	Q_OBJECT
public:
	singerBot( instrumentTrack * _track );
	virtual ~singerBot();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _this );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;


public slots:
	void lyricsChanged( void );


private:
	typedef struct
	{
		EST_Wave * wave;
		f_cnt_t remaining_frames;
		float frequency;
		float duration;
		const char * text;
		SRC_STATE * resampling_state;
		SRC_DATA resampling_data;
	} handle_data;


	class synThread : public QThread
	{
	public:
		synThread( void );
		virtual ~synThread();

		void set_data( handle_data * _hdata )
		{
			m_data = _hdata;
		}

		void unlock_synth( void )
		{
			m_synth_semaphore.release();
		}
		void lock_handle( void )
		{
			m_handle_semaphore.acquire();
		}


	protected:
		virtual void run( void );


	private:
		QSemaphore m_handle_semaphore;
		QSemaphore m_synth_semaphore;

		handle_data * m_data;

		void text_to_wave( void );
		EST_Wave * get_wave( const char * _name );

	} ;


	static synThread * s_thread;

	QTextEdit * m_lyrics;
	QStringList m_words;
	bool m_words_dirty;

	void createWave( notePlayHandle * _n );
	void play( sampleFrame * _ab, handle_data * _hdata,
							const fpp_t _frames );
	void updateWords( void );

} ;




#endif
