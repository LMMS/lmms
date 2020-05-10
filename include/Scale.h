/*
 * Scale.h - holds information about a scale and its intervals
 *
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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

#ifndef SCALE_H
#define SCALE_H

#include <cmath>
#include <vector>
#include <QObject>
#include <QString>


class Interval
{
public:
	Interval(double cents) : m_numerator(cents), m_denominator(0) {};
	Interval(unsigned long numerator, unsigned long denominator) :
		m_numerator(numerator),
		m_denominator(denominator > 0 ? denominator : 1) {};

	float getRatio() const
	{
		if (m_denominator) {return m_numerator / m_denominator;}
		else {return powf(2.f, m_numerator / 1200.f);}
	}

private:
	// Scala specifies that numerators and denominators should go at least up to 2147483647;
	// that is 10 significant digits (→ needs double) and 32 bits signed (→ needs long).
	double m_numerator;				//!< numerator of the interval fraction
	unsigned long m_denominator;	//!< denominator of the interval fraction
};


class Scale : public QObject
{
	Q_OBJECT
public:
	Scale();
	Scale(QString description, std::vector<Interval> intervals);

	QString getDescription() const;
	void setDescription(QString description);

	std::vector<Interval> & getIntervals() {return m_intervals;}

private:
	QString m_description;
	std::vector<Interval> m_intervals;

};

#endif
