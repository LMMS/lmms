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

#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QImage>
#include <QtTest>
#include "DpiContext.h"
#include "Engine.h"
#include "Graph.h"
#include "EnvelopeGraph.h"
#include "PianoView.h"
#include "PixmapButton.h"
#include "LcdWidget.h"
#include "Knob.h"
#include "embed.h"

using namespace lmms;
using namespace lmms::gui;
class DpiWidgetTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase() { Engine::init(true); }
	void cleanupTestCase() { Engine::destroy(); }
	void buttonRetainsLogicalSize()
	{
		QTemporaryDir directory;
		QFile file(directory.filePath("odd.svg"));
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write("<svg xmlns='http://www.w3.org/2000/svg' width='13' height='21'><rect width='13' height='21' fill='red'/></svg>");
		file.close();
		QWidget window;
		Graph graph(&window, Graph::Style::Linear, 24, 24);
		graph.move(30, 0);
		graph.setForeground(embed::getIconPixmap(file.fileName().toStdString(), 1.25));
		PixmapButton button(&window);
		button.setActiveGraphic(embed::getIconPixmap(file.fileName().toStdString(), 1.25));
		button.setInactiveGraphic(embed::getIconPixmap(file.fileName().toStdString(), 2.0));
		QCOMPARE(button.sizeHint(), QSize(13, 21));
		window.show();
		QCoreApplication::processEvents();
		QCOMPARE(button.sizeHint(), QSize(13, 21));
		QSignalSpy clicks(&button, &PixmapButton::doubleClicked);
		QTest::mouseDClick(&button, Qt::LeftButton, Qt::NoModifier, QPoint(6, 10));
		QCOMPARE(clicks.count(), 1);
		QVERIFY(!button.grab().isNull());
		QCOMPARE(graph.size(), QSize(24, 24));
		const qreal dpr = Dpi::effectiveDevicePixelRatio(&graph);
		QCOMPARE(graph.grab().toImage().pixelColor(qRound(5 * dpr), qRound(5 * dpr)), QColor(Qt::red));
	}
	void idleKnobStyleInvalidatesCache()
	{
		QWidget window;
		FloatModel model(0.5f, 0.f, 1.f, 0.01f);
		Knob knob(KnobType::Styled, &window);
		knob.setModel(&model);
		knob.setFixedSize(32, 32);
		knob.setCenterPointX(16);
		knob.setCenterPointY(16);
		knob.setOuterRadius(12);
		knob.setInnerRadius(2);
		knob.setLineWidth(3);
		knob.setOuterColor(Qt::red);
		window.show();
		QCoreApplication::processEvents();
		const QImage before = knob.grab().toImage();
		knob.setOuterColor(Qt::blue);
		QEvent styleChange(QEvent::StyleChange);
		QCoreApplication::sendEvent(&knob, &styleChange);
		QCoreApplication::processEvents();
		const QImage after = knob.grab().toImage();
		QCOMPARE(model.value(), 0.5f);
		QCOMPARE(after.size(), before.size());
		QVERIFY(after != before);
	}
	void lcdSpriteAndLogicalGeometry()
	{
		QTemporaryDir directory;
		QImage sprite(36, 10, QImage::Format_ARGB32);
		sprite.fill(Qt::red);
		{
			QPainter painter(&sprite);
			painter.fillRect(0, 5, 36, 5, Qt::blue);
		}
		QVERIFY(sprite.save(directory.filePath("lcd_dpitest.png")));
		const auto previous = QDir::searchPaths("artwork");
		QDir::setSearchPaths("artwork", {directory.path()});
		{
			QWidget window;
			LcdWidget lcd(1, "dpitest", &window);
			lcd.setValue(5);
			window.show();
			QCoreApplication::processEvents();
			QCOMPARE(lcd.cellHeight(), 5);
			const QSize logical = lcd.size();
			const QImage enabled = lcd.grab().toImage();
			lcd.setEnabled(false);
			QCoreApplication::processEvents();
			QCOMPARE(lcd.size(), logical);
			QVERIFY(lcd.grab().toImage() != enabled);
		}
		QDir::setSearchPaths("artwork", previous);
	}

	void pianoAndEnvelopeResources()
	{
		QTemporaryDir directory;
		for (const char* name : {"white_key", "white_key_pressed", "white_key_disabled", "envelope_graph"})
		{
			QImage image(10, 57, QImage::Format_ARGB32);
			image.fill(Qt::blue);
			QVERIFY(image.save(directory.filePath(QString::fromLatin1(name) + ".png")));
		}
		for (const char* name : {"black_key", "black_key_pressed", "black_key_disabled"})
		{
			QImage image(8, 38, QImage::Format_ARGB32);
			image.fill(Qt::black);
			QVERIFY(image.save(directory.filePath(QString::fromLatin1(name) + ".png")));
		}
		const auto previous = QDir::searchPaths("artwork");
		QDir::setSearchPaths("artwork", {directory.path()});
		{
			QWidget window;
			PianoView piano(&window);
			piano.setFocusPolicy(Qt::NoFocus); // Rendering fixture has no instrument model.
			piano.resize(80, 100);
			EnvelopeGraph envelope(&window);
			envelope.move(100, 0);
			envelope.resize(24, 64);
			window.show();
			QCoreApplication::processEvents();
			const QSize pianoSize = piano.size();
			const qreal dpr = Dpi::effectiveDevicePixelRatio(&piano);
			QCOMPARE(piano.grab().toImage().pixelColor(qRound(4 * dpr), qRound(61 * dpr)), QColor(Qt::blue));
			QCOMPARE(envelope.grab().toImage().pixelColor(qRound(4 * dpr), qRound(4 * dpr)), QColor(Qt::blue));
			QEvent paletteChange(QEvent::PaletteChange);
			QCoreApplication::sendEvent(&piano, &paletteChange);
			QCoreApplication::processEvents();
			QCOMPARE(piano.size(), pianoSize);
		}
		QDir::setSearchPaths("artwork", previous);
	}

};
#include "DpiTestMain.h"
LMMS_DPI_TEST_MAIN(DpiWidgetTest)
#include "DpiWidgetTest.moc"
