/*
 * lvsl_client.h - client for LVSL Server
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _LVSL_CLIENT_H
#define _LVSL_CLIENT_H

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QString>
#include <QtCore/QMutex>

#else

#include <qstring.h>
#include <qmutex.h>

#endif


#include "mixer.h"
#include "communication.h"
#include "midi.h"
#include "engine.h"



class remoteVSTPlugin : public QObject, public engineObject
{
	Q_OBJECT
public:
	remoteVSTPlugin( const QString & _plugin, engine * _engine );
	virtual ~remoteVSTPlugin();

	void showEditor( void );
	void hideEditor( void );

	inline const QString & name( void ) const
	{
		return( m_name );
	}

	inline Sint32 version( void ) const
	{
		return( m_version );
	}
	
	inline const QString & vendorString( void ) const
	{
		return( m_vendorString );
	}

	inline const QString & productString( void ) const
	{
		return( m_productString );
	}

	// if _wait == TRUE, process() calls waitForProcessingFinished()
	// immediately, otherwise, _out_buf can be zero and you've to call
	// waitForProcessingFinished() on your own
	bool FASTCALL process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf,
					bool _wait );
	bool FASTCALL waitForProcessingFinished( sampleFrame * _out_buf );


	void FASTCALL enqueueMidiEvent( const midiEvent & _event,
						const f_cnt_t _frames_ahead );

	const QMap<QString, QString> & parameterDump( void );
	void setParameterDump( const QMap<QString, QString> & _pdump );


	inline Uint8 inputCount( void ) const
	{
		return( m_inputCount );
	}

	inline Uint8 outputCount( void ) const
	{
		return( m_outputCount );
	}

	inline QWidget * pluginWidget( void )
	{
		return( m_pluginWidget );
	}

	inline bool failed( void ) const
	{
		return( m_failed );
	}


public slots:
	void setTempo( bpm_t _bpm );


private:
	template<typename T>
	inline T readValueS( void ) const
	{
		return( ::readValue<T>( m_serverInFD ) );
	}

	template<typename T>
	inline void writeValueS( const T & _i ) const
	{
		::writeValue<T>( _i, m_serverOutFD );
	}

	inline std::string readStringS( void ) const
	{
		return( ::readString( m_serverInFD ) );
	}

	inline void writeStringS( const char * _str ) const
	{
		::writeString( _str, m_serverOutFD );
	}

	inline void lock( void )
	{
		m_serverMutex.lock();
	}

	inline void unlock( void )
	{
		m_serverMutex.unlock();
	}

	bool messagesLeft( void ) const;
	Sint16 processNextMessage( void );

	void FASTCALL setShmKeyAndSize( const Uint16 _key, const size_t _size );


	bool m_failed;
	QString m_plugin;
	QWidget * m_pluginWidget;
	Sint32 m_pluginXID;
	QSize m_pluginGeometry;

	int m_pluginPID;
	int m_pipes[2][2];
	int m_serverInFD;
	int m_serverOutFD;

	QMutex m_serverMutex;

	QString m_name;
	Sint32 m_version;
	QString m_vendorString;
	QString m_productString;

	QMap<QString, QString> m_parameterDump;

	Uint8 m_inputCount;
	Uint8 m_outputCount;

	int m_shmID;
	float * m_shm;
	size_t m_shmSize;

	bool m_initialized;

} ;


#endif
