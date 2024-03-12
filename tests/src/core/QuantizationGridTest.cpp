/*
 * QuantizationGridTest.cpp
 *
 * Copyright (c) 2024 Dominic Clark
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
 */

#include "QuantizationGrid.h"

#include <QObject>
#include <QtTest/QtTest>

class QuantizationGridTest : public QObject
{
	Q_OBJECT

private slots:
	void testGetSetInterval()
	{
		auto grid = lmms::QuantizationGrid{};

		// Test that getter intially returns default value
		QCOMPARE(grid.interval(), 1);

		// Test that getter returns new value after calling setter
		grid.setInterval(42);
		QCOMPARE(grid.interval(), 42);
	}

	void testGetSetOffset()
	{
		auto grid = lmms::QuantizationGrid{};

		// Test that getter intially returns default value
		QCOMPARE(grid.offset(), lmms::TimePos{0});

		// Test that getter returns new value after calling setter
		grid.setOffset(lmms::TimePos{42});
		QCOMPARE(grid.offset(), lmms::TimePos{42});
	}

	void testQuantizePoint()
	{
		const auto grid = lmms::QuantizationGrid{20, lmms::TimePos{-15}};

		// Test rounding down
		QCOMPARE(grid.quantize(lmms::TimePos{-28}), lmms::TimePos{-35}); // Below offset
		QCOMPARE(grid.quantize(lmms::TimePos{-9}), lmms::TimePos{-15}); // To offset
		QCOMPARE(grid.quantize(lmms::TimePos{11}), lmms::TimePos{5}); // Above offset

		// Test rounding up
		QCOMPARE(grid.quantize(lmms::TimePos{-37}), lmms::TimePos{-35}); // Below offset
		QCOMPARE(grid.quantize(lmms::TimePos{-20}), lmms::TimePos{-15}); // To offset
		QCOMPARE(grid.quantize(lmms::TimePos{4}), lmms::TimePos{5}); // Above offset

		// Test rounding midpoint up
		QCOMPARE(grid.quantize(lmms::TimePos{-25}), lmms::TimePos{-15}); // Negative to negative
		QCOMPARE(grid.quantize(lmms::TimePos{-5}), lmms::TimePos{5}); // Negative to positive
		QCOMPARE(grid.quantize(lmms::TimePos{15}), lmms::TimePos{25}); // Positive to positive

		// Test quantizing exact values
		QCOMPARE(grid.quantize(lmms::TimePos{-35}), lmms::TimePos{-35}); // Below offset
		QCOMPARE(grid.quantize(lmms::TimePos{-15}), lmms::TimePos{-15}); // At offset
		QCOMPARE(grid.quantize(lmms::TimePos{5}), lmms::TimePos{5}); // Above offset
	}

	void testQuantizeDuration()
	{
		const auto grid = lmms::QuantizationGrid{20, lmms::TimePos{-15}};

		// Test rounding down
		QCOMPARE(grid.quantizeDuration(-18), -20); // Negative
		QCOMPARE(grid.quantizeDuration(7), 0); // To Zero
		QCOMPARE(grid.quantizeDuration(29), 20); // Positive

		// Test rounding up
		QCOMPARE(grid.quantizeDuration(-21), -20); // Negative
		QCOMPARE(grid.quantizeDuration(-4), 0); // To Zero
		QCOMPARE(grid.quantizeDuration(15), 20); // Positive

		// Test rounding midpoint up
		QCOMPARE(grid.quantizeDuration(-30), -20); // Negative to negative
		QCOMPARE(grid.quantizeDuration(-10), 0); // Negative to zero
		QCOMPARE(grid.quantizeDuration(30), 40); // Positive to positive

		// Test quantizing exact values
		QCOMPARE(grid.quantizeDuration(-20), -20); // Negative
		QCOMPARE(grid.quantizeDuration(0), 0); // Zero
		QCOMPARE(grid.quantizeDuration(20), 20); // Positive
	}
};

QTEST_GUILESS_MAIN(QuantizationGridTest)

#include "QuantizationGridTest.moc"
