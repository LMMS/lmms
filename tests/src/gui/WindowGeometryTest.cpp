/*
 * DpiRasterTest.cpp
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
#include "DpiContext.h"

using lmms::gui::Dpi::reachableGeometry;
class WindowGeometryTest : public QObject
{
	Q_OBJECT
private slots:
	void screenEdgesAndGaps()
	{
		const QRect primary(0, 0, 1920, 1080);
		const QRect secondary(-1600, 100, 1280, 800);
		QCOMPARE(reachableGeometry(QRect(100, 100, 800, 600), {primary, secondary}), QRect(100, 100, 800, 600));
		for (const QRect geometry : {QRect(1919, -590, 800, 600), QRect(2560, 100, 800, 600),
			QRect(-300, 100, 200, 200), QRect(-1800, -900, 200, 200), QRect(0, 0, 8000, 5000)})
		{
			const QRect result = reachableGeometry(geometry, {primary, secondary});
			QVERIFY(primary.contains(result) || secondary.contains(result));
			QCOMPARE(reachableGeometry(result, {primary, secondary}), result);
		}
	}
	void minimumAndMdi()
	{
		const QRect mdi(0, 0, 400, 300);
		QCOMPARE(reachableGeometry(QRect(-20, -30, 100, 100), {mdi}).topLeft(), QPoint(0, 0));
		QCOMPARE(reachableGeometry(QRect(399, 299, 100, 100), {mdi}), QRect(300, 200, 100, 100));
		const QRect oversized = reachableGeometry(QRect(2000, 2000, 800, 600), {mdi}, QSize(500, 400));
		QCOMPARE(oversized.topLeft(), QPoint(0, 0));
		QCOMPARE(oversized.size(), QSize(500, 400));
		QCOMPARE(reachableGeometry(QRect(10, 10, 0, 0), {mdi}).size(), QSize(300, 200));
		QCOMPARE(reachableGeometry(QRect(-20, 10, 30, 40), {}), QRect(-20, 10, 30, 40));
	}
};
QTEST_GUILESS_MAIN(WindowGeometryTest)
#include "WindowGeometryTest.moc"
