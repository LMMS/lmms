/*
 * SampleCacheTest.cpp
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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

#include "SampleCache.h"

#include "QTestSuite.h"
#include "SampleBuffer.h"

class SampleCacheTest : QTestSuite
{
	Q_OBJECT
private slots:
	void canAddEntry()
	{
		using namespace lmms;

		auto cache = SampleCache{};
		auto buffer = std::make_shared<const SampleBuffer>();
		cache.add("test", buffer);

		QVERIFY(cache.contains("test"));
	}

    void cannotReplaceExistingEntry()
    {
        using namespace lmms;

        auto cache = SampleCache{};
        auto bufferOne = std::make_shared<const SampleBuffer>();
        auto bufferTwo = std::make_shared<const SampleBuffer>();

        cache.add("test", bufferOne);
        cache.add("test", bufferTwo);

        QCOMPARE(*cache.get("test"), bufferOne);
    }

	void canGetEntry()
	{
		using namespace lmms;

		auto cache = SampleCache{};
		auto buffer = std::make_shared<const SampleBuffer>();
		cache.add("test", buffer);

		QCOMPARE(*cache.get("test"), buffer);
	}

    void cannotGetNonExistingEntry()
    {
        using namespace lmms;

        auto cache = SampleCache{};
        QVERIFY(!cache.get("test").has_value());
    }

	void canRemoveEntry()
	{
		using namespace lmms;

		auto cache = SampleCache{};
		auto buffer = std::make_shared<const SampleBuffer>();

		cache.add("test", buffer);
		cache.remove("test");

		QVERIFY(!cache.contains("test"));
	}

} SampleCacheTests;

#include "SampleCacheTest.moc"
