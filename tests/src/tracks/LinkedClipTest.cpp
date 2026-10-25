/*
 * LinkedClipTest.cpp
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

class LinkedClipTest : public QObject
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

	void testLinkedClonesShareNotes()
	{
		using namespace lmms;
		InstrumentTrack track(Engine::getSong());
		InstrumentTrack otherTrack(Engine::getSong());
		auto a = new MidiClip(&track);
		a->addNote(Note(TimePos(48), TimePos(0), 60), false);

		auto b = a->linkedClone();
		auto c = new MidiClip(&otherTrack);
		c->linkTo(a);
		QVERIFY(a->isLinked() && b->isLinked() && c->isLinked());
		QCOMPARE(a->linkedClips().size(), size_t{3});
		QCOMPARE(c->notes().size(), size_t{1});

		// Editing any of them changes all of them, including their automatic length
		c->addNote(Note(TimePos(48), TimePos(192 * 2), 64), false);
		QCOMPARE(a->notes().size(), size_t{2});
		QCOMPARE(b->notes().size(), size_t{2});
		QCOMPARE(a->length().getTicks(), 192 * 3);
		QCOMPARE(b->length().getTicks(), 192 * 3);

		a->notes().front()->setKey(62);
		QCOMPARE(b->notes().front()->key(), 62);

		// An ordinary clone is independent
		auto independent = a->clone();
		QVERIFY(!independent->isLinked());
		independent->clearNotes();
		QCOMPARE(a->notes().size(), size_t{2});

		// Deleting a linked clip keeps the notes for the others
		delete a;
		QCOMPARE(b->linkedClips().size(), size_t{2});
		QCOMPARE(b->notes().size(), size_t{2});

		b->unlink();
		QVERIFY(!b->isLinked() && !c->isLinked());
		b->clearNotes();
		QCOMPARE(c->notes().size(), size_t{2});

		delete b;
		delete c;
		delete independent;
	}

	void testLinksAreSavedAndRestored()
	{
		using namespace lmms;
		InstrumentTrack track(Engine::getSong());
		auto a = new MidiClip(&track);
		a->addNote(Note(TimePos(48), TimePos(0), 60), false);
		auto b = a->linkedClone();

		QDomDocument doc;
		QDomElement parent = doc.createElement("test");
		doc.appendChild(parent);
		a->saveState(doc, parent);
		b->saveState(doc, parent);
		const QString id = parent.firstChildElement().attribute("link");
		QVERIFY(!id.isEmpty());
		QCOMPARE(parent.lastChildElement().attribute("link"), id);

		// Restoring a clip's old state (as undo does) restores the notes of all linked clips
		a->addNote(Note(TimePos(48), TimePos(96), 62), false);
		QCOMPARE(b->notes().size(), size_t{2});
		a->restoreState(parent.firstChildElement());
		QCOMPARE(b->notes().size(), size_t{1});
		QVERIFY(a->isLinked());

		// Loading clips with the same link ID links them; while the originals exist, also to the originals
		InstrumentTrack otherTrack(Engine::getSong());
		MidiClip joined(&otherTrack);
		joined.restoreState(parent.firstChildElement());
		QCOMPARE(a->linkedClips().size(), size_t{3});
		joined.unlink();

		// Renewed IDs link the loaded clips with each other only
		MidiClip::renewLinkIds(parent);
		QVERIFY(parent.firstChildElement().attribute("link") != id);
		MidiClip copyA(&otherTrack);
		MidiClip copyB(&otherTrack);
		copyA.restoreState(parent.firstChildElement());
		copyB.restoreState(parent.lastChildElement());
		QCOMPARE(copyA.linkedClips().size(), size_t{2});
		QCOMPARE(a->linkedClips().size(), size_t{2});
		copyA.addNote(Note(TimePos(48), TimePos(96), 62), false);
		QCOMPARE(copyB.notes().size(), size_t{2});
		QCOMPARE(a->notes().size(), size_t{1});

		// Loading a state without a link ID unlinks the clip
		MidiClip plain(&track);
		QDomElement plainParent = doc.createElement("plain");
		plain.saveState(doc, plainParent);
		QVERIFY(!plainParent.firstChildElement().hasAttribute("link"));
		b->restoreState(plainParent.firstChildElement());
		QVERIFY(!a->isLinked());
		QVERIFY(b->notes().empty());
		QCOMPARE(a->notes().size(), size_t{1});

		delete a;
		delete b;
	}

	void testLinkLabels()
	{
		using namespace lmms;
		QCOMPARE(MidiClip::linkLabel(0), QString("A"));
		QCOMPARE(MidiClip::linkLabel(25), QString("Z"));
		QCOMPARE(MidiClip::linkLabel(26), QString("A1"));
		QCOMPARE(MidiClip::linkLabel(27), QString("B1"));
		QCOMPARE(MidiClip::linkLabel(26 * 2 + 25), QString("Z2"));

		InstrumentTrack track(Engine::getSong());
		auto a = new MidiClip(&track);
		QVERIFY(a->linkLabel().isEmpty());

		// Each linked group gets the next label, and linked clips share it
		auto a2 = a->linkedClone();
		auto b = new MidiClip(&track);
		auto b2 = b->linkedClone();
		auto b3 = b->linkedClone();
		QCOMPARE(a->linkLabel(), QString("A"));
		QCOMPARE(a2->linkLabel(), QString("A"));
		QCOMPARE(b->linkLabel(), QString("B"));
		QCOMPARE(b3->linkLabel(), QString("B"));

		// Groups that are no longer linked give up their label, and the next new group takes it
		delete a2;
		QVERIFY(a->linkLabel().isEmpty());
		b3->unlink();
		QVERIFY(b3->linkLabel().isEmpty());
		QCOMPARE(b->linkLabel(), QString("B"));
		auto c = new MidiClip(&track);
		c->linkTo(b3);
		QCOMPARE(c->linkLabel(), QString("A"));
		QCOMPARE(b3->linkLabel(), QString("A"));

		// A clip moving to another group takes that group's label
		b2->linkTo(c);
		QCOMPARE(b2->linkLabel(), QString("A"));
		QVERIFY(b->linkLabel().isEmpty());

		// After Z come A1, B1, ...
		std::vector<MidiClip*> clips;
		for (int i = 0; i < 26; ++i)
		{
			clips.push_back(new MidiClip(&track));
			clips.push_back(clips.back()->linkedClone());
		}
		QCOMPARE(clips[0]->linkLabel(), QString("B"));
		QCOMPARE(clips[2 * 24]->linkLabel(), QString("Z"));
		QCOMPARE(clips[2 * 25]->linkLabel(), QString("A1"));
		QCOMPARE(clips[2 * 25 + 1]->linkLabel(), QString("A1"));

		for (const auto& clip : clips) { delete clip; }
		delete a;
		delete b;
		delete b2;
		delete b3;
		delete c;
	}
};

QTEST_GUILESS_MAIN(LinkedClipTest)
#include "LinkedClipTest.moc"
