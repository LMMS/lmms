/*
 * Scale.cpp - implementation of scale class
 *
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#include "Scale.h"

#include <cmath>
#include <QDomElement>

namespace lmms
{


Interval::Interval(float cents) :
	m_numerator(0),
	m_denominator(0),
	m_cents(cents)
{
	m_ratio = std::exp2(m_cents / 1200.f);
}

Interval::Interval(uint32_t numerator, uint32_t denominator) :
	m_numerator(numerator),
	m_denominator(denominator > 0 ? denominator : 1),
	m_cents(0)
{
	m_ratio = static_cast<float>(m_numerator) / m_denominator;
}


void Interval::saveSettings(QDomDocument &document, QDomElement &element)
{
	if (m_denominator > 0)
	{
		element.setAttribute("num", QString::number(m_numerator));
		element.setAttribute("den", QString::number(m_denominator));
	}
	else
	{
		element.setAttribute("cents", QString::number(m_cents));
	}
}


void Interval::loadSettings(const QDomElement &element)
{
	m_numerator = element.attribute("num", "0").toULong();
	m_denominator = element.attribute("den", "0").toULong();
	m_cents = element.attribute("cents", "0").toDouble();
	if (m_denominator) {m_ratio = static_cast<float>(m_numerator) / m_denominator;}
	else { m_ratio = std::exp2(m_cents / 1200.f); }
}


Scale::Scale() :
	m_description(tr("empty"))
{
	m_intervals.emplace_back(1, 1);
}

Scale::Scale(QString description, std::vector<Interval> intervals) :
	m_description(description),
	m_intervals(std::move(intervals))
{
}


QString Scale::getDescription() const
{
	return m_description;
}


void Scale::setDescription(QString description)
{
	m_description = description;
}


void Scale::saveSettings(QDomDocument &document, QDomElement &element)
{
	element.setAttribute("description", m_description);

	for (auto& interval : m_intervals)
	{
		interval.saveState(document, element);
	}

}


void Scale::loadSettings(const QDomElement &element)
{
	m_description = element.attribute("description");

	QDomNode node = element.firstChild();
	m_intervals.clear();

	while (!node.isNull())
	{
		Interval temp;
		temp.restoreState(node.toElement());
		m_intervals.push_back(temp);
		node = node.nextSibling();
	}
}


} // namespace lmms
