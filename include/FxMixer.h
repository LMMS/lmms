/*
 * FxMixer.h - effect-mixer for LMMS
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef FX_MIXER_H
#define FX_MIXER_H

#include "Model.h"
#include "Mixer.h"
#include "EffectChain.h"
#include "JournallingObject.h"
#include "ThreadableJob.h"


class FxRoute;
typedef QVector<FxRoute *> FxRouteVector;

class FxChannel : public ThreadableJob
{
	public:
		FxChannel( int idx, Model * _parent );
		virtual ~FxChannel();

		EffectChain m_fxChain;

		// set to true when input fed from mixToChannel or child channel
		bool m_hasInput;
		// set to true if any effect in the channel is enabled and running
		bool m_stillRunning;

		float m_peakLeft;
		float m_peakRight;
		sampleFrame * m_buffer;
		BoolModel m_muteModel;
		FloatModel m_volumeModel;
		QString m_name;
		QMutex m_lock;
		int m_channelIndex; // what channel index are we
		bool m_queued; // are we queued up for rendering yet?
		bool m_muted; // are we muted? updated per period so we don't have to call m_muteModel.value() twice

		// pointers to other channels that this one sends to
		FxRouteVector m_sends;

		// pointers to other channels that send to this one
		FxRouteVector m_receives;

		virtual bool requiresProcessing() const { return true; }
		
		QAtomicInt m_dependenciesMet;
		void incrementDeps();
		void processed();
		
	private:
		virtual void doProcessing( sampleFrame * _working_buffer );
};


class FxRoute : public QObject
{
	Q_OBJECT
	public:		
		FxRoute( FxChannel * from, FxChannel * to, float amount );
		virtual ~FxRoute();
		
	fx_ch_t senderIndex() const
	{
		return m_from->m_channelIndex;
	}
	
	fx_ch_t receiverIndex() const
	{
		return m_to->m_channelIndex;
	}
	
	FloatModel * amount()
	{
		return &m_amount;
	}
	
	FxChannel * sender() const
	{
		return m_from;
	}
	
	FxChannel * receiver() const
	{
		return m_to;
	}
	
	void updateName();
		
	private:
		FxChannel * m_from;
		FxChannel * m_to;
		FloatModel m_amount;
};


class EXPORT FxMixer : public Model, public JournallingObject
{
	Q_OBJECT
public:
	FxMixer();
	virtual ~FxMixer();

	void mixToChannel( const sampleFrame * _buf, fx_ch_t _ch );

	void prepareMasterMix();
	void masterMix( sampleFrame * _buf );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const
	{
		return "fxmixer";
	}

	FxChannel * effectChannel( int _ch )
	{
		return m_fxChannels[_ch];
	}

	// make the output of channel fromChannel go to the input of channel toChannel
	// it is safe to call even if the send already exists
	FxRoute * createChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel,
						   float amount = 1.0f);
	FxRoute * createRoute( FxChannel * from, FxChannel * to, float amount );

	// delete the connection made by createChannelSend
	void deleteChannelSend(fx_ch_t fromChannel, fx_ch_t toChannel);
	void deleteChannelSend( FxRoute * route );

	// determine if adding a send from sendFrom to
	// sendTo would result in an infinite mixer loop.
	bool isInfiniteLoop(fx_ch_t fromChannel, fx_ch_t toChannel);
	bool checkInfiniteLoop( FxChannel * from, FxChannel * to );

	// return the FloatModel of fromChannel sending its output to the input of
	// toChannel. NULL if there is no send.
	FloatModel * channelSendModel(fx_ch_t fromChannel, fx_ch_t toChannel);

	// add a new channel to the Fx Mixer.
	// returns the index of the channel that was just added
	int createChannel();

	// delete a channel from the FX mixer.
	void deleteChannel(int index);

	// delete all the mixer channels except master and remove all effects
	void clear();

	// re-arrange channels
	void moveChannelLeft(int index);
	void moveChannelRight(int index);

	// reset a channel's name, fx, sends, etc
	void clearChannel(fx_ch_t channelIndex);

	// rename channels when moving etc. if they still have their original name
	void validateChannelName( int index, int oldIndex );

	inline fx_ch_t numChannels() const
	{
		return m_fxChannels.size();
	}

	FxRouteVector m_fxRoutes;

private:
	// the fx channels in the mixer. index 0 is always master.
	QVector<FxChannel *> m_fxChannels;

	// make sure we have at least num channels
	void allocateChannelsTo(int num);
	QMutex m_sendsMutex;

	friend class MixerWorkerThread;
	friend class FxMixerView;

} ;


#endif
