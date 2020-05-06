/*
 * MemoryPoolTest.cpp
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
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

#include "QTestSuite.h"

#include "MemoryPool.h"

#include <array>
#include <stack>

class MemoryPoolTest : QTestSuite
{
	Q_OBJECT
private slots:
	void MemoryPoolTests()
	{
		using T = std::array<char, 16>;
		int n = 256;
		MemoryPool<T> pool(n);

		std::stack<T*> ptrs;

		for (int i=0; i < n; i++) {
			ptrs.push(pool.allocate_bounded());
			QVERIFY(ptrs.top());
		}
		QCOMPARE(pool.allocate_bounded(), static_cast<T*>(nullptr));
		ptrs.push(pool.allocate());
		QVERIFY(ptrs.top());

		while (!ptrs.empty()) {
			pool.deallocate(ptrs.top());
			ptrs.pop();
		}
	}
} MemoryPoolTests;

#include "MemoryPoolTest.moc"
