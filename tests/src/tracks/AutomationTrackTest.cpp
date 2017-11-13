/*
 * AutomationTrackTest.cpp
 *
 * Copyright (c) 2017 Lukas W <lukaswhl/at/gmail.com>
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

#include "QTestSuite.h"

#include "QCoreApplication"

#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "TrackContainer.h"

#include "Engine.h"
#include "Song.h"

class AutomationTrackTest : QTestSuite
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
	}

	void testPatternLinear()
	{
		AutomationPattern p(nullptr);
		p.setProgressionType(AutomationPattern::LinearProgression);
		p.putValue(0, 0.0, false);
		p.putValue(100, 1.0, false);

		QCOMPARE(p.valueAt(0), 0.0f);
		QCOMPARE(p.valueAt(25), 0.25f);
		QCOMPARE(p.valueAt(50), 0.5f);
		QCOMPARE(p.valueAt(75), 0.75f);
		QCOMPARE(p.valueAt(100), 1.0f);
		QCOMPARE(p.valueAt(150), 1.0f);
	}

	void testPatternDiscrete()
	{
		AutomationPattern p(nullptr);
		p.setProgressionType(AutomationPattern::DiscreteProgression);
		p.putValue(0, 0.0, false);
		p.putValue(100, 1.0, false);

		QCOMPARE(p.valueAt(0), 0.0f);
		QCOMPARE(p.valueAt(50), 0.0f);
		QCOMPARE(p.valueAt(100), 1.0f);
		QCOMPARE(p.valueAt(150), 1.0f);
	}

	void testPatterns()
	{
		FloatModel model;

		auto song = Engine::getSong();
		AutomationTrack track(song);

		AutomationPattern p1(&track);
		p1.setProgressionType(AutomationPattern::LinearProgression);
		p1.putValue(0, 0.0, false);
		p1.putValue(10, 1.0, false);
		p1.movePosition(0);
		p1.addObject(&model);

		AutomationPattern p2(&track);
		p2.setProgressionType(AutomationPattern::LinearProgression);
		p2.putValue(0, 0.0, false);
		p2.putValue(100, 1.0, false);
		p2.movePosition(100);
		p2.addObject(&model);

		AutomationPattern p3(&track);
		p3.addObject(&model);
		//XXX: Why is this even necessary?
		p3.clear();

		QCOMPARE(song->automatedValuesAt(  0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(  5)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt( 10)[&model], 1.0f);
		QCOMPARE(song->automatedValuesAt( 50)[&model], 1.0f);
		QCOMPARE(song->automatedValuesAt(100)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(150)[&model], 0.5f);
	}

	void testBBTrack()
	{
		auto song = Engine::getSong();
		auto bbContainer = Engine::getBBTrackContainer();
		BBTrack bbTrack(song);
		AutomationTrack automationTrack(bbContainer);
		bbTrack.createTCOsForBB(bbTrack.index());

		QVERIFY(automationTrack.numOfTCOs());
		AutomationPattern* p1 = dynamic_cast<AutomationPattern*>(automationTrack.getTCO(0));
		QVERIFY(p1);

		FloatModel model;

		p1->setProgressionType(AutomationPattern::LinearProgression);
		p1->putValue(0, 0.0, false);
		p1->putValue(10, 1.0, false);
		p1->addObject(&model);

		QCOMPARE(bbContainer->automatedValuesAt( 0, bbTrack.index())[&model], 0.0f);
		QCOMPARE(bbContainer->automatedValuesAt( 5, bbTrack.index())[&model], 0.5f);
		QCOMPARE(bbContainer->automatedValuesAt(10, bbTrack.index())[&model], 1.0f);
		QCOMPARE(bbContainer->automatedValuesAt(50, bbTrack.index())[&model], 1.0f);

		BBTrack bbTrack2(song);
		bbTrack.createTCOsForBB(bbTrack2.index());

		QCOMPARE(bbContainer->automatedValuesAt(5, bbTrack.index())[&model], 0.5f);
		QVERIFY(! bbContainer->automatedValuesAt(5, bbTrack2.index()).size());

		BBTCO tco(&bbTrack);
		tco.changeLength(MidiTime::ticksPerTact() * 2);
		tco.movePosition(0);

		QCOMPARE(song->automatedValuesAt(0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(5)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt(MidiTime::ticksPerTact() + 5)[&model], 0.5f);
	}
} AutomationTrackTest;

#include "AutomationTrackTest.moc"
