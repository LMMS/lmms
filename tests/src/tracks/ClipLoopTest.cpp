/*
 * ClipLoopTest.cpp
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
#include <QtTest>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "Song.h"

class ClipLoopTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		lmms::Engine::init(true);
	}

	void cleanupTestCase()
	{
		lmms::Engine::destroy();
	}

	void testContentPositionWithoutLoop()
	{
		using namespace lmms;
		InstrumentTrack track(Engine::getSong());
		MidiClip clip(&track);
		clip.changeLength(192);
		clip.setStartTimeOffset(-48);

		QVERIFY(!clip.isLooping());
		QCOMPARE(clip.contentPosition(0).getTicks(), 48);
		QCOMPARE(clip.contentPosition(300).getTicks(), 348);
	}

	void testContentPositionWraps()
	{
		using namespace lmms;
		InstrumentTrack track(Engine::getSong());
		MidiClip clip(&track);
		clip.changeLength(192);
		clip.setStartTimeOffset(-48);
		clip.loopVisibleContent();

		QVERIFY(clip.isLooping());
		QCOMPARE(clip.loopStart().getTicks(), 48);
		QCOMPARE(clip.loopLength().getTicks(), 192);

		// Extending the clip repeats the visible section
		clip.changeLength(192 * 4);
		QCOMPARE(clip.contentPosition(0).getTicks(), 48);
		QCOMPARE(clip.contentPosition(191).getTicks(), 239);
		QCOMPARE(clip.contentPosition(192).getTicks(), 48);
		QCOMPARE(clip.contentPosition(192 * 3 + 10).getTicks(), 58);

		// Extending the clip to the left (moving the content right) repeats it before the start too
		clip.setStartTimeOffset(-48 + 20);
		QCOMPARE(clip.contentPosition(0).getTicks(), 48 + 192 - 20);
		QCOMPARE(clip.contentPosition(20).getTicks(), 48);

		clip.setLoop(0, 0);
		QVERIFY(!clip.isLooping());
		QCOMPARE(clip.contentPosition(20).getTicks(), 48);
	}

	void testLoopIsSavedAndCloned()
	{
		using namespace lmms;
		InstrumentTrack track(Engine::getSong());
		MidiClip clip(&track);
		clip.changeLength(96);
		clip.setLoop(24, 48);

		QDomDocument doc;
		QDomElement parent = doc.createElement("test");
		doc.appendChild(parent);
		clip.saveState(doc, parent);

		MidiClip loaded(&track);
		loaded.restoreState(parent.firstChildElement());
		QCOMPARE(loaded.loopStart().getTicks(), 24);
		QCOMPARE(loaded.loopLength().getTicks(), 48);

		auto cloned = std::unique_ptr<MidiClip>{clip.clone()};
		QCOMPARE(cloned->loopStart().getTicks(), 24);
		QCOMPARE(cloned->loopLength().getTicks(), 48);

		// Clips that don't loop don't store loop attributes, and load as not looping
		MidiClip plain(&track);
		QDomElement plainParent = doc.createElement("plain");
		plain.saveState(doc, plainParent);
		QVERIFY(!plainParent.firstChildElement().hasAttribute("looplen"));
		loaded.restoreState(plainParent.firstChildElement());
		QVERIFY(!loaded.isLooping());
	}

	void testFractionalTempo()
	{
		using namespace lmms;
		auto song = Engine::getSong();

		song->setTempo(120.5f);
		QCOMPARE(song->getTempo(), 120.5f);

		const auto expectedFramesPerTick
			= Engine::audioEngine()->outputSampleRate() * 60.0f * 4 / DefaultTicksPerBar / 120.5f;
		QCOMPARE(Engine::framesPerTick(), expectedFramesPerTick);

		song->setTempo(DefaultTempo);
	}
};

QTEST_GUILESS_MAIN(ClipLoopTest)
#include "ClipLoopTest.moc"
