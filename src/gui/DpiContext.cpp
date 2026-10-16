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

#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		if (const QScreen* s = top->screen())
		{
			return s->devicePixelRatio();
		}
#endif
		if (top->isVisible())
		{
			if (const QScreen* s = QGuiApplication::screenAt(top->geometry().center()))
			{
				return s->devicePixelRatio();
			}
		}
	}

	if (const auto* ps = QGuiApplication::primaryScreen())
	{
		return ps->devicePixelRatio();
	}
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
	if (logicalSize.isEmpty() || dpr <= 0.0)
	{
		return logicalSize.toSize();
	}

	const int w = static_cast<int>(std::ceil(logicalSize.width() * dpr));
	const int h = static_cast<int>(std::ceil(logicalSize.height() * dpr));
	return QSize(std::max(1, w), std::max(1, h));
}

auto rasterSizeFor(const QSize& logicalSize, qreal dpr) -> QSize
{
	return rasterSizeFor(QSizeF(logicalSize), dpr);
}

} // namespace Dpi

DpiContext::DpiContext(QWidget* targetWidget, QObject* parent) :
	QObject(parent),
	m_widget(targetWidget),
	m_window(nullptr),
	m_screen(nullptr),
	m_currentDpr(1.0)
{
	if (m_widget)
	{
		m_widget->installEventFilter(this);
		m_currentDpr = Dpi::effectiveDevicePixelRatio(m_widget);

		if (const QWidget* top = m_widget->window())
		{
			if (top->windowHandle())
			{
				bindToWindow(top->windowHandle());
			}
		}
	}
}

DpiContext::~DpiContext()
{
	unbindFromWindow();
	if (m_widget)
	{
		m_widget->removeEventFilter(this);
		m_widget = nullptr;
	}
}

void DpiContext::checkDpiChange()
{
	const qreal newDpr = Dpi::effectiveDevicePixelRatio(m_widget);
	if (std::abs(newDpr - m_currentDpr) > 0.001)
	{
		const qreal oldDpr = m_currentDpr;
		m_currentDpr = newDpr;
		emit dpiChanged(newDpr, oldDpr);
		if (m_widget)
		{
			m_widget->update();
		}
	}
}

bool DpiContext::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_widget)
	{
		if (event->type() == QEvent::Show || event->type() == QEvent::WinIdChange)
		{
			if (const QWidget* top = m_widget->window())
			{
				if (top->windowHandle() && top->windowHandle() != m_window)
				{
					bindToWindow(top->windowHandle());
				}
			}
			checkDpiChange();
		}
	}
	else if (watched == m_window)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
		if (event->type() == QEvent::DevicePixelRatioChange)
		{
			checkDpiChange();
		}
#endif
		if (event->type() == QEvent::ScreenChangeInternal)
		{
			checkDpiChange();
		}
	}

	return QObject::eventFilter(watched, event);
}

void DpiContext::bindToWindow(QWindow* window)
{
	if (m_window == window)
	{
		return;
	}

	unbindFromWindow();

	m_window = window;
	if (m_window)
	{
		m_window->installEventFilter(this);
		connect(m_window, &QWindow::screenChanged, this, &DpiContext::onScreenChanged);
		onScreenChanged(m_window->screen());
	}
}

void DpiContext::unbindFromWindow()
{
	if (m_screen)
	{
		disconnect(m_screen, nullptr, this, nullptr);
		m_screen = nullptr;
	}

	if (m_window)
	{
		m_window->removeEventFilter(this);
		disconnect(m_window, nullptr, this, nullptr);
		m_window = nullptr;
	}
}

void DpiContext::onScreenChanged(QScreen* newScreen)
{
	if (m_screen == newScreen)
	{
		return;
	}

	if (m_screen)
	{
		disconnect(m_screen, nullptr, this, nullptr);
	}

	m_screen = newScreen;
	if (m_screen)
	{
		connect(m_screen, &QScreen::logicalDotsPerInchChanged, this, [this]() {
			checkDpiChange();
		});
	}

	checkDpiChange();
}

} // namespace lmms::gui
