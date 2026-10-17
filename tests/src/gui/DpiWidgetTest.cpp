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
#include <QLabel>
#include <QtTest>
#include "DpiContext.h"
#include "Engine.h"
#include "Graph.h"
#include "EnvelopeGraph.h"
#include "PianoView.h"
#include "PixmapButton.h"
#include "LcdWidget.h"
#include "LcdFloatSpinBox.h"
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
	void lcdSpriteAndLogicalGeometry_data()
	{
		QTest::addColumn<int>("cellWidth");
		QTest::newRow("small") << 6;
		QTest::newRow("toolbar") << 11;
		QTest::newRow("wide") << 13;
	}
	void lcdSpriteAndLogicalGeometry()
	{
		QFETCH(int, cellWidth);
		QTemporaryDir directory;
		constexpr int cellHeight = 9;
		// Real themes contain 0..9, a blank, a minus, and a separate margin cell.
		QImage sprite(13 * cellWidth, 2 * cellHeight, QImage::Format_ARGB32);
		auto color = [](int cell, bool enabled) {
			return QColor::fromRgba(QColor::fromHsv(cell * 27, 255, enabled ? 255 : 128).rgba());
		};
		{
			QPainter painter(&sprite);
			for (int cell = 0; cell < 13; ++cell)
			{
				painter.fillRect(cell * cellWidth, 0, cellWidth, cellHeight, color(cell, true));
				painter.fillRect(cell * cellWidth, cellHeight, cellWidth, cellHeight, color(cell, false));
			}
		}
		QVERIFY(sprite.save(directory.filePath("lcd_dpitest.png")));
		const auto previous = QDir::searchPaths("artwork");
		QDir::setSearchPaths("artwork", {directory.path()});
		{
			QWidget window;
			LcdWidget lcd(1, "dpitest", &window);
			lcd.setSeamless(true, true);
			lcd.addTextForValue(10, " ");
			lcd.addTextForValue(11, "-");
			window.show();
			QCoreApplication::processEvents();
			QCOMPARE(lcd.cellHeight(), cellHeight);
			QCOMPARE(lcd.size(), QSize(cellWidth, cellHeight + 2));
			const qreal dpr = Dpi::effectiveDevicePixelRatio(&lcd);
			for (bool enabled : {true, false})
			{
				lcd.setEnabled(enabled);
				QCoreApplication::processEvents();
				for (int cell = 0; cell < 12; ++cell)
				{
					lcd.setValue(cell);
					const QImage rendered = lcd.grab().toImage();
					for (int x : {1, cellWidth - 2})
					{
						QCOMPARE(rendered.pixelColor(qFloor((x + 0.5) * dpr), qFloor(5.5 * dpr)), color(cell, enabled));
					}
				}
			}
			lcd.setSeamless(false, false);
			lcd.setValue(9);
			const QImage withMargins = lcd.grab().toImage();
			QCOMPARE(withMargins.pixelColor(qFloor(2.5 * dpr), qFloor(5.5 * dpr)), color(12, false));
		}
		QDir::setSearchPaths("artwork", previous);
	}

	void lcdDecimalPointEnabledState()
	{
		QTemporaryDir directory;
		QImage sprite(39, 10, QImage::Format_ARGB32);
		sprite.fill(Qt::green);
		QVERIFY(sprite.save(directory.filePath("lcd_dpitest.png")));
		QImage dot(4, 10, QImage::Format_ARGB32);
		{
			QPainter painter(&dot);
			painter.fillRect(0, 0, 4, 5, Qt::red);
			painter.fillRect(0, 5, 4, 5, Qt::blue);
		}
		QVERIFY(dot.save(directory.filePath("lcd_dpitest_dot.png")));
		const auto previous = QDir::searchPaths("artwork");
		QDir::setSearchPaths("artwork", {directory.path()});
		{
			QWidget window;
			FloatModel model(129.75f, 0.f, 999.f, 0.01f);
			LcdFloatSpinBox lcd(3, 2, "dpitest", "", &window);
			lcd.setModel(&model);
			window.show();
			QCoreApplication::processEvents();
			auto* label = lcd.findChild<QLabel*>();
			QVERIFY(label);
			QCOMPARE(label->size(), QSize(4, 5));
			const qreal dpr = Dpi::effectiveDevicePixelRatio(label);
			QCOMPARE(label->grab().toImage().pixelColor(qFloor(1.5 * dpr), qFloor(2.5 * dpr)), QColor(Qt::red));
			lcd.setEnabled(false);
			QCoreApplication::processEvents();
			// QLabel styles may gray disabled pixmaps; check the selected source row.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			const QPixmap disabledDot = label->pixmap();
#else
			const QPixmap disabledDot = label->pixmap(Qt::ReturnByValue);
#endif
			QCOMPARE(disabledDot.toImage().pixelColor(qFloor(1.5 * dpr), qFloor(2.5 * dpr)), QColor(Qt::blue));
			QCOMPARE(model.value(), 129.75f);
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
