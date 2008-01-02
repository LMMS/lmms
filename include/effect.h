/*
 * effect.h - base class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _EFFECT_H
#define _EFFECT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Qt/QtXml>

#include "plugin.h"
#include "engine.h"
#include "mixer.h"
#include "automatable_model.h"


class effectControlDialog;
class track;
class rackPlugin;


class effect : public plugin
{
public:
	effect( const plugin::descriptor * _desc,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~effect();
	
	virtual bool FASTCALL processAudioBuffer( 
			surroundSampleFrame * _buf, const fpp_t _frames );
	
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
	
	inline bool enabled( void )
	{
		return( m_enabledModel.value() );
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
		return( m_wetDryModel.value() );
	}
	
	inline float getDryLevel( void )
	{
		return( 1.0f - m_wetDryModel.value() );
	}
	
	inline float getGate( void )
	{
		const float level = m_gateModel.value();
		return( level*level * m_processors *
				engine::getMixer()->framesPerPeriod()  );
	}

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

	inline bool dontRun( void )
	{
		return( m_noRun );
	}

	inline void setDontRun( bool _state )
	{
		m_noRun = _state;
	}

	inline const descriptor::subPluginFeatures::key & getKey( void )
	{
		return( m_key );
	}

	virtual effectControlDialog * createControlDialog( track * _track ) = 0;

	static effect * instantiate( const QString & _plugin_name,
				descriptor::subPluginFeatures::key * _key );


private:
	descriptor::subPluginFeatures::key m_key;

	ch_cnt_t m_processors;

	bool m_okay;
	bool m_noRun;
	bool m_running;
	boolModel m_enabledModel;

	Uint32 m_bufferCount;
	Uint32 m_silenceTimeout;

	floatModel m_wetDryModel;
	floatModel m_gateModel;


	friend class rackPlugin;

} ;


typedef effect::descriptor::subPluginFeatures::key effectKey;
typedef effect::descriptor::subPluginFeatures::keyList effectKeyList;


#endif
