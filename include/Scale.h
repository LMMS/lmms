/*
 * Scale.h - holds information about a scale and its intervals
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

#ifndef SCALE_H
#define SCALE_H

#include <cmath>
#include <vector>
#include <QObject>
#include <QString>

#include "SerializingObject.h"

class Interval : public SerializingObject
{
public:
	Interval() : m_numerator(1), m_denominator(1) {};
	Interval(double cents) : m_numerator(cents), m_denominator(0) {};
	Interval(unsigned long numerator, unsigned long denominator) :
		m_numerator(numerator),
		m_denominator(denominator > 0 ? denominator : 1) {};

	float getRatio() const
	{
		if (m_denominator) {return m_numerator / m_denominator;}
		else {return powf(2.f, m_numerator / 1200.f);}
	}

	QString getString() const
	{
		if (m_denominator) {return QString::number(m_numerator) + "/" + QString::number(m_denominator);}
		else {return QString().sprintf("%0.1f", m_numerator);}
	}

	void saveSettings(QDomDocument &doc, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;
	inline QString nodeName() const override {return "interval";}

private:
	// Scala specifies that numerators and denominators should go at least up to 2147483647;
	// that is 10 significant digits (→ needs double) and 32 bits signed (→ needs long).
	double m_numerator;				//!< numerator of the interval fraction
	unsigned long m_denominator;	//!< denominator of the interval fraction
};


class Scale : public QObject, public SerializingObject
{
	Q_OBJECT
public:
	Scale();
	Scale(QString description, std::vector<Interval> intervals);

	QString getDescription() const;
	void setDescription(QString description);

	const std::vector<Interval> &getIntervals() const {return m_intervals;}
	void setIntervals(std::vector<Interval> input) {m_intervals = input;}

	void saveSettings(QDomDocument &doc, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;
	inline QString nodeName() const override {return "scale";}

private:
	QString m_description;					//!< name or description of the scale
	std::vector<Interval> m_intervals;		//!< a series of ratios that define the scale

};

#endif
