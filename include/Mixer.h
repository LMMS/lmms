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

#ifndef MIXER_H
#define MIXER_H

#include "ConfigManager.h"
#include "Model.h"
#include "Track.h"
#include "EffectChain.h"
#include "JournallingObject.h"
#include "ThreadableJob.h"

#include <atomic>

#include <QColor>

namespace lmms
{


class MixerRoute;
using MixerRouteVector = QVector<MixerRoute*>;

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
		sampleFrame * m_buffer;
		bool m_muteBeforeSolo;
		BoolModel m_muteModel;
		BoolModel m_soloModel;
		FloatModel m_volumeModel;
		QString m_name;
		QMutex m_lock;
		int m_channelIndex; // what channel index are we
		bool m_queued; // are we queued up for rendering yet?
		bool m_muted; // are we muted? updated per period so we don't have to call m_muteModel.value() twice
		BoolModel m_autoTrackLinkModel;

		// pointers to other channels that this one sends to
		MixerRouteVector m_sends;

		// pointers to other channels that send to this one
		MixerRouteVector m_receives;

		bool requiresProcessing() const override { return true; }
		void unmuteForSolo();


		void setColor (QColor newColor)
		{
			m_color = newColor;
			m_hasColor = true;
		}

		// TODO C++17 and above: use std::optional instead
		QColor m_color;
		bool m_hasColor;

	
		std::atomic_int m_dependenciesMet;
		void incrementDeps();
		void processed();
		
	private:
		void doProcessing() override;
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
	struct autoTrackLinkSettings
	{
		// Remark: take care of the enum order as it is used for range checking during reading the values from config
		enum class LinkStyle
		{
			Disabled,		/* don't link any styles */
			LinkNameAndColor,		/* link namd and color */
			LinkColorOnly,		/* link only color */
		};

		enum class AutoAdd
		{
			Disabled,			/* do not link tracks after add */
			Separate,			/* one channel for each track after add*/
			UseFirstTrackOnly,	/* use always the first channel in the editor for new tracks*/
		};

		enum class LinkMode
		{
			OneToOne,			/* do not link song editor tracks */
			OneToMany,			/* one channel for each song editor track  */
		};

		enum class AutoSort
		{
			Disabled,			/* automatic sorting is disabled */
			LinkedPattern,		/* linked tracks first, pattern editor afterwards */
			PatternLinked 		/* pattern editor first, linked tracks afterwards */
		};


		struct editorSettings
		{
			LinkStyle linkStyle;
			AutoAdd autoAdd;

			editorSettings()
			{
				linkStyle = LinkStyle::LinkNameAndColor;
				autoAdd = AutoAdd::Separate;
			}
		};


		bool enabled;
		bool autoDelete;
		LinkMode linkMode;
		AutoSort autoSort;
		editorSettings songEditor;
		editorSettings patternEditor;

		bool getAsBoolOrDefault(const QString & text, bool defaultValue)
		{
			return (bool)getIntInRangeOrDefault(text, 0,1,defaultValue);
		}


		AutoAdd getAsAutoAddOrDefault(const QString & text, AutoAdd defaultValue)
		{
			return (AutoAdd) getIntInRangeOrDefault(text,(int)AutoAdd::Disabled, (int)AutoAdd::UseFirstTrackOnly,(int)defaultValue);
		}

		LinkStyle getAsLinkStyleOrDefault(const QString & text, LinkStyle defaultValue)
		{
			return (LinkStyle) getIntInRangeOrDefault(text,(int)LinkStyle::Disabled, (int)LinkStyle::LinkColorOnly,(int)defaultValue);
		}

		AutoSort getAsAutoSortOrDefault(const QString & text, AutoSort defaultValue)
		{
			return (AutoSort) getIntInRangeOrDefault(text,(int)AutoSort::Disabled, (int)AutoSort::PatternLinked,(int)defaultValue);
		}


