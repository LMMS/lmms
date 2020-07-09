/*
 * AudioPort.h - base-class for objects providing sound at a port
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef AUDIO_PORT_H
#define AUDIO_PORT_H

#include <memory>
#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include "MemoryManager.h"
#include "PlayHandle.h"

class EffectChain;
class FloatModel;
class BoolModel;

class AudioPort : public ThreadableJob
{
	MM_OPERATORS
public:
	AudioPort( const QString & _name, bool _has_effect_chain = true,
		FloatModel * volumeModel = NULL, FloatModel * panningModel = NULL,
		BoolModel * mutedModel = NULL );
	virtual ~AudioPort();

	inline sampleFrame * buffer()
	{
		return m_portBuffer;
	}

	inline void lockBuffer()
	{
		m_portBufferLock.lock();
	}

	inline void unlockBuffer()
	{
		m_portBufferLock.unlock();
	}


	// indicate whether JACK & Co should provide output-buffer at ext. port
	inline bool extOutputEnabled() const
	{
		return m_extOutputEnabled;
	}

	void setExtOutputEnabled( bool _enabled );


	// next effect-channel after this audio-port
	// (-1 = none  0 = master)
	inline fx_ch_t nextFxChannel() const
	{
		return m_nextFxChannel;
	}

	inline EffectChain * effects()
	{
		return m_effects.get();
	}

	void setNextFxChannel( const fx_ch_t _chnl )
	{
		m_nextFxChannel = _chnl;
	}


	const QString & name() const
	{
		return m_name;
	}

	void setName( const QString & _new_name );


	bool processEffects();

	// ThreadableJob stuff
	void doProcessing() override;
	bool requiresProcessing() const override
	{
		return true;
	}

	void addPlayHandle( PlayHandle * handle );
	void removePlayHandle( PlayHandle * handle );

private:
	volatile bool m_bufferUsage;

	sampleFrame * m_portBuffer;
	QMutex m_portBufferLock;

	bool m_extOutputEnabled;
	fx_ch_t m_nextFxChannel;

	QString m_name;

	std::unique_ptr<EffectChain> m_effects;

	PlayHandleList m_playHandles;
	QMutex m_playHandleLock;

	FloatModel * m_volumeModel;
	FloatModel * m_panningModel;
	BoolModel * m_mutedModel;

	friend class Mixer;
	friend class MixerWorkerThread;

} ;


#endif
