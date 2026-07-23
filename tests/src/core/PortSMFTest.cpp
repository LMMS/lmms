/*
 * PortSMFTest.cpp - tests for the bundled Standard MIDI File writer
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <QtTest>
#include <sstream>
#include <string>

#include "allegro.h"

class PortSMFTest : public QObject
{
	Q_OBJECT

private slots:
	void writesTypeOneTracksTempoAndChannels()
	{
		Alg_seq sequence;
		sequence.convert_to_beats();
		sequence.add_track(1);
		sequence.add_track(2);
		sequence.insert_tempo(120, 0);
		sequence.insert_tempo(90, 4);

		sequence.add_event(sequence.create_note(0, 2, 60, 60, 100, 1), 1);
		sequence.add_event(sequence.create_note(1, 9, 65, 65, 110, 1), 2);

		std::ostringstream output(std::ios::out | std::ios::binary);
		sequence.smf_write(output);
		QVERIFY(output.good());

		const std::string bytes = output.str();
		QVERIFY(bytes.size() >= 14);
		QCOMPARE(bytes.substr(0, 4), std::string{"MThd"});
		QCOMPARE(static_cast<unsigned char>(bytes[8]), 0);
		QCOMPARE(static_cast<unsigned char>(bytes[9]), 1);
		QCOMPARE(static_cast<unsigned char>(bytes[10]), 0);
		QCOMPARE(static_cast<unsigned char>(bytes[11]), 3);

		size_t trackCount = 0;
		for (size_t offset = 0; (offset = bytes.find("MTrk", offset)) != std::string::npos; offset += 4)
		{
			++trackCount;
		}
		QCOMPARE(trackCount, size_t{3});

		const std::string tempoEvent{"\xFF\x51\x03", 3};
		size_t tempoCount = 0;
		for (size_t offset = 0; (offset = bytes.find(tempoEvent, offset)) != std::string::npos;
			offset += tempoEvent.size())
		{
			++tempoCount;
		}
		QCOMPARE(tempoCount, size_t{2});

		const std::string channelTwoNote{"\x92\x3C\x64", 3};
		QVERIFY(bytes.find(channelTwoNote) != std::string::npos);
		const std::string channelNineNote{"\x99\x41\x6E", 3};
		QVERIFY(bytes.find(channelNineNote) != std::string::npos);

		std::istringstream input(bytes, std::ios::in | std::ios::binary);
		Alg_seq imported(input, true);
		QCOMPARE(imported.get_read_error(), alg_no_error);
		QCOMPARE(imported.tracks(), 3);
		QVERIFY(imported.track(1)->length() >= 1);
		QVERIFY(imported.track(2)->length() >= 1);
		QCOMPARE((*imported.track(1))[0]->chan, 2L);
		QCOMPARE((*imported.track(2))[0]->chan, 9L);
	}
};

QTEST_GUILESS_MAIN(PortSMFTest)
#include "PortSMFTest.moc"
