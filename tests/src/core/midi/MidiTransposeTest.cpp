/*
* MidiTransposeTest.cpp
*
* Copyright (c) 2025 Keratin
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
#include <QDebug>

#include "TrackContainer.h"
#include "Engine.h"
#include "Track.h"
#include "Song.h"
#include "InstrumentTrack.h"
#include "MidiEvent.h"
#include "Instrument.h"

#include <iostream>

// TODO rename to InstrumentMidiOutputTest.cpp or something, since this is not only about tranposition.

extern "C"
{
lmms::Plugin::Descriptor dummy_midi_relay_instrument_descriptor =
	{"test", "DummyMidiRelayInstrument", "", "", 0x0100, lmms::Plugin::Type::Instrument, nullptr, "", nullptr};
}

class DummyMidiRelayInstrument : public lmms::Instrument
{
public:
	DummyMidiRelayInstrument(lmms::InstrumentTrack * _instrument_track):
		Instrument(_instrument_track, &dummy_midi_relay_instrument_descriptor) {}
	inline bool handleMidiEvent(const lmms::MidiEvent& event, const lmms::TimePos& = lmms::TimePos(), lmms::f_cnt_t offset = 0) override
	{
		m_lastEvent = event;
		return true;
	}
	QString nodeName() const override { return "DummyMidiRelayInstrument"; }
	void saveSettings(QDomDocument& doc, QDomElement& elem) override {};
	void loadSettings(const QDomElement& elem) override {};
	lmms::gui::PluginView* instantiateView(QWidget* w) override { return nullptr; }

	lmms::MidiEvent m_lastEvent;
};
/*
extern "C"
{
// necessary for getting instance out of shared lib
DummyMidiRelayInstrument * lmms_plugin_main(lmms::Model * model, void *)
{
	return new DummyMidiRelayInstrument(static_cast<lmms::InstrumentTrack *>(model));
}
}*/


class MidiTransposeTest : public QObject
{
	Q_OBJECT
public:
	lmms::InstrumentTrack* m_track;
	DummyMidiRelayInstrument* m_instrument;

	lmms::MidiEvent testMidiEvent(const lmms::MidiEvent& event)
	{
		using namespace lmms;
		// Reset last event so that it doesn't accidentally get returned if this sent event doesn't generate an out event.
		// Using MidiSystemReset as a temporary value. The -1 parameters are what should be checked to make sure the event is invalid (i.e., no event was recieved.)
		m_instrument->m_lastEvent = MidiEvent(MidiSystemReset, -1, -1);
		m_track->processInEvent(event, 0, 0);
		Engine::audioEngine()->renderNextBuffer();
		return m_instrument->m_lastEvent;
	}

	void resetTrack()
	{
		using namespace lmms;
		m_track->baseNoteModel()->setValue(DefaultBaseKey);
		m_track->firstKeyModel()->setValue(0);
		m_track->lastKeyModel()->setValue(NumKeys - 1);
		m_track->midiPort()->setOutputChannel(0);
		m_track->midiPort()->setWritable(false);
		m_track->midiPort()->setMPEEnabled(false);
		m_track->midiPort()->setMPEPitchRange(16);
		Engine::getSong()->setMasterPitch(0);
	}

private slots:
	void initTestCase()
	{
		using namespace lmms;
		Engine::init(true);
		NotePlayHandleManager::init();

		m_track = dynamic_cast<InstrumentTrack*>(Track::create(Track::Type::Instrument, Engine::getSong()));
		m_instrument = new DummyMidiRelayInstrument(m_track);
		m_track->setInstrument(m_instrument);
	}

	void cleanupTestCase()
	{
		using namespace lmms;
		Engine::destroy();
		NotePlayHandleManager::free();
	}

	/** @brief Tests all notes to ensure the min/max keyboard boundaries are respected.
	 */
	void KeyboardRangeTest()
	{
		using namespace lmms;
		resetTrack();

		// Set the first and last key bounds to something non-zero
		m_track->firstKeyModel()->setValue(13);
		m_track->lastKeyModel()->setValue(NumKeys - 13);

		for (int key = 0; key < NumKeys; ++key)
		{
			// Using channel 0, but it shouldn't matter.
			MidiEvent testOnEvent = MidiEvent(MidiNoteOn, 0, key, 100);
			MidiEvent recievedOnEvent = testMidiEvent(testOnEvent);
			MidiEvent testOffEvent = MidiEvent(MidiNoteOff, 0, key, 100);
			MidiEvent recievedOffEvent = testMidiEvent(testOffEvent);
			if (m_track->isKeyMapped(key))
			{
				QCOMPARE(recievedOnEvent.type(), testOnEvent.type());
				QCOMPARE(recievedOnEvent.key(), testOnEvent.key());

				QCOMPARE(recievedOffEvent.type(), testOffEvent.type());
				QCOMPARE(recievedOffEvent.key(), testOffEvent.key());
			}
			else
			{
				// If the key is not mapped, it should not create an event (the event returned by testMidiEvent will be invalid)
				QCOMPARE(recievedOnEvent.key(), -1);
				QCOMPARE(recievedOffEvent.key(), -1);
			}
		}
	}


