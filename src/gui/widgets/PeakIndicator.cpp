/*
 * PeakIndicator.cpp - Peak indicator widget
 *
 * Copyright (c) 2024- Michael Gregorius
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

#include "PeakIndicator.h"

#include "lmms_math.h"

#include <QMouseEvent>


namespace lmms::gui
{

PeakIndicator::PeakIndicator(QWidget* parent) :
	QLabel(parent),
	m_peak(0.f)
{
	setAlignment(Qt::AlignCenter);

	updatePeakDisplay();
}

void PeakIndicator::resetPeakToMinusInf()
{
	m_peak = 0;
	updatePeakDisplay();
}

void PeakIndicator::updatePeak(float peak)
{
	if (peak > m_peak)
	{
		m_peak = peak;
		updatePeakDisplay();
	}
}

void PeakIndicator::mousePressEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		resetPeakToMinusInf();
	}
}

void PeakIndicator::updatePeakDisplay()
{
	// Treat everything below -144 dbFS as -inf. Otherwise some residual signals show up
	// in the form of very small dbFS values, e.g. -857.1 dbFS.
	// TODO Make the threshold configurable in the settings?
	if (m_peak <= dbfsToAmp(-144.))
	{
		setText(tr("-inf"));
	}
	else
	{
		auto dbfs = ampToDbfs(m_peak);
		setText(QString::number(dbfs, 'f', 1));
	}
}

} // namespace lmms::gui
