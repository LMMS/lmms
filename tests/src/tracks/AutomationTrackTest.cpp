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

#include <QtTest>


#include "AutomationClip.h"
#include "AutomationTrack.h"
#include "DetuningHelper.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "PatternClip.h"
#include "PatternTrack.h"
#include "PatternStore.h"

#include "Engine.h"
#include "Song.h"

class AutomationTrackTest : public QObject
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

	void testClipLinear()
	{
		using namespace lmms;

		AutomationClip c(nullptr);
		c.setProgressionType(AutomationClip::ProgressionType::Linear);
		c.putValue(0, 0.0, false);
		c.putValue(100, 1.0, false);

		QCOMPARE(c.valueAt(0), 0.0f);
		QCOMPARE(c.valueAt(25), 0.25f);
		QCOMPARE(c.valueAt(50), 0.5f);
		QCOMPARE(c.valueAt(75), 0.75f);
		QCOMPARE(c.valueAt(100), 1.0f);
		QCOMPARE(c.valueAt(150), 1.0f);
	}

	void testClipDiscrete()
	{
		using namespace lmms;

		AutomationClip c(nullptr);
		c.setProgressionType(AutomationClip::ProgressionType::Discrete);
		c.putValue(0, 0.0, false);
		c.putValue(100, 1.0, false);

		QCOMPARE(c.valueAt(0), 0.0f);
		QCOMPARE(c.valueAt(50), 0.0f);
		QCOMPARE(c.valueAt(100), 1.0f);
		QCOMPARE(c.valueAt(150), 1.0f);
	}

	void testClips()
	{
		using namespace lmms;

		FloatModel model;

		auto song = Engine::getSong();
		AutomationTrack track(song);

		AutomationClip c1(&track);
		c1.setProgressionType(AutomationClip::ProgressionType::Linear);
		c1.putValue(0, 0.0, false);
		c1.putValue(10, 1.0, false);
		c1.movePosition(0);
		c1.addObject(&model);

		AutomationClip c2(&track);
		c2.setProgressionType(AutomationClip::ProgressionType::Linear);
		c2.putValue(0, 0.0, false);
		c2.putValue(100, 1.0, false);
		c2.movePosition(100);
		c2.addObject(&model);

		AutomationClip c3(&track);
		c3.addObject(&model);
		//XXX: Why is this even necessary?
		c3.clear();

		QCOMPARE(song->automatedValuesAt(  0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(  5)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt( 10)[&model], 1.0f);
		QCOMPARE(song->automatedValuesAt( 50)[&model], 1.0f);
		QCOMPARE(song->automatedValuesAt(100)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(150)[&model], 0.5f);
	}

	void testLengthRespected()
	{
		using namespace lmms;

		FloatModel model;

		auto song = Engine::getSong();
		AutomationTrack track(song);

		AutomationClip c(&track);
		c.setProgressionType(AutomationClip::ProgressionType::Linear);
		c.addObject(&model);

		c.putValue(0, 0.0, false);
		c.putValue(100, 1.0, false);

		c.changeLength(100);
		QCOMPARE(song->automatedValuesAt(  0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt( 50)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt(100)[&model], 1.0f);

		c.changeLength(50);
		QCOMPARE(song->automatedValuesAt(  0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt( 50)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt(100)[&model], 0.5f);
	}

	void testInlineAutomation()
	{
		using namespace lmms;

		auto song = Engine::getSong();

		InstrumentTrack instrumentTrack(song);

		MidiClip midiClip(&instrumentTrack);
		midiClip.changeLength(TimePos(4, 0));
		Note* note = midiClip.addNote(Note(TimePos(4, 0)), false);
		note->createDetuning();

		auto clip = note->detuning()->automationClip();
		clip->setProgressionType( AutomationClip::ProgressionType::Linear );
		clip->putValue(TimePos(0, 0), 0.0);
		clip->putValue(TimePos(4, 0), 1.0);

		QCOMPARE(clip->valueAt(TimePos(0, 0)), 0.0f);
		QCOMPARE(clip->valueAt(TimePos(1, 0)), 0.25f);
		QCOMPARE(clip->valueAt(TimePos(2, 0)), 0.5f);
		QCOMPARE(clip->valueAt(TimePos(4, 0)), 1.0f);
	}

	void testPatternTrack()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		auto patternStore = Engine::patternStore();
		PatternTrack patternTrack(song);
		AutomationTrack automationTrack(patternStore);
		automationTrack.createClipsForPattern(patternStore->numOfPatterns() - 1);

		QVERIFY(automationTrack.numOfClips());
		auto c1 = dynamic_cast<AutomationClip*>(automationTrack.getClip(0));
		QVERIFY(c1);

		FloatModel model;

		c1->setProgressionType(AutomationClip::ProgressionType::Linear);
		c1->putValue(0, 0.0, false);
		c1->putValue(10, 1.0, false);
		c1->addObject(&model);

		QCOMPARE(patternStore->automatedValuesAt( 0, patternTrack.patternIndex())[&model], 0.0f);
		QCOMPARE(patternStore->automatedValuesAt( 5, patternTrack.patternIndex())[&model], 0.5f);
		QCOMPARE(patternStore->automatedValuesAt(10, patternTrack.patternIndex())[&model], 1.0f);
		QCOMPARE(patternStore->automatedValuesAt(50, patternTrack.patternIndex())[&model], 1.0f);

		PatternTrack patternTrack2(song);

		QCOMPARE(patternStore->automatedValuesAt(5, patternTrack.patternIndex())[&model], 0.5f);
		QVERIFY(! patternStore->automatedValuesAt(5, patternTrack2.patternIndex()).size());

		PatternClip clip(&patternTrack);
		clip.changeLength(TimePos::ticksPerBar() * 2);
		clip.movePosition(0);

		QCOMPARE(song->automatedValuesAt(0)[&model], 0.0f);
		QCOMPARE(song->automatedValuesAt(5)[&model], 0.5f);
		QCOMPARE(song->automatedValuesAt(TimePos::ticksPerBar() + 5)[&model], 0.5f);
	}

	void testGlobalAutomation()
	{
		using namespace lmms;

		// Global automation should not have priority, see https://github.com/LMMS/lmms/issues/4268
		// Tests regression caused by 75077f6200a5aee3a5821aae48a3b8466ed8714a
		auto song = Engine::getSong();

		auto globalTrack = song->globalAutomationTrack();
		AutomationClip globalClip(globalTrack);

		AutomationTrack localTrack(song);
		AutomationClip localClip(&localTrack);

		FloatModel model;
		globalClip.setProgressionType(AutomationClip::ProgressionType::Discrete);
		localClip.setProgressionType(AutomationClip::ProgressionType::Discrete);
		globalClip.addObject(&model);
		localClip.addObject(&model);
		globalClip.putValue(0, 100.0f, false);
		localClip.putValue(0, 50.0f, false);

		QCOMPARE(song->automatedValuesAt(0)[&model], 50.0f);
	}

};

QTEST_GUILESS_MAIN(AutomationTrackTest)
#include "AutomationTrackTest.moc"
