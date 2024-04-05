/*
 * EnvelopeGraph.cpp - Displays envelope graphs
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMouseEvent>
#include <QPainter>

#include "EnvelopeGraph.h"

#include "EnvelopeAndLfoParameters.h"

namespace lmms
{

namespace gui
{

const int TIME_UNIT_WIDTH = 40;


EnvelopeGraph::EnvelopeGraph(QWidget* parent) :
	QWidget(parent),
	ModelView(nullptr, this),
	m_params(nullptr)
{
	setFixedSize(m_envGraph.size());
}

void EnvelopeGraph::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
}

void EnvelopeGraph::mousePressEvent(QMouseEvent* me)
{
	if(me->button() == Qt::LeftButton)
	{
		toggleAmountModel();
	}
}

void EnvelopeGraph::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// Draw the graph background
	p.drawPixmap(rect(), m_envGraph);

	const auto * params = castModel<EnvelopeAndLfoParameters>();
	if (!params)
	{
		return;
	}

	const float amount = params->getAmountModel().value();
	const float predelay = params->getPredelayModel().value();
	const float attack = params->getAttackModel().value();
	const float hold = params->getHoldModel().value();
	const float decay = params->getDecayModel().value();
	const float sustain = params->getSustainModel().value();
	const float release = params->getReleaseModel().value();

	const float gray_amount = 1.0f - fabsf(amount);

	p.setPen(QPen(QColor(static_cast<int>(96 * gray_amount),
				static_cast<int>(255 - 159 * gray_amount),
				static_cast<int>(128 - 32 * gray_amount)),
									2));

	const QColor end_points_color(0x99, 0xAF, 0xFF);
	const QColor end_points_bg_color(0, 0, 2);

	const int y_base = m_envGraph.height() - 3;
	const int avail_height = m_envGraph.height() - 6;
	
	int x1 = static_cast<int>(predelay * TIME_UNIT_WIDTH);
	int x2 = x1 + static_cast<int>(attack * TIME_UNIT_WIDTH);
	int x3 = x2 + static_cast<int>(hold * TIME_UNIT_WIDTH);
	int x4 = x3 + static_cast<int>((decay * (1 - sustain)) * TIME_UNIT_WIDTH);
	int x5 = x4 + static_cast<int>(release * TIME_UNIT_WIDTH);

	if (x5 > 174)
	{
		x1 = (x1 * 174) / x5;
		x2 = (x2 * 174) / x5;
		x3 = (x3 * 174) / x5;
		x4 = (x4 * 174) / x5;
		x5 = (x5 * 174) / x5;
	}
	x1 += 2;
	x2 += 2;
	x3 += 2;
	x4 += 2;
	x5 += 2;

	p.drawLine(x1, y_base, x2, y_base - avail_height);
	p.fillRect(x1 - 1, y_base - 2, 4, 4, end_points_bg_color);
	p.fillRect(x1, y_base - 1, 2, 2, end_points_color);

	p.drawLine(x2, y_base - avail_height, x3, y_base - avail_height);
	p.fillRect(x2 - 1, y_base - 2 - avail_height, 4, 4,
							end_points_bg_color);
	p.fillRect(x2, y_base - 1 - avail_height, 2, 2, end_points_color);

	const int sustainHeight = static_cast<int>(y_base - avail_height + (1 - sustain) * avail_height);

	p.drawLine(x3, y_base-avail_height, x4, sustainHeight);
	p.fillRect(x3 - 1, y_base - 2 - avail_height, 4, 4, end_points_bg_color);
	p.fillRect(x3, y_base - 1 - avail_height, 2, 2, end_points_color);
	
	p.drawLine(x4, sustainHeight, x5, y_base);
	p.fillRect(x4 - 1, sustainHeight - 2, 4, 4, end_points_bg_color);
	p.fillRect(x4, sustainHeight - 1, 2, 2, end_points_color);
	p.fillRect(x5 - 1, y_base - 2, 4, 4, end_points_bg_color);
	p.fillRect(x5, y_base - 1, 2, 2, end_points_color);
}

void EnvelopeGraph::toggleAmountModel()
{
	auto* params = castModel<EnvelopeAndLfoParameters>();
	auto& amountModel = params->getAmountModel();

	if (amountModel.value() < 1.0f )
	{
		amountModel.setValue( 1.0f );
	}
	else
	{
		amountModel.setValue( 0.0f );
	}
}

} // namespace gui

} // namespace lmms
