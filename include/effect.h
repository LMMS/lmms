/*
 * effect.h - base class for effects
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _EFFECT_H
#define _EFFECT_H

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef QT4

#include <QtCore/QMutex>

#else

#include<qmutex.h>

#endif


#include "qt3support.h"

#include "engine.h"
#include "mixer.h"


class effect: public engineObject
{
public:
	effect( engine * _engine );
	virtual ~effect();
	
	virtual bool FASTCALL processAudioBuffer( 
			surroundSampleFrame * _buf, const fpab_t _frames );
	
	inline ch_cnt_t getProcessorCount( void )
	{
		return( m_processors );
	}
	
	inline void setProcessorCount( ch_cnt_t _processors )
	{
		m_processors = _processors;
	}
	
	inline bool isOkay( void )
	{
		return( m_okay );
	}
	
	inline void setOkay( bool _state )
	{
		m_okay = _state;
	}
	
	
	inline bool isRunning( void )
	{
		return( m_running );
	}
	
	inline void startRunning( void ) 
	{ 
		m_bufferCount = 0;
		m_running = TRUE; 
	}
	
	inline void stopRunning( void )
	{
		m_running = FALSE;
	}
	
	inline bool isBypassed( void )
	{
		return( m_bypass );
	}
	
	inline void setBypass( bool _mode )
	{
		m_bypass = _mode;
	}
	
	inline Uint32 getTimeout( void )
	{
		return( m_silenceTimeout );
	}
	
	inline void setTimeout( Uint32 _time_out )
	{
		m_silenceTimeout = _time_out;
	}
	
	inline float getWetLevel( void )
	{
		return( m_wetDry );
	}
	
	inline float getDryLevel( void )
	{
		return( 1.0f - m_wetDry );
	}
	
	inline void setWetLevel( float _wet )
	{
		m_wetDry = _wet;
	}
	
	inline const QString & getName( void )
	{
		return( m_name );
	}
	
	inline void setName( QString _name )
	{
		m_name = _name;
	}
	
	inline float getGate( void )
	{
		return( m_gate );
	}
	
	void FASTCALL setGate( float _level );
	
	inline Uint32 getBufferCount( void )
	{
		return( m_bufferCount );
	}
	
	inline void resetBufferCount( void )
	{
		m_bufferCount = 0;
	}
	
	inline void incrementBufferCount( void )
	{
		m_bufferCount++;
	}
	
	inline bool tryLock( void )
	{
		return( m_processLock.tryLock() );
	}
	
	inline void lock( void )
	{
		m_processLock.lock();
	}
	
	inline void unlock( void )
	{
		m_processLock.unlock();
	}
	
	inline bool dontRun( void )
	{
		return( m_noRun );
	}
	
	inline void setDontRun( bool _state )
	{
		m_noRun = _state;
	}
	
private:
	QString m_name;
	ch_cnt_t m_processors;
	
	bool m_okay;
	bool m_noRun;
	bool m_running;
	bool m_bypass;
	
	Uint32 m_bufferCount;
	Uint32 m_silenceTimeout;
	
	float m_wetDry;
	float m_gate;
	QMutex m_processLock;
};

#endif

#endif
