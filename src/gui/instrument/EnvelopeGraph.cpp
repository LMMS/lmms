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

#include "EnvelopeGraph.h"

#include <QMouseEvent>
#include <QPainter>

#include "EnvelopeAndLfoParameters.h"
#include "lmms_math.h"

#include <cmath>


namespace lmms
{

namespace gui
{

static QColor interpolateInRgb(const QColor& a, const QColor& b, float t)
{
	qreal ar, ag, ab, aa;
	a.getRgbF(&ar, &ag, &ab, &aa);

	qreal br, bg, bb, ba;
	b.getRgbF(&br, &bg, &bb, &ba);

	const float interH = lerp(ar, br, t);
	const float interS = lerp(ag, bg, t);
	const float interV = lerp(ab, bb, t);
	const float interA = lerp(aa, ba, t);

	return QColor::fromRgbF(interH, interS, interV, interA);
}

EnvelopeGraph::EnvelopeGraph(QWidget* parent) :
	QWidget(parent),
	ModelView(nullptr, this)
{
	setMinimumSize(m_envGraph.size());
}

void EnvelopeGraph::modelChanged()
{
	m_params = castModel<EnvelopeAndLfoParameters>();
}

void EnvelopeGraph::mousePressEvent(QMouseEvent* me)
{
	if (me->button() == Qt::LeftButton) { toggleAmountModel(); }
}

void EnvelopeGraph::paintEvent(QPaintEvent*)
{
	QPainter p{this};
	p.setRenderHint(QPainter::Antialiasing);

	// Draw the graph background
	p.drawPixmap(rect(), m_envGraph);

	const auto* params = castModel<EnvelopeAndLfoParameters>();
	if (!params) { return; }

	// For the calculation of the percentages we will for now make use of the knowledge
	// that the range goes from 0 to a positive max value, i.e. that it is in [0, max].
	const float amount = params->getAmountModel().value();

	const float predelay = params->getPredelayModel().value();
	const float predelayPercentage = predelay / params->getPredelayModel().maxValue();

	const float attack = params->getAttackModel().value();
	const float attackPercentage = attack / params->getAttackModel().maxValue();

	const float hold = params->getHoldModel().value();
	const float holdPercentage = hold / params->getHoldModel().maxValue();

	const float decay = params->getDecayModel().value();
	const float decayPercentage = decay / params->getDecayModel().maxValue();

	const float sustain = params->getSustainModel().value();

	const float release = params->getReleaseModel().value();
	const float releasePercentage = release / params->getReleaseModel().maxValue();

	// The margin to the left and right so that we do not clip too much of the lines and markers
	const float margin = 2.0;
	// This is the maximum width that each of the five segments (DAHDR) can occupy so that
	// they always fit on the screen with maximum settings.
	const float maximumSegmentWidth = (width() - margin * 2) / 5.;

	// Compute the actual widths that the segments occupy and add them to the
	// previous x coordinates starting at the margin.
	const float predelayX = margin + predelayPercentage * maximumSegmentWidth;
	const float attackX = predelayX + attackPercentage * maximumSegmentWidth;
	const float holdX = attackX + holdPercentage * maximumSegmentWidth;
	const float decayX = holdX + (decayPercentage * (1 - sustain)) * maximumSegmentWidth;
	const float releaseX = decayX + releasePercentage * maximumSegmentWidth;

	// Now compute the "full" points including y coordinates
	const int yTop = 3;
	const qreal yBase = height() - 3;
	const int availableHeight = yBase - yTop;

	const QPointF predelayPoint{predelayX, yBase};
	const QPointF attackPoint{attackX, yTop};
	const QPointF holdPoint{holdX, yTop};
	const QPointF decayPoint{decayX, yTop + (1 - sustain) * availableHeight};
	const QPointF releasePoint{releaseX, yBase};


	// Now that we have all points we can draw the lines

	// Compute the color of the lines based on the amount of the envelope
	const float absAmount = std::abs(amount);
	const QColor noAmountColor{96, 91, 96};
	const QColor fullAmountColor{0, 255, 128};
	const QColor lineColor{interpolateInRgb(noAmountColor, fullAmountColor, absAmount)};

	// Determine the line width so that it scales with the widget
	// Use the minimum value of the current width and height to compute it.
	const qreal lineWidth = std::min(width(), height()) / 20.;
	const QPen linePen{lineColor, lineWidth};
	p.setPen(linePen);

	QPolygonF linePoly;
	linePoly << predelayPoint << attackPoint << holdPoint << decayPoint << releasePoint;
	p.drawPolyline(linePoly);


	// Now draw all marker on top of the lines
	const QColor markerFillColor{153, 175, 255};
	const QColor markerOutlineColor{0, 0, 0};

	QPen pen;
	pen.setWidthF(lineWidth * 0.75);
	pen.setBrush(markerOutlineColor);
	p.setPen(pen);
	p.setBrush(markerFillColor);

	// Compute the size of the circle we will draw based on the line width
	const qreal baseRectSize = lineWidth * 3;
	const QSizeF rectSize{baseRectSize, baseRectSize};

	auto drawMarker = [&](const QPointF & point)
	{
		// Create a rectangle that has the given point at its center
		QRectF bgRect{point + QPointF(-baseRectSize/2, -baseRectSize/2), rectSize};
		p.drawEllipse(bgRect);
	};

	drawMarker(predelayPoint);
	drawMarker(attackPoint);
	drawMarker(holdPoint);
	drawMarker(decayPoint);
	drawMarker(releasePoint);
}

void EnvelopeGraph::toggleAmountModel()
{
	auto* params = castModel<EnvelopeAndLfoParameters>();
	auto& amountModel = params->getAmountModel();

	amountModel.setValue(amountModel.value() < 1.0 ? 1.0 : 0.0);
}

} // namespace gui

} // namespace lmms
