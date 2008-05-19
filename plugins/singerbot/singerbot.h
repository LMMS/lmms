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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef USE_3RDPARTY_LIBSRC
#include <samplerate.h>
#else
#include "src/3rdparty/samplerate/samplerate.h"
#endif

#include <semaphore.h>

#include "instrument.h"
#include "instrument_view.h"
#include "mixer.h"


class File;
class QTextEdit;
class sampleBuffer;
class singerBotView;

class singerBot : public instrument
{
    Q_OBJECT
public:
	singerBot( instrumentTrack * _track );
	virtual ~singerBot();

	virtual void playNote( notePlayHandle * _n, bool _try_parallelizing,
						sampleFrame * _working_buf );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual pluginView * instantiateView( QWidget * _parent );

	void setPlainText( const QString & _plain_text, bool _emitDataChanged = TRUE );
    const QString & getPlainText();

private:
	typedef struct
	{
		short * wave;
		int num_samples;
		int sample_rate;
		f_cnt_t remaining_frames;
		float frequency;
		float duration;
		const char * text;
		SRC_STATE * resampling_state;
		SRC_DATA resampling_data;
	} handle_data;


	QString m_file_suffix;

	File * m_shm;
	sem_t * m_handle_semaphore;
	sem_t * m_synth_semaphore;

	// m_plain_text is the model, and m_words is parsed list of the words in m_plain_text
	QString m_plain_text;
	QStringList m_words;
	bool m_words_dirty;

	void createWave( notePlayHandle * _n );
	void play( sampleFrame * _ab, handle_data * _hdata,
							const fpp_t _frames );

	void synth_send( handle_data * _hdata );
	void synth_read( handle_data * _hdata );

	const char * addSuffix( const char * _s );
    void showHandleData( handle_data * _hdata );

    friend class singerBotView;

} ;




class singerBotView : public instrumentView
{
	Q_OBJECT
public:
	singerBotView( instrument * _instrument, QWidget * _parent );
	virtual ~singerBotView();

public slots:
	void viewTextChanged( void );
    void modelTextChanged( void );

private:
    QTextEdit * m_lyrics;

} ;


#endif
