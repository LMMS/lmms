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

#include<qmutex.h>

#include "qt3support.h"

#include "engine.h"
#include "ladspa_2_lmms.h"
#include "mixer.h"
#include "ladspa_control.h"


typedef enum bufferRates
{
	CHANNEL_IN,
	CHANNEL_OUT,
	AUDIO_RATE_INPUT,
	AUDIO_RATE_OUTPUT,
	CONTROL_RATE_INPUT,
	CONTROL_RATE_OUTPUT
} buffer_rate_t;

typedef enum bufferData
{
	TOGGLED,
	INTEGER,
	FLOAT,
	NONE
} buffer_data_t;

typedef struct portDescription
{
	QString name;
	ch_cnt_t proc;
	Uint16 port_id;
	Uint16 control_id;
	buffer_rate_t rate;
	buffer_data_t data_type;
	LADSPA_Data max;
	LADSPA_Data min;
	LADSPA_Data def;
	LADSPA_Data value;
	LADSPA_Data * buffer;
	ladspaControl * control;
} port_desc_t;

typedef vvector<port_desc_t *> multi_proc_t;

class effect: public engineObject
{
public:
	effect( const ladspa_key_t & _key, engine * _engine );
	~effect();
	
	bool FASTCALL processAudioBuffer( surroundSampleFrame * _buf, const fpab_t _frames );
	void FASTCALL setControl( Uint16 _control, LADSPA_Data _data );
	
	inline const multi_proc_t & getControls( void )
	{
		return( m_controls );
	}
	
	inline ch_cnt_t getProcessorCount( void )
	{
		return( m_processors );
	}
	
	inline void setRunning( void ) 
	{ 
		m_bufferCount = 0;
		m_running = TRUE; 
	};
	
	inline void setBypass( bool _mode )
	{
		m_bypass = _mode;
	}
	
	inline bool isRunning( void )
	{
		return( m_running );
	}
	
	inline void setTimeout( Uint32 _time_out )
	{
		m_silenceTimeout = _time_out;
	}
	
	inline void setWetDry( float _wet )
	{
		m_wetDry = _wet;
	}
	
	inline const QString & getName( void )
	{
		return( m_name );
	}
	
	void FASTCALL setGate( float _level );
	
private:
	ladspa_key_t m_key;
	ladspa2LMMS * m_ladspa;
	QString m_name;
	
	ch_cnt_t m_processors;
	Uint16 m_effectChannels;
	Uint16 m_portCount;
	fpab_t m_bufferSize;
				
	const LADSPA_Descriptor * m_descriptor;
	vvector<LADSPA_Handle> m_handles;
	
	vvector<multi_proc_t> m_ports;
	multi_proc_t m_controls;
	
	effect * m_output;
	
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