		LinkMode getAsLinkModeOrDefault(const QString & text, LinkMode defaultValue)
		{
			return (LinkMode) getIntInRangeOrDefault(text,(int)LinkMode::OneToOne, (int)LinkMode::OneToMany,(int)defaultValue);
		}

		bool autoAdd()
		{
			return songEditor.autoAdd != AutoAdd::Disabled || patternEditor.autoAdd != AutoAdd::Disabled;
		}


		bool linkName(bool isPatternEditor)
		{
			return isPatternEditor ?
				patternEditor.linkStyle == LinkStyle::LinkNameAndColor :
				songEditor.linkStyle == LinkStyle::LinkNameAndColor;
		}

		bool linkColor(bool isPatternEditor)
		{
			return isPatternEditor ?
				(patternEditor.linkStyle == LinkStyle::LinkNameAndColor) || (patternEditor.linkStyle == LinkStyle::LinkColorOnly) :
				(songEditor.linkStyle == LinkStyle::LinkNameAndColor) || (songEditor.linkStyle == LinkStyle::LinkColorOnly);
		}

		bool linkAnyStyles()
		{
			return patternEditor.linkStyle != LinkStyle::Disabled ||  songEditor.linkStyle != LinkStyle::Disabled;
		}

		autoTrackLinkSettings()
		{
			enabled = false;
			autoDelete = true;
			linkMode = LinkMode::OneToOne;
			autoSort = AutoSort::Disabled;
		}

	private:
		int getIntInRangeOrDefault(const QString & text, int lower, int upper, int defaultValue)
		{
			if (text == nullptr || text.isEmpty()) return defaultValue;
			auto asInt = text.toInt();
			if (asInt < lower || asInt >upper) return defaultValue;
			return asInt;
		}

	};



	Mixer();
	~Mixer() override;

	void mixToChannel( const sampleFrame * _buf, mix_ch_t _ch );

	void prepareMasterMix();
	void masterMix( sampleFrame * _buf );

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
	void swapChannels(int indexA, int indexB);

	void toggleAutoTrackLink(int index);

	// process tracks which have a mixer channel assigned
	void processAssignedTracks(std::function<void(Track * track, IntModel * model, MixerChannel * channel)> process);
	// process tracks assigned to a specific channel
	void processChannelTracks(MixerChannel * channel, std::function<void(Track * track)> process);
	IntModel * getChannelModelByTrack(Track * track);	
	MixerChannel * getCustomChannelByTrack(Track * track);
	std::vector<int> getUsedChannelCounts();
	bool isAutoTrackLinkToggleAllowed(int index);
	//bool autoLinkTrackConfigEnabled();

	autoTrackLinkSettings getAutoLinkTrackSettings();
	void saveAutoLinkTrackSettings(autoTrackLinkSettings settings);

	// reset a channel's name, fx, sends, etc
	void clearChannel(mix_ch_t channelIndex);

	// rename channels when moving etc. if they still have their original name
	void validateChannelName( int index, int oldIndex );

	void toggledSolo();
	void activateSolo();
	void deactivateSolo();

	inline mix_ch_t numChannels() const
	{
		return m_mixerChannels.size();
	}

	MixerRouteVector m_mixerRoutes;

private:

	inline const QString & getAutoTrackCfg(ConfigManager * cfg, const QString & postFix)
	{
		return cfg->value("AutoTrackLink", "settings_"+postFix);
	}

	inline void setAutoTrackCfg(ConfigManager * cfg, const QString & postFix, int value)
	{
		cfg->setValue("AutoTrackLink", "settings_"+postFix,  QString::number(value));
	}

	// the mixer channels in the mixer. index 0 is always master.
	QVector<MixerChannel *> m_mixerChannels;

	// make sure we have at least num channels
	void allocateChannelsTo(int num);

	int m_lastSoloed;
} ;


} // namespace lmms

#endif
