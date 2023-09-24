/*
 * MixerDbMarkers.cpp - dB markers for mixer channels
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "MixerDbMarkers.h"
#include <QWidget>
#include <QPainter>

namespace lmms::gui 
{
    MixerDbMarkers::MixerDbMarkers(QWidget* parent) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }

    void MixerDbMarkers::setMinDb(int minDb) 
    {
        m_minDb = minDb;
    }

    void MixerDbMarkers::setMaxDb(int maxDb) 
    {
        m_maxDb = maxDb;
    }

    void MixerDbMarkers::paintEvent(QPaintEvent* event) 
    {
        const auto dbRange = m_maxDb - m_minDb;
        if (dbRange <= 0 || dbRange % 3 != 0) { return; }

        const auto numMarkers = dbRange / 3;
        const auto markerSpacing = rect().height() / numMarkers;
        auto markerDb = m_maxDb;

        QPainter painter{this};
        painter.setPen(Qt::white);
        
        auto font = painter.font();
        font.setPointSize(7);
        painter.setFont(font);

        for (int i = 0; i <= numMarkers; ++i)
        {
            const auto markerPoint = QPoint{rect().x(), rect().y() + i * markerSpacing};
            const auto markerWidth = rect().width() / (i % 2 == 0 ? 6 : 4);

            painter.drawLine(markerPoint, QPoint{markerPoint.x() + markerWidth, markerPoint.y()});
            if (i % 2 != 0) 
            {
                const auto markerRect = QRect{markerPoint.x() + 5, markerPoint.y() - markerSpacing, rect().width() - markerWidth, rect().height()};
                painter.drawText(markerRect, Qt::AlignRight, QString::number(std::abs(markerDb)));
            }

            markerDb -= 3;
        }        
    }

} // namespace lmms::gui
