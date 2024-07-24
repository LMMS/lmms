/*
 * Mixer.h - effect-mixer for LMMS
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_MIXER_H
#define LMMS_MIXER_H

#include "Model.h"
#include "EffectChain.h"
#include "JournallingObject.h"
#include "ThreadableJob.h"

#include <atomic>
#include <optional>
#include <QColor>

namespace lmms
{


class MixerRoute;
using MixerRouteVector = std::vector<MixerRoute*>;

class MixerChannel : public ThreadableJob
{
	public:
		MixerChannel( int idx, Model * _parent );
		virtual ~MixerChannel();

		EffectChain m_fxChain;

		// set to true when input fed from mixToChannel or child channel
		bool m_hasInput;
		// set to true if any effect in the channel is enabled and running
		bool m_stillRunning;

		float m_peakLeft;
		float m_peakRight;
		SampleFrame* m_buffer;
		bool m_muteBeforeSolo;
		BoolModel m_muteModel;
		BoolModel m_soloModel;
		FloatModel m_volumeModel;
		QString m_name;
		QMutex m_lock;
		int m_channelIndex; // what channel index are we
		bool m_queued; // are we queued up for rendering yet?
		bool m_muted; // are we muted? updated per period so we don't have to call m_muteModel.value() twice

		// pointers to other channels that this one sends to
		MixerRouteVector m_sends;

		// pointers to other channels that send to this one
		MixerRouteVector m_receives;

		bool requiresProcessing() const override { return true; }
		void unmuteForSolo();

		auto color() const -> const std::optional<QColor>& { return m_color; }
		void setColor(const std::optional<QColor>& color) { m_color = color; }

		std::atomic_size_t m_dependenciesMet;
		void incrementDeps();
		void processed();
		
	private:
		void doProcessing() override;

		std::optional<QColor> m_color;
};

class MixerRoute : public QObject
{
	Q_OBJECT
public:
	MixerRoute( MixerChannel * from, MixerChannel * to, float amount );
	~MixerRoute() override = default;

	mix_ch_t senderIndex() const
	{
		return m_from->m_channelIndex;
	}

	mix_ch_t receiverIndex() const
	{
		return m_to->m_channelIndex;
	}

	FloatModel * amount()
	{
		return &m_amount;
	}

	MixerChannel * sender() const
	{
		return m_from;
	}

	MixerChannel * receiver() const
	{
		return m_to;
	}

	void updateName();

	private:
		MixerChannel * m_from;
		MixerChannel * m_to;
		FloatModel m_amount;
};


class LMMS_EXPORT Mixer : public Model, public JournallingObject
{
	Q_OBJECT
public:
	Mixer();
	~Mixer() override;

	void mixToChannel( const SampleFrame* _buf, mix_ch_t _ch );

	void prepareMasterMix();
	void masterMix( SampleFrame* _buf );

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override
	{
		return "mixer";
	}

	MixerChannel * mixerChannel( int _ch )
	{
		return m_mixerChannels[_ch];
	}

	// make the output of channel fromChannel go to the input of channel toChannel
	// it is safe to call even if the send already exists
	MixerRoute * createChannelSend(mix_ch_t fromChannel, mix_ch_t toChannel,
						   float amount = 1.0f);
	MixerRoute * createRoute( MixerChannel * from, MixerChannel * to, float amount );

	// delete the connection made by createChannelSend
	void deleteChannelSend(mix_ch_t fromChannel, mix_ch_t toChannel);
	void deleteChannelSend( MixerRoute * route );

	// determine if adding a send from sendFrom to
	// sendTo would result in an infinite mixer loop.
	bool isInfiniteLoop(mix_ch_t fromChannel, mix_ch_t toChannel);
	bool checkInfiniteLoop( MixerChannel * from, MixerChannel * to );

	// return the FloatModel of fromChannel sending its output to the input of
	// toChannel. NULL if there is no send.
	FloatModel * channelSendModel(mix_ch_t fromChannel, mix_ch_t toChannel);

	// add a new channel to the mixer.
	// returns the index of the channel that was just added
	int createChannel();

	// delete a channel from the mixer.
	void deleteChannel(int index);

	// delete all the mixer channels except master and remove all effects
	void clear();

	// re-arrange channels
	void moveChannelLeft(int index);
	void moveChannelRight(int index);

	// reset a channel's name, fx, sends, etc
	void clearChannel(mix_ch_t channelIndex);

	// rename channels when moving etc. if they still have their original name
	void validateChannelName( int index, int oldIndex );

	// check if the index channel receives audio from any other channel
	// or from any instrument or sample track
	bool isChannelInUse(int index);

	void toggledSolo();
	void activateSolo();
	void deactivateSolo();

	inline mix_ch_t numChannels() const
	{
		return m_mixerChannels.size();
	}

	MixerRouteVector m_mixerRoutes;

private:
	// the mixer channels in the mixer. index 0 is always master.
	std::vector<MixerChannel*> m_mixerChannels;

	// make sure we have at least num channels
	void allocateChannelsTo(int num);

	int m_lastSoloed;
} ;


} // namespace lmms

#endif // LMMS_MIXER_H
