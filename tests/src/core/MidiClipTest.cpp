/*
 * MidiClipTest.cpp
 *
 * Copyright (c) 2025 LMMS - https://lmms.io
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

#include <QElapsedTimer>

#include "Clip.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "Note.h"
#include "Song.h"
#include "Track.h"


class MidiClipTest : public QObject
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

	void testCopyDataTo()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setName("Source Clip");
		src.setAutoResize(false);
		src.changeLength(TimePos(4, 0));
		src.setColor(QColor(255, 0, 0));

		Note note1(TimePos(0, 48), TimePos(0, 0), 60, 100, 0);
		Note note2(TimePos(0, 96), TimePos(1, 0), 64, 80, -20);
		Note note3(TimePos(0, 48), TimePos(2, 0), 67, 120, 20);
		src.addNote(note1, false);
		src.addNote(note2, false);
		src.addNote(note3, false);

		QCOMPARE(src.notes().size(), (std::size_t)3);

		MidiClip dst(&track);
		dst.changeLength(TimePos(1, 0));
		dst.setName("Destination");

		QVERIFY(src.copyDataTo(&dst));
		QCOMPARE(dst.notes().size(), (std::size_t)3);
		QCOMPARE(dst.name(), QString("Source Clip"));
		QVERIFY(dst.color().has_value());
		QCOMPARE(*dst.color(), QColor(255, 0, 0));
		QCOMPARE(dst.length(), TimePos(4, 0));
		QCOMPARE(dst.getAutoResize(), false);
		QCOMPARE(dst.isMuted(), false);
		QCOMPARE(dst.type(), src.type());

		auto& dstNotes = dst.notes();
		QCOMPARE(dstNotes[0]->key(), 60);
		QCOMPARE(dstNotes[0]->length(), TimePos(0, 48));
		QCOMPARE(dstNotes[0]->pos(), TimePos(0, 0));
		QCOMPARE(dstNotes[0]->getVolume(), (volume_t)100);
		QCOMPARE(dstNotes[0]->getPanning(), (panning_t)0);

		QCOMPARE(dstNotes[1]->key(), 64);
		QCOMPARE(dstNotes[1]->getVolume(), (volume_t)80);
		QCOMPARE(dstNotes[1]->getPanning(), (panning_t)-20);

		QCOMPARE(dstNotes[2]->key(), 67);
		QCOMPARE(dstNotes[2]->getVolume(), (volume_t)120);
		QCOMPARE(dstNotes[2]->getPanning(), (panning_t)20);
		QCOMPARE(dstNotes[2]->pos(), TimePos(2, 0));
	}

	void testCopyDataToNoteIndependence()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.addNote(Note(TimePos(0, 48), TimePos(0, 0), 60), false);

		MidiClip dst(&track);
		src.copyDataTo(&dst);

		QCOMPARE(src.notes().size(), (std::size_t)1);
		QCOMPARE(dst.notes().size(), (std::size_t)1);

		dst.notes()[0]->setKey(72);
		QCOMPARE(src.notes()[0]->key(), 60);
		QCOMPARE(dst.notes()[0]->key(), 72);
	}

	void testCopyDataToMuted()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.toggleMute();
		QVERIFY(src.isMuted());

		MidiClip dst(&track);
		QVERIFY(!dst.isMuted());

		src.copyDataTo(&dst);
		QVERIFY(dst.isMuted());
	}

	void testCopyDataToEmptySource()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.changeLength(TimePos(8, 0));
		src.setName("Empty Clip");

		MidiClip dst(&track);
		dst.addNote(Note(TimePos(0, 48)), false);
		QCOMPARE(dst.notes().size(), (std::size_t)1);

		src.copyDataTo(&dst);
		QCOMPARE(dst.notes().size(), (std::size_t)0);
		QCOMPARE(dst.name(), QString("Empty Clip"));
		QCOMPARE(dst.length(), TimePos(8, 0));
	}

	void testCopyDataToWrongType()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);

		QVERIFY(!src.copyDataTo(nullptr));
	}

	void testCopyStateToUsesFastPath()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setName("Source via copyStateTo");
		src.changeLength(TimePos(2, 0));
		src.addNote(Note(TimePos(0, 96), TimePos(0, 0), 62), false);
		src.addNote(Note(TimePos(0, 96), TimePos(1, 0), 65), false);

		MidiClip dst(&track);
		TimePos dstPos(TimePos(4, 0));
		dst.movePosition(dstPos);

		Clip::copyStateTo(&src, &dst);

		QCOMPARE(dst.notes().size(), (std::size_t)2);
		QCOMPARE(dst.name(), QString("Source via copyStateTo"));
		QCOMPARE(dst.length(), TimePos(2, 0));

		QCOMPARE(dst.notes()[0]->key(), 62);
		QCOMPARE(dst.notes()[1]->key(), 65);
	}

	void testCopyStateToCrossTrack()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track1(song);
		InstrumentTrack track2(song);

		MidiClip src(&track1);
		src.addNote(Note(TimePos(0, 48)), false);

		MidiClip dst(&track2);

		Clip::copyStateTo(&src, &dst);
		QCOMPARE(dst.notes().size(), (std::size_t)1);
	}

	void testCloneWithNullTrack()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.addNote(Note(TimePos(0, 48), TimePos(0, 0), 60), false);
		src.addNote(Note(TimePos(0, 48), TimePos(1, 0), 64), false);
		src.setName("NullTrackTest");
		src.setColor(QColor(0, 255, 0));

		std::unique_ptr<MidiClip> clone(static_cast<MidiClip*>(src.clone()));
		// detach clone from track to simulate internal-copy buffer lifecycle
		track.removeClip(clone.get());

		QVERIFY(clone != nullptr);
		QCOMPARE(clone->notes().size(), (std::size_t)2);

		MidiClip dst(&track);
		QVERIFY(clone->copyDataTo(&dst));
		QCOMPARE(dst.notes().size(), (std::size_t)2);
		QCOMPARE(dst.name(), QString("NullTrackTest"));
		QVERIFY(dst.color().has_value());
		QCOMPARE(*dst.color(), QColor(0, 255, 0));
		QCOMPARE(dst.notes()[0]->key(), 60);
		QCOMPARE(dst.notes()[1]->key(), 64);
		QVERIFY(dst.getTrack() == &track);
	}

	void stressMultiCopy()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setAutoResize(false);
		int notesPerClip = 1120;
		for (int n = 0; n < notesPerClip; n++)
		{
			int key = 36 + (n % 48);
			src.addNote(Note(TimePos(0, 12), TimePos(n / 4, (n % 4) * 12), key, 100, 0), false);
		}

		int numCopies = 50;
		QList<MidiClip*> dstClips;
		QElapsedTimer t;
		t.start();

		for (int i = 0; i < numCopies; i++)
		{
			auto* dst = new MidiClip(&track);
			QVERIFY(src.copyDataTo(dst));
			QCOMPARE(dst->notes().size(), (std::size_t)notesPerClip);
			dstClips.push_back(dst);
		}

		qDebug() << "STRESS_MULTI notes=" << notesPerClip << " copies=" << numCopies
			 << " ms=" << t.elapsed();

		QCOMPARE(dstClips.size(), numCopies);
		qDeleteAll(dstClips);
	}

	void stressFuzzyNotes()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setAutoResize(false);

		src.addNote(Note(TimePos(0, 0),  TimePos(0, 0),   0,   0,  -100), false);
		src.addNote(Note(TimePos(0, 1),  TimePos(0, 0),  60,   100, 0),   false);
		src.addNote(Note(TimePos(4, 0),  TimePos(0, 0),  100,  50, 50),  false);
		src.addNote(Note(TimePos(0, 48), TimePos(1, 0),  127,  127, 100), false);
		src.addNote(Note(TimePos(0, 48), TimePos(0, 0),  64,   80, 0),   false);
		src.addNote(Note(TimePos(0, 1),  TimePos(10, 0), 50,   40, -50), false);

		src.toggleMute();
		src.setName("FuzzySrc");
		src.setColor(QColor(100, 200, 50));

		for (int i = 0; i < 10; i++)
		{
			MidiClip dst(&track);
			QVERIFY(src.copyDataTo(&dst));
			QCOMPARE(dst.notes().size(), src.notes().size());
			QCOMPARE(dst.name(), QString("FuzzySrc"));
			QVERIFY(dst.isMuted());
			QCOMPARE(dst.length(), src.length());

			for (std::size_t n = 0; n < src.notes().size(); n++)
			{
				QCOMPARE(dst.notes()[n]->key(),      src.notes()[n]->key());
				QCOMPARE(dst.notes()[n]->getVolume(), src.notes()[n]->getVolume());
				QCOMPARE(dst.notes()[n]->getPanning(),src.notes()[n]->getPanning());
				QCOMPARE(dst.notes()[n]->pos(),       src.notes()[n]->pos());
				QCOMPARE(dst.notes()[n]->length(),    src.notes()[n]->length());
			}
		}
	}

	void stressCloneRepeat()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setAutoResize(false);
		int notesPerClip = 1120;
		for (int n = 0; n < notesPerClip; n++)
		{
			int key = 36 + (n % 48);
			src.addNote(Note(TimePos(0, 12), TimePos(n / 4, (n % 4) * 12), key, 100, 0), false);
		}

		int iterations = 50;
		QElapsedTimer t;
		t.start();

		for (int i = 0; i < iterations; i++)
		{
			std::unique_ptr<MidiClip> clone(static_cast<MidiClip*>(src.clone()));
			track.removeClip(clone.get());

			MidiClip dst(&track);
			QVERIFY(clone->copyDataTo(&dst));
			QCOMPARE(dst.notes().size(), (std::size_t)notesPerClip);
		}

		qDebug() << "STRESS_CLONE notes=" << notesPerClip << " iter=" << iterations
			 << " ms=" << t.elapsed();
	}

	void stressOverwrite()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setAutoResize(false);
		int notesPerClip = 1120;
		for (int n = 0; n < notesPerClip; n++)
		{
			int key = 36 + (n % 48);
			src.addNote(Note(TimePos(0, 12), TimePos(n / 4, (n % 4) * 12), key, 100, 0), false);
		}

		MidiClip dst(&track);
		for (int n = 0; n < 500; n++)
		{
			dst.addNote(Note(TimePos(0, 48), TimePos(n, 0), 72, 50, 0), false);
		}
		dst.setName("PreOverwrite");
		dst.toggleMute();

		QVERIFY(src.copyDataTo(&dst));
		QCOMPARE(dst.notes().size(), src.notes().size());
		QCOMPARE(dst.name(), src.name());
		QVERIFY(!dst.isMuted());
	}

	void stressFalseStart()
	{
		using namespace lmms;

		auto song = Engine::getSong();
		InstrumentTrack track(song);

		MidiClip src(&track);
		src.setAutoResize(false);
		for (int n = 0; n < 500; n++)
		{
			int key = 36 + (n % 48);
			src.addNote(Note(TimePos(0, 12), TimePos(n / 4, (n % 4) * 12), key, 100, 0), false);
		}

		MidiClip dst(&track);
		QVERIFY(!src.copyDataTo(nullptr));
		QVERIFY(src.copyDataTo(&dst));
		QCOMPARE(dst.notes().size(), (std::size_t)500);
	}
};


QTEST_GUILESS_MAIN(MidiClipTest)
#include "MidiClipTest.moc"
