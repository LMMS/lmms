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


Scale::Scale() :
	m_description(tr("default (12-TET)"))
{
	m_intervals.push_back(Interval(1, 1));
	m_intervals.push_back(Interval(100.0));
	m_intervals.push_back(Interval(200.0));
	m_intervals.push_back(Interval(300.0));
	m_intervals.push_back(Interval(400.0));
	m_intervals.push_back(Interval(500.0));
	m_intervals.push_back(Interval(600.0));
	m_intervals.push_back(Interval(700.0));
	m_intervals.push_back(Interval(800.0));
	m_intervals.push_back(Interval(900.0));
	m_intervals.push_back(Interval(1000.0));
	m_intervals.push_back(Interval(1100.0));
	m_intervals.push_back(Interval(2, 1));
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

