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

#include <QSignalSpy>
#include <QtTest>
#include "DpiContext.h"

using namespace lmms::gui;
class DpiCacheTest : public QObject
{
	Q_OBJECT
private slots:
	void contextOwnershipAndNotifications()
	{
		QWidget first;
		QWidget second;
		auto* child = new QWidget(&first);
		auto* context = DpiContext::forWidget(child);
		QCOMPARE(context, DpiContext::forWidget(&first));
		QVERIFY(context != DpiContext::forWidget(&second));
		int refreshes = 0;
		Dpi::watch(child, [&] { ++refreshes; });
		QTRY_COMPARE(refreshes, 1);
		// A production context notification must refresh registered idle consumers.
		context->dpiChanged(1.25, 1.0);
		context->dpiChanged(1.5, 1.25);
		QTRY_COMPARE(refreshes, 2);
		child->setParent(&second);
		Dpi::refresh(child);
		QTRY_COMPARE(refreshes, 3);
		const int before = refreshes;
		context->dpiChanged(2, 1.5);
		QCoreApplication::processEvents();
		QCOMPARE(refreshes, before);
		DpiContext::forWidget(&second)->dpiChanged(2, 1);
		QTRY_COMPARE(refreshes, before + 1);
		QPointer<QWidget> guard(child);
		delete child;
		QVERIFY(guard.isNull());
		DpiContext::forWidget(&second)->dpiChanged(1, 2);
		QCoreApplication::processEvents();
	}
	void shownWindowIsReachable()
	{
		QWidget window;
		window.resize(100, 100);
		window.move(100000, 100000);
		window.show();
		DpiContext::forWidget(&window);
		QTRY_VERIFY(window.screen()->availableGeometry().contains(window.frameGeometry()));
	}

	void unchangedMetricsDoNotNotify()
	{
		QWidget window;
		window.show();
		auto* context = DpiContext::forWidget(&window);
		QCoreApplication::processEvents();
		QSignalSpy changes(context, &DpiContext::dpiChanged);
		context->checkDpiChange();
		context->checkDpiChange();
		QCOMPARE(changes.count(), 0);
		QCOMPARE(context->devicePixelRatio(), Dpi::effectiveDevicePixelRatio(&window));
		if (QGuiApplication::platformName() == "offscreen" && qEnvironmentVariableIsSet("QT_SCALE_FACTOR"))
		{
			QCOMPARE(context->devicePixelRatio(), qEnvironmentVariable("QT_SCALE_FACTOR").toDouble());
		}
		QPointer<DpiContext> guard(context);
		window.setAttribute(Qt::WA_NativeWindow);
		window.setWindowFlags(Qt::Tool);
		window.hide();
		window.show();
		context->checkDpiChange();
		QVERIFY(guard);
	}
};
#include "DpiTestMain.h"
LMMS_DPI_TEST_MAIN(DpiCacheTest)
#include "DpiCacheTest.moc"
