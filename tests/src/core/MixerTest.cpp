/*
 * MixerTest.cpp
 *
 * Copyright (c) 2026 Ostap Brehin <osbre/at/protonmail.com>
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

#include <QDomDocument>
#include <QSignalSpy>
#include <QtTest>

#include "ConfigManager.h"
#include "Engine.h"
#include "Mixer.h"
#include "PatternStore.h"
#include "SampleTrack.h"
#include "Song.h"

class MixerTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		using namespace lmms;
		Engine::init(true);
	}

	void cleanupTestCase()
	{
		using namespace lmms;
		Engine::destroy();
	}

	void init()
	{
		using namespace lmms;
		Engine::getSong()->clearProject();
	}

	void testChannelSignals()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();

		auto added = QSignalSpy{mixer, &Mixer::channelAdded};
		auto removed = QSignalSpy{mixer, &Mixer::channelRemoved};
		auto swapped = QSignalSpy{mixer, &Mixer::channelsSwapped};

		const int first = mixer->createChannel();
		const int second = mixer->createChannel();
		QCOMPARE(added.count(), 2);
		QCOMPARE(added.at(1).at(0).toInt(), second);

		mixer->moveChannelLeft(second);
		QCOMPARE(swapped.count(), 1);
		QCOMPARE(swapped.at(0).at(0).toInt(), first);
		QCOMPARE(swapped.at(0).at(1).toInt(), second);

		mixer->deleteChannel(first);
		QCOMPARE(removed.count(), 1);
		QCOMPARE(removed.at(0).at(0).toInt(), first);
	}

	void testTrackChannelRangeFollowsMixer()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto track = Track::create(Track::Type::Sample, Engine::getSong());
		const auto model = track->mixerChannelModel();

		const int index = mixer->createChannel();
		QCOMPARE(model->maxValue(), index);

		model->setValue(index);

		mixer->deleteChannel(index);
		QCOMPARE(model->maxValue(), index - 1);
		QCOMPARE(model->value(), 0);
	}

	void testAutoAssignSetting()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();

		ConfigManager::inst()->setValue("app", "autoassignmixerchannel", "1");
		const auto assigned = Track::create(Track::Type::Sample, Engine::getSong());
		ConfigManager::inst()->setValue("app", "autoassignmixerchannel", "0");
		const auto unassigned = Track::create(Track::Type::Sample, Engine::getSong());

		QVERIFY(mixer->linkedChannel(*assigned));
		QCOMPARE(mixer->linkedChannel(*assigned)->m_name, assigned->name());
		QCOMPARE(unassigned->mixerChannelModel()->value(), 0);
	}

	void testLinkedChannelFollowsTrack()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto track = Track::create(Track::Type::Sample, Engine::getSong());
		const int index = mixer->createLinkedChannel(*track);
		const auto channel = mixer->mixerChannel(index);
		QCOMPARE(mixer->linkedTrack(index), track);

		track->setName("Drums");
		track->setColor(QColor{Qt::red});
		QCOMPARE(channel->m_name, QString{"Drums"});
		QCOMPARE(channel->color(), QColor{Qt::red});

		// routing the track elsewhere unlinks it
		auto routingChanged = QSignalSpy{mixer, &Mixer::trackRoutingChanged};
		track->mixerChannelModel()->setValue(0);
		track->setName("Bass");
		QCOMPARE(routingChanged.count(), 1);
		QVERIFY(!mixer->linkedChannel(*track));
		QVERIFY(!mixer->linkedTrack(index));
		QCOMPARE(channel->m_name, QString{"Drums"});
	}

	void testLinkedChannelChangesTrack()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto track = Track::create(Track::Type::Sample, Engine::getSong());
		const int linked = mixer->createLinkedChannel(*track);
		const int unlinked = mixer->createChannel();

		mixer->setChannelName(linked, "Kick");
		QCOMPARE(track->name(), QString{"Kick"});
		QCOMPARE(mixer->mixerChannel(linked)->m_name, QString{"Kick"});
		QCOMPARE(mixer->linkedTrack(linked), track);

		mixer->setChannelName(unlinked, "Bus");
		QCOMPARE(mixer->mixerChannel(unlinked)->m_name, QString{"Bus"});
		QCOMPARE(track->name(), QString{"Kick"});

		mixer->setChannelColor(linked, QColor{Qt::blue});
		QCOMPARE(track->color(), QColor{Qt::blue});
		QCOMPARE(mixer->mixerChannel(linked)->color(), QColor{Qt::blue});

		mixer->setChannelColor(linked, std::nullopt);
		QVERIFY(!track->color().has_value());
		QVERIFY(!mixer->mixerChannel(linked)->color().has_value());

		mixer->setChannelColor(unlinked, QColor{Qt::green});
		QCOMPARE(mixer->mixerChannel(unlinked)->color(), QColor{Qt::green});
		QVERIFY(!track->color().has_value());
	}

	void testLinkAndUnlinkChannel()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto track = Track::create(Track::Type::Sample, Engine::getSong());
		track->setName("Pad");
		const int index = mixer->createChannel();
		QVERIFY(!mixer->canLinkChannel(0));
		QVERIFY(!mixer->canLinkChannel(index));

		track->mixerChannelModel()->setValue(index);
		QVERIFY(mixer->canLinkChannel(index));
		mixer->linkChannel(index);
		QCOMPARE(mixer->linkedTrack(index), track);
		QCOMPARE(mixer->mixerChannel(index)->m_name, QString{"Pad"});
		QVERIFY(!mixer->canLinkChannel(index));

		mixer->unlinkChannel(index);
		QVERIFY(!mixer->linkedTrack(index));
		track->setName("Lead");
		QCOMPARE(mixer->mixerChannel(index)->m_name, QString{"Pad"});

		// a channel shared by several tracks has no single track to follow
		const auto other = Track::create(Track::Type::Sample, Engine::getSong());
		other->mixerChannelModel()->setValue(index);
		QVERIFY(!mixer->canLinkChannel(index));
	}

	void testLinkedChannelsFollowTrackOrder()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto song = Engine::getSong();

		const auto channelOrder = [mixer] {
			auto channels = std::vector<MixerChannel*>{};
			for (int i = 1; i < mixer->numChannels(); ++i) { channels.push_back(mixer->mixerChannel(i)); }
			return channels;
		};

		const auto first = Track::create(Track::Type::Sample, song);
		const auto firstChannel = mixer->mixerChannel(mixer->createLinkedChannel(*first));
		const auto second = Track::create(Track::Type::Sample, song);
		const auto secondChannel = mixer->mixerChannel(mixer->createLinkedChannel(*second));
		const auto unlinked = mixer->mixerChannel(mixer->createChannel());

		// Pattern Editor channels come first, and the unlinked channel stays after the second one
		const auto pattern = Track::create(Track::Type::Sample, Engine::patternStore());
		const auto patternChannel = mixer->mixerChannel(mixer->createLinkedChannel(*pattern));
		QVERIFY(channelOrder() == (std::vector{patternChannel, firstChannel, secondChannel, unlinked}));

		const auto& tracks = song->tracks();
		song->moveTrack(second, std::ranges::find(tracks, first) - tracks.begin());
		QVERIFY(channelOrder() == (std::vector{patternChannel, secondChannel, firstChannel, unlinked}));

		song->moveTrack(second, tracks.size() - 1);
		QVERIFY(channelOrder() == (std::vector{patternChannel, firstChannel, secondChannel, unlinked}));
	}

	void testLinkIsSaved()
	{
		using namespace lmms;
		const auto mixer = Engine::mixer();
		const auto track = Track::create(Track::Type::Sample, Engine::getSong());
		mixer->createLinkedChannel(*track);

		auto doc = QDomDocument{};
		auto parent = doc.createElement("test");
		const auto element = track->saveState(doc, parent);
		QCOMPARE(element.attribute("linkedmixerchannel"), QString{"1"});

		// the channel stays linked to the original track
		const auto clone = track->clone();
		QVERIFY(!mixer->linkedChannel(*clone));
		QVERIFY(mixer->linkedChannel(*track));

		delete track;
		const auto loaded = Track::create(element, Engine::getSong());
		QVERIFY(mixer->linkedChannel(*loaded));
	}
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
