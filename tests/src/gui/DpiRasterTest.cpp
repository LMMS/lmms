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

#include <limits>
#include <QGuiApplication>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QtTest>

#include "DpiContext.h"
#include "embed.h"

using namespace lmms::gui;

class DpiRasterTest : public QObject
{
	Q_OBJECT
private slots:
	void testRasterSizeStandardScales()
	{
		const QSize logical(100, 32);

		// 100% -> 100 x 32
		QCOMPARE(Dpi::rasterSizeFor(logical, 1.0), QSize(100, 32));

		// 125% -> ceil(100 * 1.25) = 125, ceil(32 * 1.25) = 40
		QCOMPARE(Dpi::rasterSizeFor(logical, 1.25), QSize(125, 40));

		// 150% -> ceil(100 * 1.50) = 150, ceil(32 * 1.50) = 48
		QCOMPARE(Dpi::rasterSizeFor(logical, 1.5), QSize(150, 48));

		// 175% -> ceil(100 * 1.75) = 175, ceil(32 * 1.75) = 56
		QCOMPARE(Dpi::rasterSizeFor(logical, 1.75), QSize(175, 56));

		// 200% -> ceil(100 * 2.0) = 200, ceil(32 * 2.0) = 64
		QCOMPARE(Dpi::rasterSizeFor(logical, 2.0), QSize(200, 64));
	}

	void testRasterSizeOddDimensions()
	{
		const QSize oddSize(21, 13);

		// 1.25 -> ceil(21 * 1.25) = ceil(26.25) = 27, ceil(13 * 1.25) = ceil(16.25) = 17
		QCOMPARE(Dpi::rasterSizeFor(oddSize, 1.25), QSize(27, 17));

		// 1.50 -> ceil(21 * 1.50) = ceil(31.5) = 32, ceil(13 * 1.50) = ceil(19.5) = 20
		QCOMPARE(Dpi::rasterSizeFor(oddSize, 1.5), QSize(32, 20));

		// 1.75 -> ceil(21 * 1.75) = ceil(36.75) = 37, ceil(13 * 1.75) = ceil(22.75) = 23
		QCOMPARE(Dpi::rasterSizeFor(oddSize, 1.75), QSize(37, 23));
	}

	void testRasterSizeEdgeCases()
	{
		// Invalid / zero dimensions
		QCOMPARE(Dpi::rasterSizeFor(QSize(0, 0), 1.5), QSize(0, 0));
		QCOMPARE(Dpi::rasterSizeFor(QSize(-10, -5), 1.5), QSize(0, 0));

		QCOMPARE(Dpi::rasterSizeFor(QSize(100, 32), 1.5625), QSize(157, 50));
		QCOMPARE(Dpi::rasterSizeFor(QSizeF(std::numeric_limits<qreal>::infinity(), 10), 1), QSize(0, 0));
		QCOMPARE(Dpi::rasterSizeFor(QSize(100, 10), std::numeric_limits<qreal>::infinity()), QSize(100, 10));
		QCOMPARE(Dpi::rasterSizeFor(QSize(100, 10), std::numeric_limits<qreal>::quiet_NaN()), QSize(100, 10));
		QCOMPARE(Dpi::rasterSizeFor(QSize(std::numeric_limits<int>::max(), 10), 2), QSize(0, 0));
		// DPR <= 0 defaults to 1.0
		QCOMPARE(Dpi::rasterSizeFor(QSize(100, 50), 0.0), QSize(100, 50));
		QCOMPARE(Dpi::rasterSizeFor(QSize(100, 50), -1.25), QSize(100, 50));
	}

	void testLogicalSizeQImage()
	{
		// Physical 200x100 image with DPR 2.0 -> logical size 100x50
		QImage img(200, 100, QImage::Format_ARGB32);
		img.setDevicePixelRatio(2.0);
		QCOMPARE(lmms::embed::logicalSize(img), QSize(100, 50));

		// Physical 125x100 image with DPR 1.25 -> logical size 100x80
		QImage img125(125, 100, QImage::Format_ARGB32);
		img125.setDevicePixelRatio(1.25);
		QCOMPARE(lmms::embed::logicalSize(img125), QSize(100, 80));

		// Null image
		QImage nullImg;
		QCOMPARE(lmms::embed::logicalSize(nullImg), QSize(0, 0));
	}

	void testLogicalSizeQPixmap()
	{
		QPixmap pm(150, 90);
		pm.setDevicePixelRatio(1.5);
		QCOMPARE(lmms::embed::logicalSize(pm), QSize(100, 60));

		QPixmap nullPm;
		QCOMPARE(lmms::embed::logicalSize(nullPm), QSize(0, 0));
	}
};

#include "DpiTestMain.h"
LMMS_DPI_TEST_MAIN(DpiRasterTest)
#include "DpiRasterTest.moc"
