/*
 * AudioBusHandle.h - ThreadableJob between PlayHandle and MixerChannel
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2025 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_AUDIO_BUS_HANDLE_H
#define LMMS_AUDIO_BUS_HANDLE_H

#include <memory>
#include <QString>
#include <QMutex>

#include "PlayHandle.h"

namespace lmms
{

class EffectChain;
class FloatModel;
class BoolModel;

/**
	@brief Job between @ref PlayHandle and @ref MixerChannel

	A @ref ThreadableJob class located at the exit point of each @ref PlayHandle into a @ref MixerChannel
	(or into an audio device, in case of @ref AudioJack, but this is not supported in AudioBusHandle yet).
	It contains an optional @ref EffectChain which is e.g. visualized in the
	@ref InstrumentTrackWindow or @ref SampleTrackWindow.
	For processing, it adds all input play handles into an internal buffer,
	processes the @ref EffectChain (if existing) on that buffer
	and finally merges the buffer into its @ref MixerChannel.
*/
class AudioBusHandle : public ThreadableJob
{
public:
	AudioBusHandle(const QString& name, bool hasEffectChain = true,
		FloatModel* volumeModel = nullptr, FloatModel* panningModel = nullptr,
		BoolModel* mutedModel = nullptr);
	virtual ~AudioBusHandle();

	SampleFrame* buffer() { return m_buffer; }

	// indicate whether JACK & Co should provide output-buffer at ext. port
	bool extOutputEnabled() const { return m_extOutputEnabled; }
	void setExtOutputEnabled(bool enabled);

	// next mixer-channel after this audio-bus-handle
	// (-1 = none  0 = master)
	mix_ch_t nextMixerChannel() const { return m_nextMixerChannel; }
	void setNextMixerChannel(const mix_ch_t chnl) { m_nextMixerChannel = chnl; }

	const QString& name() const { return m_name; }
	void setName(const QString& newName);

	EffectChain* effects() { return m_effects.get(); }
	bool processEffects();

	// ThreadableJob stuff
	void doProcessing() override;
	bool requiresProcessing() const override { return true; }

	void addPlayHandle(PlayHandle* handle);
	void removePlayHandle(PlayHandle* handle);

private:
	volatile bool m_bufferUsage;

	SampleFrame* const m_buffer;

	bool m_extOutputEnabled;
	mix_ch_t m_nextMixerChannel;

	QString m_name;

	std::unique_ptr<EffectChain> m_effects;

	PlayHandleList m_playHandles;
	QMutex m_playHandleLock;

	FloatModel* m_volumeModel;
	FloatModel* m_panningModel;
	BoolModel* m_mutedModel;

	friend class AudioEngine;
	friend class AudioEngineWorkerThread;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUS_HANDLE_H
