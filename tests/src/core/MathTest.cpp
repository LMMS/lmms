/*
 * MathTest.cpp
 *
 * Copyright (c) 2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include <QObject>
#include <QtTest>

#include "lmms_math.h"

class MathTest : public QObject
{
	Q_OBJECT
private slots:
	void NumDigitsTest()
	{
		using namespace lmms;
		QCOMPARE(numDigitsAsInt(1.f), 1);
		QCOMPARE(numDigitsAsInt(9.9f), 2);
		QCOMPARE(numDigitsAsInt(10.f), 2);
		QCOMPARE(numDigitsAsInt(0.f), 1);
		QCOMPARE(numDigitsAsInt(-100.f), 4);
		QCOMPARE(numDigitsAsInt(-99.f), 3);
		QCOMPARE(numDigitsAsInt(-0.4f), 1); // there is no "-0" for LED spinbox
		QCOMPARE(numDigitsAsInt(-0.99f), 2);
		QCOMPARE(numDigitsAsInt(1000000000), 10);
		QCOMPARE(numDigitsAsInt(-1000000000), 11);
		QCOMPARE(numDigitsAsInt(900000000), 9);
		QCOMPARE(numDigitsAsInt(-900000000), 10);
	}
};

QTEST_GUILESS_MAIN(MathTest)
#include "MathTest.moc"
