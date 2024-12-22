/*
 * LfoGraph.h - Displays LFO graphs
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2024-     Michael Gregorius
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

#ifndef LMMS_GUI_LFO_GRAPH_H
#define LMMS_GUI_LFO_GRAPH_H

#include <QWidget>

#include "ModelView.h"
#include "embed.h"

namespace lmms
{

class EnvelopeAndLfoParameters;

namespace gui
{

class LfoGraph : public QWidget, public ModelView
{
	Q_OBJECT
	Q_PROPERTY(QColor noAmountColor MEMBER m_noAmountColor)
	Q_PROPERTY(QColor fullAmountColor MEMBER m_fullAmountColor)

public:
	LfoGraph(QWidget* parent);

protected:
	void mousePressEvent(QMouseEvent* me) override;
	void paintEvent(QPaintEvent* pe) override;

private:
	void drawInfoText(const EnvelopeAndLfoParameters&);
	void toggleAmountModel();

private:
	QPixmap m_lfoGraph = embed::getIconPixmap("lfo_graph");

	float m_randomGraph {0.};
	QColor m_noAmountColor;
	QColor m_fullAmountColor;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_LFO_GRAPH_H
