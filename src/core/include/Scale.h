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

#ifndef LMMS_SCALE_H
#define LMMS_SCALE_H

#include <cstdint>
#include <vector>
#include <QObject>
#include <QString>

#include "SerializingObject.h"

namespace lmms
{

class Interval : public SerializingObject
{
public:
	Interval() : m_numerator(1), m_denominator(1), m_cents(0), m_ratio(1) {};
	explicit Interval(float cents);
	Interval(uint32_t numerator, uint32_t denominator);

	float getRatio() const {return m_ratio;}

	QString getString() const
	{
		if (m_denominator) {return QString::number(m_numerator) + "/" + QString::number(m_denominator);}
		else {return QString("%1").arg(m_cents, 0, 'f', 4);}
	}

	void saveSettings(QDomDocument &doc, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;
	inline QString nodeName() const override {return "interval";}

private:
	// Scala specifies that numerators and denominators should go at least up to 2147483647 → use uint32_t.
	uint32_t m_numerator;   //!< numerator of the interval fraction
	uint32_t m_denominator; //!< denominator of the interval fraction
	float m_cents;          //!< interval defined in cents (used when denominator is set to zero)
	float m_ratio;          //!< precomputed output value for better performance
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
	void setIntervals(std::vector<Interval> input) {m_intervals = std::move(input);}

	void saveSettings(QDomDocument &doc, QDomElement &element) override;
	void loadSettings(const QDomElement &element) override;
	inline QString nodeName() const override {return "scale";}

private:
	QString m_description;                  //!< name or description of the scale
	std::vector<Interval> m_intervals;      //!< a series of ratios that define the scale

};


} // namespace lmms

#endif // LMMS_SCALE_H
