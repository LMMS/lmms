/*
 * WindowGeometryTest.cpp
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

#include <algorithm>
#include <QList>
#include <QObject>
#include <QRect>
#include <QtTest>

namespace {

QRect clampGeometryToScreens(QRect geom, const QList<QRect>& screenGeoms, const QRect& primaryGeom)
{
	// Minimum sensible dimensions
	if (geom.width() < 50) { geom.setWidth(300); }
	if (geom.height() < 50) { geom.setHeight(200); }

	bool onScreen = false;
	for (const auto& screen : screenGeoms)
	{
		if (screen.intersects(geom))
		{
			onScreen = true;
			break;
		}
	}

	if (!onScreen)
	{
		geom.moveTo(
			std::clamp(geom.x(), primaryGeom.left(), std::max(primaryGeom.left(), primaryGeom.right() - geom.width())),
			std::clamp(geom.y(), primaryGeom.top(), std::max(primaryGeom.top(), primaryGeom.bottom() - geom.height()))
		);
	}
	return geom;
}

QRect clampSubWindowToMdi(QRect geom, const QRect& mdiRect)
{
	if (geom.width() < 50) { geom.setWidth(300); }
	if (geom.height() < 50) { geom.setHeight(200); }

	if (geom.x() < 0) { geom.moveLeft(0); }
	if (geom.y() < 0) { geom.moveTop(0); }

	if (mdiRect.width() > 100 && mdiRect.height() > 100)
	{
		if (geom.x() >= mdiRect.width())
		{
			geom.moveLeft(std::max(0, mdiRect.width() - geom.width()));
		}
		if (geom.y() >= mdiRect.height())
		{
			geom.moveTop(std::max(0, mdiRect.height() - geom.height()));
		}
	}
	return geom;
}

} // namespace

class WindowGeometryTest : public QObject
{
	Q_OBJECT
private slots:
	void testNormalWindowWithinBounds()
	{
		const QRect screen(0, 0, 1920, 1080);
		const QRect win(100, 100, 800, 600);

		const QRect result = clampGeometryToScreens(win, {screen}, screen);
		QCOMPARE(result, win);
	}

	void testOffScreenDisconnectedMonitor()
	{
		// Old project saved on disconnected 4K screen at (2560, 100)
		const QRect primary(0, 0, 1920, 1080);
		const QRect win(2560, 100, 800, 600);

		const QRect result = clampGeometryToScreens(win, {primary}, primary);
		// Clamped onto primary screen
		QVERIFY(primary.contains(result));
		QCOMPARE(result.size(), win.size());
	}

	void testNegativeOffScreenCoordinates()
	{
		const QRect primary(0, 0, 1920, 1080);
		const QRect win(-800, -600, 500, 400);

		const QRect result = clampGeometryToScreens(win, {primary}, primary);
		QVERIFY(primary.contains(result));
	}

	void testSubWindowNegativeAndOverflowClamping()
	{
		const QRect mdi(0, 0, 1600, 900);

		// Negative coordinates clamped to 0
		const QRect negWin(-100, -50, 400, 300);
		const QRect negResult = clampSubWindowToMdi(negWin, mdi);
		QCOMPARE(negResult.topLeft(), QPoint(0, 0));

		// Far right overflow clamped to inside MDI area
		const QRect farWin(2000, 1500, 400, 300);
		const QRect farResult = clampSubWindowToMdi(farWin, mdi);
		QCOMPARE(farResult.right() <= mdi.width(), true);
		QCOMPARE(farResult.bottom() <= mdi.height(), true);
	}

	void testMinimumWindowSizeEnforcement()
	{
		const QRect screen(0, 0, 1920, 1080);
		const QRect tinyWin(50, 50, 10, 5);

		const QRect result = clampGeometryToScreens(tinyWin, {screen}, screen);
		QCOMPARE(result.width() >= 50, true);
		QCOMPARE(result.height() >= 50, true);
	}
};

QTEST_GUILESS_MAIN(WindowGeometryTest)
#include "WindowGeometryTest.moc"
