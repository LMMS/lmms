/*
 * DpiCacheTest.cpp
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
#include <QPixmap>
#include <QSize>
#include <QtTest>

#include "DpiContext.h"

// Simulated cache item representing Knob or Clip cached raster pixmap state
struct WidgetRasterCache
{
	QPixmap pixmap;
	qreal cachedDpr = 1.0;
	QSize cachedSize;
	float modelValue = 0.0f;

	bool isValid(qreal currentDpr, const QSize& currentSize, float currentValue) const
	{
		if (pixmap.isNull()) { return false; }
		if (qAbs(cachedDpr - currentDpr) > 0.001) { return false; }
		if (cachedSize != currentSize) { return false; }
		if (qAbs(modelValue - currentValue) > 0.0001f) { return false; }
		return true;
	}

	void update(qreal currentDpr, const QSize& currentSize, float currentValue)
	{
		const QSize rasterSize = lmms::gui::Dpi::rasterSizeFor(currentSize, currentDpr);
		pixmap = QPixmap(rasterSize);
		pixmap.setDevicePixelRatio(currentDpr);
		cachedDpr = currentDpr;
		cachedSize = currentSize;
		modelValue = currentValue;
	}
};

class DpiCacheTest : public QObject
{
	Q_OBJECT
private slots:
	void testCacheValidationOnDprChange()
	{
		WidgetRasterCache cache;
		const QSize logicalSize(32, 32);
		const float value = 0.75f;

		// Initial render at DPR 1.0
		cache.update(1.0, logicalSize, value);
		QVERIFY(cache.isValid(1.0, logicalSize, value));

		// Moving to monitor with DPR 1.25 WITHOUT model value change:
		// Previously, code checking only modelValue would skip redraw.
		// With DPI awareness, cachedDpr != currentDpr invalidates cache!
		QVERIFY(!cache.isValid(1.25, logicalSize, value));

		// Update for new DPR 1.25
		cache.update(1.25, logicalSize, value);
		QVERIFY(cache.isValid(1.25, logicalSize, value));

		// Same DPR and value reuses cache
		QVERIFY(cache.isValid(1.25, logicalSize, value));

		// Moving to 1.50 invalidates
		QVERIFY(!cache.isValid(1.50, logicalSize, value));
	}

	void testCacheValidationOnSizeChange()
	{
		WidgetRasterCache cache;
		cache.update(1.25, QSize(32, 32), 0.5f);

		// Size change at same DPR invalidates
		QVERIFY(!cache.isValid(1.25, QSize(48, 48), 0.5f));
	}

	void testCacheValidationOnValueChange()
	{
		WidgetRasterCache cache;
		cache.update(1.25, QSize(32, 32), 0.5f);

		// Value change at same DPR and size invalidates
		QVERIFY(!cache.isValid(1.25, QSize(32, 32), 0.8f));
	}
};

QTEST_MAIN(DpiCacheTest)
#include "DpiCacheTest.moc"
