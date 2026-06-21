/*
 * MidiFileTest.cpp - tests for Standard MIDI File serialization
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <array>

#include <QtTest>

#include "MidiFile.hpp"

class MidiFileTest : public QObject
{
	Q_OBJECT

private slots:
	void writesTypeOneHeader()
	{
		std::array<uint8_t, 14> bytes{};
		MidiFile::MIDIHeader header(3, MidiFile::MIDIFormat::Type1);

		QCOMPARE(header.writeToBuffer(bytes.data()), 14);
		QCOMPARE(static_cast<int>(bytes[8]), 0);
		QCOMPARE(static_cast<int>(bytes[9]), 1);
		QCOMPARE(static_cast<int>(bytes[10]), 0);
		QCOMPARE(static_cast<int>(bytes[11]), 3);
	}

	void writesChannelSpecificNoteEvents()
	{
		std::array<uint8_t, 64> bytes{};
		MidiFile::MIDITrack<64> track;
		track.channel = 9;
		track.addNoteAtTick(60, 100, 0, 128);

		QCOMPARE(track.writeToBuffer(bytes.data()), 21);
		QCOMPARE(static_cast<int>(bytes[9]), 0x99);
		QCOMPARE(static_cast<int>(bytes[14]), 0x89);
	}

	void writesTempoMetaEvent()
	{
		std::array<uint8_t, 64> bytes{};
		MidiFile::MIDITrack<64> track;
		track.addTempo(120, 0);

		QCOMPARE(track.writeToBuffer(bytes.data()), 19);
		QCOMPARE(static_cast<int>(bytes[9]), 0xff);
		QCOMPARE(static_cast<int>(bytes[10]), 0x51);
		QCOMPARE(static_cast<int>(bytes[11]), 0x03);
		QCOMPARE(static_cast<int>(bytes[12]), 0x07);
		QCOMPARE(static_cast<int>(bytes[13]), 0xa1);
		QCOMPARE(static_cast<int>(bytes[14]), 0x20);
	}

	void serializesLargeTracksDynamically()
	{
		MidiFile::MIDITrack<64> track;
		for (uint32_t event = 0; event < 20; ++event)
		{
			track.addTempo(120 + event, event * 128);
		}

		const auto bytes = track.writeToVector();
		QVERIFY(bytes.size() > 64);
		QCOMPARE(static_cast<int>(bytes[0]), 'M');
		QCOMPARE(static_cast<int>(bytes[1]), 'T');
		QCOMPARE(static_cast<int>(bytes[2]), 'r');
		QCOMPARE(static_cast<int>(bytes[3]), 'k');
	}
};

QTEST_GUILESS_MAIN(MidiFileTest)
#include "MidiFileTest.moc"
