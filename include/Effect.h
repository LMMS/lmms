/*
 * Effect.h - base class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef EFFECT_H
#define EFFECT_H

#include "Plugin.h"
#include "engine.h"
#include "Mixer.h"
#include "AutomatableModel.h"
#include "TempoSyncKnobModel.h"


class EffectChain;
class EffectControls;


class EXPORT Effect : public Plugin
{
	Q_OBJECT
public:
	Effect( const Plugin::Descriptor * _desc,
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key );
	virtual ~Effect();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "effect";
	}

	
	virtual bool processAudioBuffer( sampleFrame * _buf,
						const fpp_t _frames ) = 0;

	inline ch_cnt_t processorCount() const
	{
		return m_processors;
	}

	inline void setProcessorCount( ch_cnt_t _processors )
	{
		m_processors = _processors;
	}

	inline bool isOkay() const
	{
		return m_okay;
	}

	inline void setOkay( bool _state )
	{
		m_okay = _state;
	}


	inline bool isRunning() const
	{
		return m_running;
	}

	inline void startRunning() 
	{ 
		m_bufferCount = 0;
		m_running = true; 
	}

	inline void stopRunning()
	{
		m_running = false;
	}

	inline bool isEnabled() const
	{
		return m_enabledModel.value();
	}

	inline f_cnt_t timeout() const
	{
		const float samples = engine::mixer()->processingSampleRate() * m_autoQuitModel.value() / 1000.0f;
		return 1 + ( static_cast<int>( samples ) / engine::mixer()->framesPerPeriod() );
	}

	inline float wetLevel() const
	{
		return m_wetDryModel.value();
	}

	inline float dryLevel() const
	{
		return 1.0f - m_wetDryModel.value();
	}

	inline float gate() const
	{
		const float level = m_gateModel.value();
		return level*level * m_processors;
	}

	inline f_cnt_t bufferCount() const
	{
		return m_bufferCount;
	}

	inline void resetBufferCount()
	{
		m_bufferCount = 0;
	}

	inline void incrementBufferCount()
	{
		++m_bufferCount;
	}

	inline bool dontRun() const
	{
		return m_noRun;
	}

	inline void setDontRun( bool _state )
	{
		m_noRun = _state;
	}

	inline const Descriptor::SubPluginFeatures::Key & key() const
	{
		return m_key;
	}

	EffectChain * effectChain() const
	{
		return m_parent;
	}

	virtual EffectControls * controls() = 0;

	static Effect * instantiate( const QString & _plugin_name,
				Model * _parent,
				Descriptor::SubPluginFeatures::Key * _key );


protected:
	void checkGate( double _out_sum );

	virtual PluginView * instantiateView( QWidget * );

	// some effects might not be capable of higher sample-rates so they can
	// sample it down before processing and back after processing
	inline void sampleDown( const sampleFrame * _src_buf,
							sampleFrame * _dst_buf,
							sample_rate_t _dst_sr )
	{
		resample( 0, _src_buf,
				engine::mixer()->processingSampleRate(),
					_dst_buf, _dst_sr,
					engine::mixer()->framesPerPeriod() );
	}

	inline void sampleBack( const sampleFrame * _src_buf,
							sampleFrame * _dst_buf,
							sample_rate_t _src_sr )
	{
		resample( 1, _src_buf, _src_sr, _dst_buf,
				engine::mixer()->processingSampleRate(),
			engine::mixer()->framesPerPeriod() * _src_sr /
				engine::mixer()->processingSampleRate() );
	}
	void reinitSRC();


private:
	EffectChain * m_parent;
	void resample( int _i, const sampleFrame * _src_buf,
					sample_rate_t _src_sr,
					sampleFrame * _dst_buf, sample_rate_t _dst_sr,
					const f_cnt_t _frames );

	Descriptor::SubPluginFeatures::Key m_key;

	ch_cnt_t m_processors;

	bool m_okay;
	bool m_noRun;
	bool m_running;
	f_cnt_t m_bufferCount;

	BoolModel m_enabledModel;
	FloatModel m_wetDryModel;
	FloatModel m_gateModel;
	TempoSyncKnobModel m_autoQuitModel;

	SRC_DATA m_srcData[2];
	SRC_STATE * m_srcState[2];


	friend class EffectView;
	friend class EffectChain;

} ;


typedef Effect::Descriptor::SubPluginFeatures::Key EffectKey;
typedef Effect::Descriptor::SubPluginFeatures::KeyList EffectKeyList;


#endif
