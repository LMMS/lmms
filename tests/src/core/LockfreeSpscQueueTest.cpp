/*
 * LockfreeSpscQueueTest.cpp
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#include "LockfreeSpscQueue.h"

#include <QObject>
#include <QtTest>

using lmms::LockfreeSpscQueue;

class LockfreeSpscQueueTest : public QObject
{
	Q_OBJECT
private slots:
	void hasCorrectDefaults()
	{
		constexpr auto size = 4;
		auto queue = LockfreeSpscQueue<int, size>{};
		QCOMPARE(queue.size(), 0);
		QCOMPARE(queue.capacity(), size - 1);
	}

	void canEnqueueDequeueTest()
	{
		auto queue = LockfreeSpscQueue<int, 4>{};

		QVERIFY(queue.enqueue(1));
		QCOMPARE(queue.peek(), 1);
		QCOMPARE(queue.size(), 1);

		auto value = queue.dequeue();
		QVERIFY(value);
		QCOMPARE(*value, 1);

		QCOMPARE(queue.size(), 0);
		QCOMPARE(queue.empty(), true);
	}

	void canEnqueueDequeueBatchTest()
	{
		auto queue = LockfreeSpscQueue<int, 4>{};
		auto buffer = std::array{1, 2, 3};

		QVERIFY(queue.enqueue(buffer.data(), buffer.size()));

		// clear old values
		std::fill_n(buffer.data(), buffer.size(), 0);

		QVERIFY(queue.dequeue(buffer.data(), buffer.size()));
		QCOMPARE(buffer[0], 1);
		QCOMPARE(buffer[1], 2);
		QCOMPARE(buffer[2], 3);
	}

	void cantEnqueueWhenFullTest()
	{
		auto queue = LockfreeSpscQueue<int, 4>{};
		auto buffer = std::array{1, 2, 3};

		QVERIFY(queue.enqueue(buffer.data(), buffer.size()));
		QVERIFY(queue.full());

		QVERIFY(!queue.enqueue(4));
	}

	void cantDequeueWhenEmptyTest()
	{
		auto queue = LockfreeSpscQueue<int, 4>{};
		QVERIFY(queue.empty());

		QVERIFY(!queue.dequeue());
	}

	void canPeekBeforeDequeueTest()
	{
		auto queue = LockfreeSpscQueue<int, 4>{};

		QVERIFY(queue.enqueue(1));
		QCOMPARE(queue.peek(), 1);
	}

	void canWaitForDataTest()
	{
		using namespace std::chrono_literals;

		auto queue = LockfreeSpscQueue<int, 4>{};

		auto producer = std::thread{[&queue] {
			std::this_thread::sleep_for(25ms);
			queue.enqueue(1);
		}};

		auto consumer = std::thread{[&queue] {
			QVERIFY(queue.empty());
			queue.waitForData();

			auto value = queue.dequeue();

			QVERIFY(value);
			QCOMPARE(*value, 1);
		}};

		producer.join();
		consumer.join();
	}

	void canShutdownWhenWaitingForDataTest()
	{
		using namespace std::chrono_literals;

		auto queue = LockfreeSpscQueue<int, 4>{};

		auto producer = std::thread{[&queue] {
			std::this_thread::sleep_for(25ms);
			queue.shutdown();
		}};

		auto consumer = std::thread{[&queue] {
			QVERIFY(queue.empty());
			queue.waitForData();
		}};

		producer.join();
		consumer.join();
	}
};

QTEST_GUILESS_MAIN(LockfreeSpscQueueTest)
#include "LockfreeSpscQueueTest.moc"
