/*
 * LmmsStyle.cpp - the graphical style used by LMMS to create a consistent
 *				  interface
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <array>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep
#include <QStyleFactory>
#include <QStyleOption>

#include "embed.h"
#include "LmmsStyle.h"
#include "TextFloat.h"


namespace lmms::gui
{


QLinearGradient getGradient(const QColor & _col, const QRectF & _rect)
{
    QLinearGradient g(_rect.topLeft(), _rect.bottomLeft());

    // Forzar blanco puro en todo el gradiente
    QColor c = QColor(255, 255, 255);
    g.setColorAt(0.0, c);
    g.setColorAt(1.0, c);

    return g;
}

QLinearGradient darken(const QLinearGradient & _gradient)
{
    // Aunque se llame "darken", lo dejamos en blanco igualmente
    QLinearGradient g(_gradient.start(), _gradient.finalStop());
    QColor c = QColor(255, 255, 255);
    g.setColorAt(0.0, c);
    g.setColorAt(1.0, c);
    return g;
}

void drawPath(QPainter *p, const QPainterPath &path,
              const QColor &col, const QColor &borderCol,
              bool dark = false)
{
    const QRectF pathRect = path.boundingRect();

    const QLinearGradient baseGradient = getGradient(col, pathRect);
    const QLinearGradient darkGradient = darken(baseGradient);

    p->setOpacity(1.0);

    // fill siempre blanco
    p->fillPath(path, baseGradient);

    // borde también blanco
    p->strokePath(path, QPen(QColor(255, 255, 255), 2));
}

int LmmsStyle::pixelMetric(PixelMetric _metric, const QStyleOption * _option,
                           const QWidget * _widget) const
{
    switch (_metric)
    {
        case QStyle::PM_ButtonMargin:      return 3;
        case QStyle::PM_ButtonIconSize:    return 20;
        case QStyle::PM_ToolBarItemMargin: return 1;
        case QStyle::PM_ToolBarItemSpacing:return 2;
        case QStyle::PM_TitleBarHeight:    return 24;
        default: return QProxyStyle::pixelMetric(_metric, _option, _widget);
    }
}

QImage LmmsStyle::colorizeXpm(const char * const * xpm, const QBrush& fill) const
{
    QImage arrowXpm(xpm);
    QImage arrow(arrowXpm.size(), QImage::Format_ARGB32);
    QPainter arrowPainter(&arrow);

    // Rellenar siempre en blanco
    arrowPainter.fillRect(arrow.rect(), QColor(255, 255, 255));
    arrowPainter.end();
    arrow.setAlphaChannel(arrowXpm);

    return arrow;
}

void LmmsStyle::hoverColors(bool sunken, bool hover, bool active,
                            QColor& color, QColor& blend) const
{
    // Forzar todos los estados a blanco
    color = QColor(255, 255, 255);
    blend = QColor(255, 255, 255);
}

} // namespace lmms::gui
