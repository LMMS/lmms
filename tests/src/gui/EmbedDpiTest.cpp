/*
 * EmbedDpiTest.cpp
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

#include <QGuiApplication>
#include <QObject>
#include <QPixmap>
#include <QPixmapCache>
#include <QtTest>

#include "embed.h"

class EmbedDpiTest : public QObject
{
	Q_OBJECT
private slots:
	void testPixmapRequestProperties()
	{
		lmms::embed::PixmapRequest req;
		req.name = "test_icon";
		req.devicePixelRatio = 1.25;
		req.logicalSize = QSize(24, 24);

		QCOMPARE(req.devicePixelRatio, 1.25);
		QCOMPARE(req.logicalSize, QSize(24, 24));
		QCOMPARE(req.name, "test_icon");
	}

	void testCacheKeyDprSeparation()
	{
		// Different DPR requests produce distinct cache keys so 1.0, 1.25, 2.0 can coexist
		const auto key1 = QStringLiteral("%1_w%2_h%3_dpr%4")
			.arg("play")
			.arg(24)
			.arg(24)
			.arg(QString::number(1.0, 'f', 2));

		const auto key125 = QStringLiteral("%1_w%2_h%3_dpr%4")
			.arg("play")
			.arg(24)
			.arg(24)
			.arg(QString::number(1.25, 'f', 2));

		const auto key200 = QStringLiteral("%1_w%2_h%3_dpr%4")
			.arg("play")
			.arg(24)
			.arg(24)
			.arg(QString::number(2.0, 'f', 2));

		QVERIFY(key1 != key125);
		QVERIFY(key125 != key200);
		QVERIFY(key1 != key200);
		QCOMPARE(key125, QString("play_w24_h24_dpr1.25"));
	}

	void testEmptyOrInvalidNameHandling()
	{
		auto pm = lmms::embed::getIconPixmap("");
		QVERIFY(pm.isNull());

		auto pmDpr = lmms::embed::getIconPixmap("", 1.25);
		QVERIFY(pmDpr.isNull());
	}
};

QTEST_MAIN(EmbedDpiTest)
#include "EmbedDpiTest.moc"
