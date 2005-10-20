/*
 * mixer.h - audio-device-independent mixer for LMMS
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _MIXER_H
#define _MIXER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QThread>
#include <QMutex>
#include <QVector>

#else

#include <qobject.h>
#include <qthread.h>
#include <qmutex.h>
#include <qvaluevector.h>

#endif


#include "types.h"
#include "volume.h"
#include "panning.h"
#include "note.h"
#include "play_handle.h"


class audioDevice;
class midiDevice;
class lmmsMainWin;
class plugin;


const int DEFAULT_BUFFER_SIZE = 512;

const Uint16 MAX_SAMPLE_PACKETS = 256;	// how many parallel audio-samples-
					// buffers shall be maximal exist and
					// mixed together?

const Uint8  DEFAULT_CHANNELS = 2;

const Uint8  SURROUND_CHANNELS =
#ifndef DISABLE_SURROUND
				4;
#else
				2;
#endif

const Uint8  QUALITY_LEVELS = 2;
const Uint32 DEFAULT_QUALITY_LEVEL = 0;
const Uint32 HIGH_QUALITY_LEVEL = DEFAULT_QUALITY_LEVEL+1;
extern Uint32 SAMPLE_RATES[QUALITY_LEVELS];
const Uint32 DEFAULT_SAMPLE_RATE = 44100;//SAMPLE_RATES[DEFAULT_QUALITY_LEVEL];


typedef sampleType sampleFrame[DEFAULT_CHANNELS];
typedef sampleType surroundSampleFrame[SURROUND_CHANNELS];

typedef struct
{
	float vol[SURROUND_CHANNELS];
} volumeVector;


const Uint32 BYTES_PER_SAMPLE = sizeof( sampleType );
const Uint32 BYTES_PER_FRAME = sizeof( sampleFrame );
const Uint32 BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );
const Uint32 BYTES_PER_OUTPUT_SAMPLE = sizeof( outputSampleType );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;


const float BASE_FREQ = 440.0f;
const tones BASE_TONE = A;
const octaves BASE_OCTAVE = OCTAVE_4;



class mixer : 
#ifndef QT4
		public QObject,
#endif
		public QThread
{
	Q_OBJECT
public:
	static inline mixer * inst( void )
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new mixer();
		}
		return( s_instanceOfMe );
	}


	void FASTCALL addBuffer( sampleFrame * _buf, Uint32 _frames,
						Uint32 _framesAhead,
						volumeVector & _volumeVector );
	inline Uint32 framesPerAudioBuffer( void ) const
	{
		return( m_framesPerAudioBuffer );
	}


	inline bool highQuality( void ) const
	{
		return( m_qualityLevel > DEFAULT_QUALITY_LEVEL );
	}


	inline const surroundSampleFrame * currentAudioBuffer( void ) const
	{
		return( m_curBuf );
	}

	// audio-device-stuff
	inline const QString & audioDevName( void ) const
	{
		return( m_audioDevName );
	}

	void FASTCALL setAudioDevice( audioDevice * _dev, bool _hq );
	void restoreAudioDevice( void );


	// MIDI-device-stuff
	inline const QString & midiDevName( void ) const
	{
		return( m_midiDevName );
	}

	inline midiDevice * getMIDIDevice( void )
	{
		return( m_midiDev );
	}


	inline void addPlayHandle( playHandle * _ph )
	{
		m_playHandles.push_back( _ph );
	}

	inline void removePlayHandle( playHandle * _ph )
	{
		m_playHandlesToRemove.push_back( _ph );
	}

	void checkValidityOfPlayHandles( void );



	inline int sampleRate( void )
	{
		return( SAMPLE_RATES[m_qualityLevel] );
	}


	inline float masterOutput( void ) const
	{
		return( m_masterOutput );
	}

	inline void setMasterOutput( float _mo )
	{
		m_masterOutput = _mo;
	}


	static inline sampleType clip( sampleType _s )
	{
		if( _s > 1.0f )
		{
			return( 1.0f );
		}
		else if( _s < -1.0f )
		{
			return( -1.0f );
		}
		return( _s );
	}


	void pause( void )
	{
		m_safetySyncMutex.lock();
	}

	void play( void )
	{
		m_safetySyncMutex.unlock();
	}


	void clear( void );


	void FASTCALL clearAudioBuffer( sampleFrame * _ab, Uint32 _frames );
#ifndef DISABLE_SURROUND
	void FASTCALL clearAudioBuffer( surroundSampleFrame * _ab,
							Uint32 _frames );
#endif

	inline bool haveNoRunningNotes( void ) const
	{
		return( m_playHandles.size() == 0 );
	}


public slots:
	void setHighQuality( bool _hq_on = FALSE );


signals:
	void sampleRateChanged( void );
	void nextAudioBuffer( const surroundSampleFrame *, Uint32 _frames );


private:
	struct samplePacket
	{
		surroundSampleFrame * m_buffer;	// actual buffer for
						// wave-data
		Uint32 m_frames;
		Uint32 m_framesDone;
		Uint32 m_framesAhead;		// number of frames, the buffer 
						// should be mixed ahead
		volume m_vol;
		panning m_pan;
		enum samplePacketStates
		{
			READY, FILLING, UNUSED
		} m_state;
	} ;


	static mixer * s_instanceOfMe;

	mixer();
	~mixer();

	void quitThread( void );


	// we don't allow to create mixer by using copy-ctor
	mixer( const mixer & ) :
#ifndef QT4
		QObject(),
#endif
		QThread(),
		m_curBuf( m_buffer1 ),
		m_nextBuf( m_buffer2 )
	{
	}

	virtual void run( void );


	void FASTCALL mixSamplePacket( samplePacket * _sp );


	audioDevice * tryAudioDevices( void );
	midiDevice * tryMIDIDevices( void );



	sampleFrame * m_silence;
#ifndef DISABLE_SURROUND
	surroundSampleFrame * m_surroundSilence;// cool, silence in surround ;-)
#endif


	samplePacket m_samplePackets[MAX_SAMPLE_PACKETS];

	Uint32 m_framesPerAudioBuffer;

	surroundSampleFrame * m_buffer1;
	surroundSampleFrame * m_buffer2;

	surroundSampleFrame * m_curBuf;
	surroundSampleFrame * m_nextBuf;

	bool m_discardCurBuf;


	playHandleVector m_playHandles;
	playHandleVector m_playHandlesToRemove;

	Uint8 m_qualityLevel;
	volatile float m_masterOutput;

	volatile bool m_quit;


	audioDevice * m_audioDev;
	audioDevice * m_oldAudioDev;
	QString m_audioDevName;


	midiDevice * m_midiDev;
	QString m_midiDevName;


	QMutex m_safetySyncMutex;
	QMutex m_devMutex;


	friend class lmmsMainWin;

} ;


#endif
