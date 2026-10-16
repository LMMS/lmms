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

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPixmapCache>
#include <QTemporaryDir>
#include <QtTest>
#include "DpiContext.h"
#include "embed.h"

using namespace lmms;
class EmbedDpiTest : public QObject
{
	Q_OBJECT
private slots:
	void resourceSizes_data()
	{
		QTest::addColumn<qreal>("dpr");
		for (qreal dpr : {1., 1.25, 1.5, 1.5625, 1.75, 2.})
		{
			QTest::newRow(qPrintable(QString::number(dpr))) << dpr;
		}
	}
	void resourceSizes()
	{
		QFETCH(qreal, dpr);
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto svgPath = directory.filePath("odd.svg");
		QFile svg(svgPath);
		QVERIFY(svg.open(QIODevice::WriteOnly));
		svg.write("<svg xmlns='http://www.w3.org/2000/svg' width='13' height='21'><rect width='13' height='21' fill='red'/></svg>");
		svg.close();
		const auto resource = embed::getIconResource(svgPath.toStdString(), {QSize(-1, -1), dpr});
		QVERIFY(!resource.pixmap.isNull());
		QCOMPARE(resource.logicalSize, QSize(13, 21));
		QCOMPARE(embed::logicalSize(resource.pixmap), QSize(13, 21));
		QCOMPARE(resource.pixmap.size(), gui::Dpi::rasterSizeFor(QSize(13, 21), dpr));
		QCOMPARE(resource.pixmap.devicePixelRatio(), dpr);
		QCOMPARE(resource.pixmap.toImage().pixelColor(resource.pixmap.width() - 1,
			resource.pixmap.height() - 1), QColor(Qt::red));
		const auto again = resource.atDpr(dpr);
		QCOMPARE(again.pixmap.cacheKey(), resource.pixmap.cacheKey());
		const auto other = resource.atDpr(dpr + 0.001);
		QVERIFY(other.pixmap.cacheKey() != resource.pixmap.cacheKey());
		QCOMPARE(other.logicalSize, resource.logicalSize);
		QCOMPARE(other.pixmap.devicePixelRatio(), dpr + 0.001);
	}
	void rasterVariantsAndThemeOrder()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QDir root(directory.path());
		QVERIFY(root.mkdir("selected"));
		QVERIFY(root.mkdir("fallback"));
		QImage base(24, 16, QImage::Format_ARGB32);
		base.fill(Qt::red);
		QVERIFY(base.save(directory.filePath("selected/icon.png")));
		QImage high(48, 32, QImage::Format_ARGB32);
		high.fill(Qt::blue);
		QVERIFY(high.save(directory.filePath("fallback/icon@2x.png")));
		const auto previous = QDir::searchPaths("dpitest");
		QDir::setSearchPaths("dpitest", {directory.filePath("selected"), directory.filePath("fallback")});
		const auto selected = embed::getIconResource("dpitest:icon", {QSize(-1, -1), 1.25});
		QCOMPARE(selected.logicalSize, QSize(24, 16));
		QCOMPARE(selected.pixmap.toImage().pixelColor(0, 0), QColor(Qt::red));
		QVERIFY(high.save(directory.filePath("selected/icon@2x.png")));
		const auto highSelected = embed::getIconResource("dpitest:icon", {QSize(-1, -1), 1.25});
		QCOMPARE(highSelected.logicalSize, QSize(24, 16));
		QCOMPARE(highSelected.pixmap.size(), QSize(30, 20));
		QCOMPARE(highSelected.pixmap.toImage().pixelColor(0, 0), QColor(Qt::blue));
		QFile vector(directory.filePath("selected/icon.svg"));
		QVERIFY(vector.open(QIODevice::WriteOnly));
		vector.write("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='16'><rect width='24' height='16' fill='green'/></svg>");
		vector.close();
		const QPixmap nativeIcon = embed::getIcon("dpitest:icon").pixmap(QSize(24, 16));
		QVERIFY(!nativeIcon.isNull());
		QCOMPARE(nativeIcon.toImage().pixelColor(0, 0), QColor("green"));
		QDir::setSearchPaths("dpitest", {directory.filePath("fallback")});
		base.fill(Qt::green);
		QVERIFY(base.save(directory.filePath("fallback/icon.png")));
		const auto fallback = embed::getIconResource("dpitest:icon", {QSize(-1, -1), 1.});
		QCOMPARE(fallback.pixmap.toImage().pixelColor(0, 0), QColor(Qt::green));
		QDir::setSearchPaths("dpitest", previous);
	}
	void xpmAndMissingResources()
	{
		static const char* const xpm[] = {"2 2 1 1", "x c #ff0000", "xx", "xx"};
		const auto resource = embed::getIconResource("test-xpm", {QSize(13, 21), 1.25}, xpm);
		QCOMPARE(resource.logicalSize, QSize(13, 21));
		QCOMPARE(resource.pixmap.size(), QSize(17, 27));
		QCOMPARE(resource.atDpr(2).logicalSize, resource.logicalSize);
		char color[] = "x c #ff0000";
		const char* temporaryXpm[] = {"2 2 1 1", color, "xx", "xx"};
		const auto owned = embed::getIconResource("temporary-xpm", {QSize(13, 21), 1.25}, temporaryXpm);
		color[5] = '0'; color[6] = '0'; color[9] = 'f'; color[10] = 'f';
		QCOMPARE(owned.atDpr(2).pixmap.toImage().pixelColor(0, 0), QColor(Qt::red));
		const auto renamed = embed::getIconResource("another-temporary-xpm", {QSize(13, 21), 1.25}, temporaryXpm);
		QCOMPARE(renamed.pixmap.toImage().pixelColor(0, 0), QColor(Qt::blue));
		QTemporaryDir directory;
		QFile invalid(directory.filePath("invalid.svg"));
		QVERIFY(invalid.open(QIODevice::WriteOnly));
		invalid.write("<svg invalid");
		invalid.close();
		const auto badSvg = embed::getIconResource(invalid.fileName().toStdString(), {QSize(13, 21), 1.25});
		QCOMPARE(badSvg.pixmap.size(), QSize(1, 1));
		QCOMPARE(badSvg.pixmap.toImage().pixelColor(0, 0).alpha(), 0);
		QVERIFY(embed::getIconPixmap("").isNull());
		const auto missing = embed::getIconPixmap("does-not-exist-dpi-test", 1.25);
		QCOMPARE(missing.size(), QSize(1, 1));
		QCOMPARE(missing.toImage().pixelColor(0, 0).alpha(), 0);
	}
};
#include "DpiTestMain.h"
LMMS_DPI_TEST_MAIN(EmbedDpiTest)
#include "EmbedDpiTest.moc"
