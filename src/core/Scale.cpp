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

#include <QDomElement>


Scale::Scale() :
	m_description(tr("empty"))
{
	m_intervals.push_back(Interval(1, 1));
}

Scale::Scale(QString description, std::vector<Interval> intervals) :
	m_description(description),
	m_intervals(intervals)
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


void Interval::saveSettings(QDomDocument &document, QDomElement &element)
{
	element.setAttribute("num", QString::number(m_numerator));
	element.setAttribute("den", QString::number(m_denominator));
}


void Interval::loadSettings(const QDomElement &element)
{
	m_numerator = element.attribute("num").toDouble();
	m_denominator = element.attribute("den").toULong();
}


void Scale::saveSettings(QDomDocument &document, QDomElement &element)
{
	element.setAttribute("description", m_description);

    for (auto it = m_intervals.begin(); it != m_intervals.end(); it++)
    {
        (*it).saveState(document, element);
    }

}


void Scale::loadSettings(const QDomElement &element)
{
	m_description = element.attribute("description");

	QDomNode node = element.firstChild();
	m_intervals.clear();

	for (int i = 0; !node.isNull(); i++)
	{
		Interval temp;
		temp.restoreState(node.toElement());
		m_intervals.push_back(temp);
		node = node.nextSibling();
	}
}
