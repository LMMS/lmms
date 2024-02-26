/*
 * ThreadPoolTest.cpp
 *
 * Copyright (c) 2024 saker
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

#include "ThreadPool.h"

#include <QObject>
#include <QtTest/QtTest>
#include <chrono>

class ThreadPoolTest : public QObject
{
	Q_OBJECT
private:
	static constexpr size_t NumWorkers = 2;

	auto enqueueDummyTask() -> std::future<bool>
	{
		using namespace lmms;
		auto task = ThreadPool::instance().enqueue([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			return true;
		});
		return task;
	}

private slots:

	void initTestCase()
	{
		using namespace lmms;
		ThreadPool::init(NumWorkers);
	}

	void canSetNumWorkersTest()
	{
		using namespace lmms;
		QCOMPARE(ThreadPool::instance().numWorkers(), NumWorkers);
	}

	void canProcessTaskTest()
	{
		using namespace lmms;
		auto task = enqueueDummyTask();
		QCOMPARE(task.get(), true);
	}

	void canProcessMultipleTasksTest()
	{
		using namespace lmms;
		auto taskOne = enqueueDummyTask();
		auto taskTwo = enqueueDummyTask();
		QCOMPARE(taskOne.get(), true);
		QCOMPARE(taskTwo.get(), true);
	}
};

QTEST_GUILESS_MAIN(ThreadPoolTest)
#include "ThreadPoolTest.moc"