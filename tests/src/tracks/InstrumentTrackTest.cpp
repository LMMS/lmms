/*
 * InstrumentTrackTest.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <satemiej@gmail.com>
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

#include "InstrumentTrack.h"
#include <QtTest/QtTest>

#include "QCoreApplication"

#include "MidiClip.h"
#include "TrackContainer.h"

#include "Engine.h"
#include "Song.h"

class InstrumentTrackTest : public QObject
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

	void testAddClip()
	{
		using namespace lmms;

		InstrumentTrack track;
		MidiClip clip;

		track.addClip(&clip);

		QCOMPARE(clip.getTrack(), &track);
		QCOMPARE(track.containsClip(&clip), true);
	}

	void testRemoveClip()
	{
		using namespace lmms;

		InstrumentTrack track;
		MidiClip clip;
		track.addClip(&clip);

		track.removeClip(&clip);

		QCOMPARE(clip.getTrack(), nullptr);
		QCOMPARE(track.containsClip(&clip), false);
	}
};

QTEST_GUILESS_MAIN(InstrumentTrackTest)
#include "InstrumentTrackTest.moc"
