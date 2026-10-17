/*
 * DpiContext.h - Per-window DPI awareness and dynamic monitor scaling helper
 *
 * Copyright (c) 2026 LMMS Developers
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

#ifndef LMMS_GUI_DPI_CONTEXT_H
#define LMMS_GUI_DPI_CONTEXT_H

#include <cmath>
#include <functional>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QList>
#include <QWidget>
#include <QWindow>
#include <QScreen>
#include <QSize>
#include <QSizeF>

#include "lmms_export.h"

class QScreen;
class QWidget;
class QWindow;

namespace lmms::gui
{

namespace Dpi
{

//! @brief Returns the effective device pixel ratio for a widget.
//! Uses native window handle if available, or widget screen, or primaryScreen fallback.
LMMS_EXPORT auto effectiveDevicePixelRatio(const QWidget* widget = nullptr) -> qreal;

//! @brief Returns the effective device pixel ratio for a QWindow.
LMMS_EXPORT auto effectiveDevicePixelRatio(const QWindow* window) -> qreal;

//! @brief Computes physical pixel raster size for a given logical size and DPR.
LMMS_EXPORT auto rasterSizeFor(const QSizeF& logicalSize, qreal dpr) -> QSize;
LMMS_EXPORT auto rasterSizeFor(const QSize& logicalSize, qreal dpr) -> QSize;

//! GUI-thread registration. Callback also runs after show/reparent and visual style changes.
LMMS_EXPORT void watch(QWidget* widget, std::function<void()> refresh);
LMMS_EXPORT void refresh(QWidget* subtree);

//! Pure logical-coordinate helper shared by window restoration and tests.
LMMS_EXPORT auto reachableGeometry(QRect geometry, const QList<QRect>& areas,
	QSize minimumSize = QSize(1, 1)) -> QRect;

} // namespace Dpi

class LMMS_EXPORT DpiContext : public QObject
{
	Q_OBJECT
public:
	explicit DpiContext(QWidget* targetWidget, QObject* parent = nullptr);
	~DpiContext() override;
	static auto forWidget(QWidget* widget) -> DpiContext*;

	auto targetWidget() const -> QWidget* { return m_widget; }
	auto devicePixelRatio() const -> qreal { return m_currentDpr; }

	//! @brief Check if DPI or screen changed and notify if different.
	void checkDpiChange();

signals:
	void dpiChanged(qreal newDpr, qreal oldDpr);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	void bindToWindow(QWindow* window);
	void unbindFromWindow();
	void onScreenChanged(QScreen* newScreen);
	void scheduleCheck();
	void scheduleGeometryCheck();
	void ensureReachableGeometry();

	QPointer<QWidget> m_widget;
	QPointer<QWindow> m_window;
	QPointer<QScreen> m_screen;
	qreal m_currentDpr = 1.0;
	bool m_checkPending = false;
	bool m_geometryCheckPending = true;
};

} // namespace lmms::gui

#endif // LMMS_GUI_DPI_CONTEXT_H
