/*
* MPETest.cpp
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
#include "MPEManager.h"


class MPETest : public QObject
{
	Q_OBJECT
public:

private slots:
	/** @brief Checks that notes are routed to the channels with the least number of active notes, or the channel with the oldest NoteOff in the case of a tie.
	 */
	void MPEChannelRoutingTest()
	{
		using namespace lmms;

		MPEManager mpeManager;

		// Setup lower zone to use first 5 channels (0 is the manager channel, 1-4 are member channels), and 0 channels for the upper zone
		mpeManager.config(5, 0);

		// Currently, the active note counts should be 0 on all channels
		// 0 0 0 0 0

		// "Send" noteOn events to some of the channels
		mpeManager.noteOn(1);
		mpeManager.noteOn(1);
		mpeManager.noteOn(1);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(3);
		mpeManager.noteOn(3);
		// Now, the active note counts should look like:
		// 0 3 4 2 0
		// Ignoring the first channel (manager channel), the next active note should be spawned in channel 4, since it has the least notes.
		QCOMPARE(mpeManager.findAvailableChannel(), 4);


		// Add some notes to channel 4
		mpeManager.noteOn(4);
		mpeManager.noteOn(4);
		mpeManager.noteOn(4);

		// The note counts should look like this:
		// 0 3 4 2 3
		// Now channel 3 has the fewest active notes
		QCOMPARE(mpeManager.findAvailableChannel(), 3);

		// Release a few of the notes
		mpeManager.noteOff(2);
		mpeManager.noteOff(2);
		mpeManager.noteOff(2);
		mpeManager.noteOff(1);
		mpeManager.noteOff(1);
		mpeManager.noteOff(3);
		mpeManager.noteOff(4);
		mpeManager.noteOff(4);
		// The note counts should look like this:
		// 0 1 1 1 1

		// According to the MPE specification best practice (page 17, A.3), if channels are tied for the number of active notes, the note should be routed to the channel with the oldest noteOff signal.
		// In this case, channel 2 released its notes first, so the new note should be routed there
		QCOMPARE(mpeManager.findAvailableChannel(), 2);
	}
};

QTEST_GUILESS_MAIN(MPETest)
#include "MPETest.moc"
