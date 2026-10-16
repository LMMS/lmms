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
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class DpiNativeTest : public QObject
{
	Q_OBJECT
private slots:
	void hiddenNativeWindow()
	{
#ifdef Q_OS_WIN
		if (QGuiApplication::platformName() != "windows") { QSKIP("Requires the native Windows platform plugin"); }
		using GetContext = HANDLE (WINAPI*)(HWND);
		using GetAwareness = int (WINAPI*)(HANDLE);
		using GetDpi = UINT (WINAPI*)(HWND);
		using ContextsEqual = BOOL (WINAPI*)(HANDLE, HANDLE);
		const HMODULE user32 = GetModuleHandleW(L"user32.dll");
		const auto context = reinterpret_cast<GetContext>(GetProcAddress(user32, "GetWindowDpiAwarenessContext"));
		const auto awareness = reinterpret_cast<GetAwareness>(GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext"));
		const auto dpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow"));
		const auto equal = reinterpret_cast<ContextsEqual>(GetProcAddress(user32, "AreDpiAwarenessContextsEqual"));
		if (!context || !awareness || !dpi || !equal) { QSKIP("Windows DPI query APIs unavailable"); }
		// A frameless hidden window avoids native caption minimum sizes. Place it
		// explicitly on the primary screen instead of relying on Windows' placement.
		QWidget widget(nullptr, Qt::Tool | Qt::FramelessWindowHint);
		widget.setGeometry(QRect(QGuiApplication::primaryScreen()->availableGeometry().center(), QSize(100, 32)));
		const HWND handle = reinterpret_cast<HWND>(widget.winId());
		QCoreApplication::processEvents();
		QVERIFY(!widget.isVisible());
		QCOMPARE(awareness(context(handle)), 2);
		// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is the documented pseudo-handle -4.
		QVERIFY(equal(context(handle), reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4))));
		const qreal nativeDpr = dpi(handle) / 96.0;
		QCOMPARE(lmms::gui::Dpi::effectiveDevicePixelRatio(&widget), nativeDpr);
		RECT client;
		QVERIFY(GetClientRect(handle, &client));
		QCOMPARE(int(client.right - client.left), qRound(100 * nativeDpr));
		QCOMPARE(int(client.bottom - client.top), qRound(32 * nativeDpr));
		qInfo() << "Native Windows awareness=PerMonitorV2; DPI=" << dpi(handle) << "DPR=" << nativeDpr
			<< "client=" << client.right - client.left << client.bottom - client.top;
#else
		QSKIP("Windows-only native awareness check");
#endif
	}
};
#include "DpiTestMain.h"
LMMS_DPI_TEST_MAIN(DpiNativeTest)
#include "DpiNativeTest.moc"
