/*
 * effect.h - base class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <Qt/QtXml>

#include "plugin.h"
#include "engine.h"
#include "mixer.h"
#include "automatable_model.h"
#include "tempo_sync_knob.h"


class effectChain;
class effectControls;
class track;


class EXPORT effect : public plugin
{
public:
	effect( const plugin::descriptor * _desc,
			model * _parent,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~effect();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "effect" );
	}

	
	virtual bool processAudioBuffer( sampleFrame * _buf,
						const fpp_t _frames ) = 0;

	inline ch_cnt_t getProcessorCount( void ) const
	{
		return( m_processors );
	}

	inline void setProcessorCount( ch_cnt_t _processors )
	{
		m_processors = _processors;
	}

	inline bool isOkay( void ) const
	{
		return( m_okay );
	}

	inline void setOkay( bool _state )
	{
		m_okay = _state;
	}


	inline bool isRunning( void ) const
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

	inline bool isEnabled( void ) const
	{
		return( m_enabledModel.value() );
	}

	inline f_cnt_t getTimeout( void ) const
	{
		const float samples =
				engine::getMixer()->processingSampleRate() *
					m_autoQuitModel.value() / 1000.0f;
		return( 1 + ( static_cast<Uint32>( samples ) / 
				engine::getMixer()->framesPerPeriod() ) );
	}

	inline float getWetLevel( void ) const
	{
		return( m_wetDryModel.value() );
	}

	inline float getDryLevel( void ) const
	{
		return( 1.0f - m_wetDryModel.value() );
	}

	inline float getGate( void ) const
	{
		const float level = m_gateModel.value();
		return( level*level * m_processors *
				engine::getMixer()->framesPerPeriod()  );
	}

	inline f_cnt_t getBufferCount( void ) const
	{
		return( m_bufferCount );
	}

	inline void resetBufferCount( void )
	{
		m_bufferCount = 0;
	}

	inline void incrementBufferCount( void )
	{
		++m_bufferCount;
	}

	inline bool dontRun( void ) const
	{
		return( m_noRun );
	}

	inline void setDontRun( bool _state )
	{
		m_noRun = _state;
	}

	inline const descriptor::subPluginFeatures::key & getKey( void ) const
	{
		return( m_key );
	}

	virtual effectControls * getControls( void ) = 0;

	static effect * instantiate( const QString & _plugin_name,
				model * _parent,
				descriptor::subPluginFeatures::key * _key );


protected:
	void checkGate( double _out_sum );

	virtual pluginView * instantiateView( QWidget * );

	// some effects might not be capable of higher sample-rates so they can
	// sample it down before processing and back after processing
	inline void sampleDown( const sampleFrame * _src_buf,
					sampleFrame * _dst_buf,
					sample_rate_t _dst_sr )
	{
		resample( 0, _src_buf,
				engine::getMixer()->processingSampleRate(),
					_dst_buf, _dst_sr,
					engine::getMixer()->framesPerPeriod() );
	}

	inline void sampleBack( const sampleFrame * _src_buf,
					sampleFrame * _dst_buf,
					sample_rate_t _src_sr )
	{
		resample( 1, _src_buf, _src_sr, _dst_buf,
				engine::getMixer()->processingSampleRate(),
			engine::getMixer()->framesPerPeriod() * _src_sr /
				engine::getMixer()->processingSampleRate() );
	}
	void reinitSRC( void );


private:
	void resample( int _i, const sampleFrame * _src_buf,
						sample_rate_t _src_sr,
			sampleFrame * _dst_buf, sample_rate_t _dst_sr,
					const fpp_t _frames );

	descriptor::subPluginFeatures::key m_key;

	ch_cnt_t m_processors;

	bool m_okay;
	bool m_noRun;
	bool m_running;
	f_cnt_t m_bufferCount;

	boolModel m_enabledModel;
	floatModel m_wetDryModel;
	floatModel m_gateModel;
	tempoSyncKnobModel m_autoQuitModel;

	SRC_DATA m_srcData[2];
	SRC_STATE * m_srcState[2];


	friend class effectView;
	friend class effectChain;

} ;


typedef effect::descriptor::subPluginFeatures::key effectKey;
typedef effect::descriptor::subPluginFeatures::keyList effectKeyList;


#endif
