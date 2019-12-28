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

#include <string>
#include <vector>


struct LMMS_EXPORT Interval
{
	Interval(double ratio) : m_numerator(ratio), m_denominator(1){};
	Interval(unsigned long numerator, unsigned long denominator) :
		m_numerator(numerator),
		m_denominator(denominator > 0 ? denominator : 1){};

	// Scala specifies that numerators and denominators should go at least up to 2147483647;
	// that is 10 significant digits (→ needs double) and 32 bits signed (→ needs long).
	double m_numerator;				//!< numerator of the interval fraction
	unsigned long m_denominator;	//!< denominator of the interval fraction
};


class LMMS_EXPORT Scale
{
public:
	Scale(std::string description, std::vector<Interval> intervals);

	QString getDescription();
	void setDescription(QString description);

private:


}

#endif