	/** @brief Tests changing the base note square above the instrument piano, and checks that the output midi signals are transposed correctly.
	 */
	void BaseNoteTest()
	{
		using namespace lmms;
		resetTrack();
		// With no base note offset, the output note should match the input
		MidiEvent testOnEvent = MidiEvent(MidiNoteOn, 0, DefaultKey, 100);
		MidiEvent recievedOnEvent = testMidiEvent(testOnEvent);
		MidiEvent testOffEvent = MidiEvent(MidiNoteOff, 0, DefaultKey, 100);
		MidiEvent recievedOffEvent = testMidiEvent(testOffEvent);
		QCOMPARE(recievedOnEvent.key(), DefaultKey);
		QCOMPARE(recievedOffEvent.key(), DefaultKey);

		// Moving the base note up 1 semitone should make the output notes one semitone lower
		m_track->baseNoteModel()->setValue(DefaultBaseKey + 1);

		testOnEvent = MidiEvent(MidiNoteOn, 0, DefaultKey, 100);
		recievedOnEvent = testMidiEvent(testOnEvent);
		testOffEvent = MidiEvent(MidiNoteOff, 0, DefaultKey, 100);
		recievedOffEvent = testMidiEvent(testOffEvent);
		QCOMPARE(recievedOnEvent.key(), DefaultKey - 1);
		QCOMPARE(recievedOffEvent.key(), DefaultKey - 1);
	}


	/** @brief Tests changing the global transposition/master pitch slider at the top of the windw, and checks that the output midi signals are transposed correctly.
	 */
	void GlobalTranspositionTest()
	{
		using namespace lmms;
		resetTrack();
		// With no base note offset, the output note should match the input
		MidiEvent testOnEvent = MidiEvent(MidiNoteOn, 0, DefaultKey, 100);
		MidiEvent recievedOnEvent = testMidiEvent(testOnEvent);
		MidiEvent testOffEvent = MidiEvent(MidiNoteOff, 0, DefaultKey, 100);
		MidiEvent recievedOffEvent = testMidiEvent(testOffEvent);
		QCOMPARE(recievedOnEvent.key(), DefaultKey);
		QCOMPARE(recievedOffEvent.key(), DefaultKey);

		// Setting the global transposition to 1 should move all output notes up by 1 semitone
		Engine::getSong()->setMasterPitch(1);

		testOnEvent = MidiEvent(MidiNoteOn, 0, DefaultKey, 100);
		recievedOnEvent = testMidiEvent(testOnEvent);
		testOffEvent = MidiEvent(MidiNoteOff, 0, DefaultKey, 100);
		recievedOffEvent = testMidiEvent(testOffEvent);
		QCOMPARE(recievedOnEvent.key(), DefaultKey + 1);
		QCOMPARE(recievedOffEvent.key(), DefaultKey + 1);
	}


	/** @brief Ensures the output channel of all forms of midi input matches the specified instrument output channel
	 */
	void OutputChannelTest()
	{
		using namespace lmms;
		resetTrack();

		// With no output channel specified and midi output disabled, the output channel should match the input channel.
		for (int channel = 0; channel < MidiChannelCount; ++channel)
		{
			MidiEvent testOnEvent = MidiEvent(MidiNoteOn, channel, DefaultKey, 100);
			MidiEvent recievedOnEvent = testMidiEvent(testOnEvent);
			MidiEvent testOffEvent = MidiEvent(MidiNoteOff, channel, DefaultKey, 100);
			MidiEvent recievedOffEvent = testMidiEvent(testOffEvent);
			QCOMPARE(recievedOnEvent.channel(), channel);
			QCOMPARE(recievedOffEvent.channel(), channel);
		}

		// Set the output channel to be non-zero and enable midi output
		// NOTE: the output channel model is 1-indexed, so it accepts values 1-16 for the midi channels. 0 just means it is auto-routed.
		m_track->midiPort()->setOutputChannel(7 + 1);
		m_track->midiPort()->setWritable(true);
		// Output channel should now match regardless of input channel.
		for (int channel = 0; channel < MidiChannelCount; ++channel)
		{
			MidiEvent testOnEvent = MidiEvent(MidiNoteOn, channel, DefaultKey, 100);
			MidiEvent recievedOnEvent = testMidiEvent(testOnEvent);
			MidiEvent testOffEvent = MidiEvent(MidiNoteOff, channel, DefaultKey, 100);
			MidiEvent recievedOffEvent = testMidiEvent(testOffEvent);
			QCOMPARE(recievedOnEvent.channel(), 7);
			QCOMPARE(recievedOffEvent.channel(), 7);
		}
	}
};

QTEST_GUILESS_MAIN(MidiTransposeTest)
#include "MidiTransposeTest.moc"
