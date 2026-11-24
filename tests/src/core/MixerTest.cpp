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

#include <QSignalSpy>
#include <QtTest>

#include "Engine.h"
#include "Mixer.h"
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
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
