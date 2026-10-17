/*
 * DpiContext.cpp - Per-window DPI awareness and dynamic monitor scaling helper
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

#include "DpiContext.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QTimer>
#include <QDebug>

#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace lmms::gui
{

namespace Dpi
{

auto effectiveDevicePixelRatio(const QWidget* widget) -> qreal
{
	if (widget)
	{
		const QWidget* top = widget->window() ? widget->window() : widget;
		if (const QWindow* wh = top->windowHandle())
		{
			return wh->devicePixelRatio();
		}
		return widget->devicePixelRatioF();
	}
	// Explicit provisional value for resources requested before a window exists.
	return 1.0;
}

auto effectiveDevicePixelRatio(const QWindow* window) -> qreal
{
	if (window)
	{
		return window->devicePixelRatio();
	}
	return effectiveDevicePixelRatio(static_cast<const QWidget*>(nullptr));
}

auto rasterSizeFor(const QSizeF& logicalSize, qreal dpr) -> QSize
{
	if (logicalSize.isEmpty() || !std::isfinite(logicalSize.width()) || !std::isfinite(logicalSize.height()))
	{
		return QSize(0, 0);
	}
	if (!std::isfinite(dpr) || dpr <= 0) { dpr = 1; }
	const qreal w = std::ceil(logicalSize.width() * dpr);
	const qreal h = std::ceil(logicalSize.height() * dpr);
	if (!std::isfinite(w) || !std::isfinite(h)
		|| w > std::numeric_limits<int>::max() || h > std::numeric_limits<int>::max())
	{
		return QSize(0, 0);
	}
	return QSize(static_cast<int>(w), static_cast<int>(h));
}

auto rasterSizeFor(const QSize& logicalSize, qreal dpr) -> QSize
{
	return rasterSizeFor(QSizeF(logicalSize), dpr);
}

} // namespace Dpi

namespace
{
class MetricsWatcher : public QObject
{
public:
	MetricsWatcher(QWidget* widget, std::function<void()> refresh) :
		QObject(widget), m_widget(widget), m_refresh(std::move(refresh))
	{
		widget->installEventFilter(this);
		schedule();
	}
public:
	void schedule()
	{
		if (m_pending) { return; }
		m_pending = true;
		QTimer::singleShot(0, this, [this] {
			m_pending = false;
			if (!m_widget) { return; }
			auto* context = DpiContext::forWidget(m_widget);
			if (context != m_context)
			{
				disconnect(m_connection);
				m_context = context;
				m_connection = connect(context, &DpiContext::dpiChanged, this,
					[this] { schedule(); });
			}
			m_refresh();
			m_widget->update();
		});
	}
	bool eventFilter(QObject* object, QEvent* event) override
	{
		switch (event->type())
		{
		case QEvent::Show:
		case QEvent::ParentChange:
		case QEvent::WinIdChange:
		case QEvent::FontChange:
		case QEvent::PaletteChange:
		case QEvent::StyleChange:
		case QEvent::EnabledChange:
			schedule();
			break;
		default: break;
		}
		return QObject::eventFilter(object, event);
	}
	QPointer<QWidget> m_widget;
	QPointer<DpiContext> m_context;
	QMetaObject::Connection m_connection;
	std::function<void()> m_refresh;
	bool m_pending = false;
};
}

void Dpi::refresh(QWidget* subtree)
{
	for (auto* child : subtree->findChildren<QObject*>())
	{
		if (auto* watcher = dynamic_cast<MetricsWatcher*>(child)) { watcher->schedule(); }
	}
}

void Dpi::watch(QWidget* widget, std::function<void()> refresh)
{
	new MetricsWatcher(widget, std::move(refresh));
}

auto Dpi::reachableGeometry(QRect geometry, const QList<QRect>& areas, QSize minimumSize) -> QRect
{
	if (areas.isEmpty()) { return geometry; }
	if (geometry.width() <= 0) { geometry.setWidth(300); }
	if (geometry.height() <= 0) { geometry.setHeight(200); }
	// Choose the greatest intersection; otherwise the closest available screen.
	QRect chosen;
	qint64 bestOverlap = -1;
	qreal bestDistance = std::numeric_limits<qreal>::max();
	for (const auto& area : areas)
	{
		if (area.isEmpty()) { continue; }
		const QRect intersection = area.intersected(geometry);
		const qint64 overlap = intersection.isEmpty() ? 0 : qint64(intersection.width()) * intersection.height();
		const QPointF delta = QPointF(area.center()) - QPointF(geometry.center());
		const qreal distance = delta.x() * delta.x() + delta.y() * delta.y();
		if (overlap > bestOverlap || (overlap == bestOverlap && distance < bestDistance))
		{
			chosen = area;
			bestOverlap = overlap;
			bestDistance = distance;
		}
	}
	if (chosen.isEmpty()) { return geometry; }
	geometry.setSize(geometry.size().boundedTo(chosen.size()).expandedTo(minimumSize));
	// An oversized minimum is allowed, but its top left (and title bar) remains reachable.
	geometry.moveLeft(std::clamp(geometry.x(), chosen.left(),
		chosen.left() + std::max(0, chosen.width() - geometry.width())));
	geometry.moveTop(std::clamp(geometry.y(), chosen.top(),
		chosen.top() + std::max(0, chosen.height() - geometry.height())));
	return geometry;
}

auto DpiContext::forWidget(QWidget* widget) -> DpiContext*
{
	QWidget* top = widget->window();
	for (auto* child : top->children())
	{
		if (auto* context = qobject_cast<DpiContext*>(child)) { return context; }
	}
	return new DpiContext(top, top);
}

DpiContext::DpiContext(QWidget* targetWidget, QObject* parent) :
	QObject(parent), m_widget(targetWidget ? targetWidget->window() : nullptr)
{
	if (m_widget)
	{
		m_widget->installEventFilter(this);
		m_currentDpr = Dpi::effectiveDevicePixelRatio(m_widget);
	}
	connect(qGuiApp, &QGuiApplication::screenAdded, this, [this] { scheduleGeometryCheck(); });
	connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] { scheduleGeometryCheck(); });
	scheduleCheck();
}

DpiContext::~DpiContext()
{
	unbindFromWindow();
}

void DpiContext::scheduleGeometryCheck()
{
	m_geometryCheckPending = true;
	scheduleCheck();
}

void DpiContext::scheduleCheck()
{
	if (m_checkPending) { return; }
	m_checkPending = true;
	QTimer::singleShot(0, this, [this] {
		m_checkPending = false;
		checkDpiChange();
	});
}

void DpiContext::checkDpiChange()
{
	if (!m_widget) { return; }
	if (!m_widget->isWindow())
	{
		// A detached editor has been reattached. Its consumers rebind to the owning
		// top-level context; the old context must not keep monitoring that same handle.
		unbindFromWindow();
		m_widget->removeEventFilter(this);
		deleteLater();
		return;
	}
	bindToWindow(m_widget->windowHandle());
	if (m_geometryCheckPending && m_widget->isVisible() && !m_widget->isMinimized())
	{
		m_geometryCheckPending = false;
		ensureReachableGeometry();
	}

	const qreal newDpr = Dpi::effectiveDevicePixelRatio(m_widget);
	if (!qFuzzyCompare(newDpr, m_currentDpr))
	{
		const qreal oldDpr = m_currentDpr;
		m_currentDpr = newDpr;
		qDebug() << "LMMS window DPR:" << m_widget->objectName() << oldDpr << "->" << newDpr;
		emit dpiChanged(newDpr, oldDpr);
		m_widget->update();
	}
}

void DpiContext::ensureReachableGeometry()
{
	const auto type = m_widget->windowType();
	if (type != Qt::Popup && type != Qt::ToolTip && !m_widget->isMaximized()
		&& !m_widget->isFullScreen() && !m_widget->isMinimized())
	{
		if (QGuiApplication::platformName() == "wayland")
		{
			// Global positioning is controlled by the compositor.
			if (m_screen)
			{
				m_widget->resize(m_widget->size().boundedTo(m_screen->availableGeometry().size())
					.expandedTo(m_widget->minimumSize()));
			}
		}
		else
		{
			QList<QRect> areas;
			for (const auto* screen : QGuiApplication::screens()) { areas.append(screen->availableGeometry()); }
			const QMargins margins = m_window ? m_window->frameMargins() : QMargins();
			const QSize frameSize(margins.left() + margins.right(), margins.top() + margins.bottom());
			const QRect reachable = Dpi::reachableGeometry(m_widget->frameGeometry(), areas,
				m_widget->minimumSize() + frameSize).marginsRemoved(margins);
			if (reachable != m_widget->geometry()) { m_widget->setGeometry(reachable); }
		}
	}
}

bool DpiContext::eventFilter(QObject* watched, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::WindowStateChange:
		scheduleGeometryCheck();
		break;
	case QEvent::Show:
	case QEvent::Resize:
	case QEvent::ParentChange:
	case QEvent::WinIdChange:
	case QEvent::ScreenChangeInternal:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	case QEvent::DevicePixelRatioChange:
#endif
		scheduleCheck();
		break;
	default: break;
	}
	return QObject::eventFilter(watched, event);
}

void DpiContext::bindToWindow(QWindow* window)
{
	if (m_window == window) { return; }
	unbindFromWindow();
	m_window = window;
	if (m_window)
	{
		m_window->installEventFilter(this);
		connect(m_window, &QWindow::screenChanged, this, [this](QScreen* screen) {
			onScreenChanged(screen);
			scheduleCheck();
		});
		onScreenChanged(m_window->screen());
#ifdef Q_OS_WIN
		using GetContext = HANDLE (WINAPI*)(HWND);
		using GetAwareness = int (WINAPI*)(HANDLE);
		const HMODULE user32 = GetModuleHandleW(L"user32.dll");
		const auto context = reinterpret_cast<GetContext>(GetProcAddress(user32, "GetWindowDpiAwarenessContext"));
		const auto awareness = reinterpret_cast<GetAwareness>(GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext"));
		if (QGuiApplication::platformName() == "windows" && context && awareness)
		{
			qDebug() << "LMMS native window DPI awareness (0=unaware, 1=system, 2=per-monitor):"
				<< awareness(context(reinterpret_cast<HWND>(m_window->winId())));
		}
#endif
	}
}

void DpiContext::unbindFromWindow()
{
	if (m_screen) { disconnect(m_screen, nullptr, this, nullptr); }
	m_screen = nullptr;
	if (m_window)
	{
		m_window->removeEventFilter(this);
		disconnect(m_window, nullptr, this, nullptr);
	}
	m_window = nullptr;
}

void DpiContext::onScreenChanged(QScreen* screen)
{
	if (m_screen == screen) { return; }
	if (m_screen) { disconnect(m_screen, nullptr, this, nullptr); }
	m_screen = screen;
	if (m_screen)
	{
		connect(m_screen, &QScreen::logicalDotsPerInchChanged, this, [this] { scheduleCheck(); });
		connect(m_screen, &QScreen::physicalDotsPerInchChanged, this, [this] { scheduleCheck(); });
		connect(m_screen, &QScreen::geometryChanged, this, [this] { scheduleGeometryCheck(); });
		connect(m_screen, &QScreen::availableGeometryChanged, this, [this] { scheduleGeometryCheck(); });
	}
}

} // namespace lmms::gui
