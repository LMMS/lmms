/*
 * AutomationNode.h - Declaration of class AutomationNode, which contains
 *                       all information about an automation node
 *
 * Copyright (c) 2020 Ian Caio <iancaio_dev/at/hotmail.com>
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

#ifndef AUTOMATION_NODE_H
#define AUTOMATION_NODE_H


class AutomationPattern;


class AutomationNode
{
public:
	AutomationNode(); // Dummy constructor for the QMap
	AutomationNode(AutomationPattern* pat, float value, int key);
	AutomationNode(AutomationPattern* pat, float inValue, float outValue, int key);

	inline float getInValue()
	{
		return m_inValue;
	}
	inline const float getInValue() const
	{
		return m_inValue;
	}
	void setInValue(float value);

	inline float getOutValue()
	{
		return m_outValue;
	}
	inline const float getOutValue() const
	{
		return m_outValue;
	}
	void setOutValue(float value);

	/**
	 * @brief Gets the offset between inValue and outValue
	 * @return Float representing the offset between inValue and outValue
	 */
	inline float getValueOffset()
	{
		return m_outValue - m_inValue;
	}
	inline const float getValueOffset() const
	{
		return m_outValue - m_inValue;
	}

	/**
	 * @brief Gets the tangent of the left side of the node
	 * @return Float with the tangent from the inValue side
	 */
	inline float getInTangent()
	{
		return m_inTangent;
	}
	inline const float getInTangent() const
	{
		return m_inTangent;
	}

	/**
	 * @brief Sets the tangent of the left side of the node
	 * @param Float with the tangent for the inValue side
	 */
	inline void setInTangent(float tangent)
	{
		m_inTangent = tangent;
	}

	/**
	 * @brief Gets the tangent of the right side of the node
	 * @return Float with the tangent from the outValue side
	 */
	inline float getOutTangent()
	{
		return m_outTangent;
	}
	inline const float getOutTangent() const
	{
		return m_outTangent;
	}

	/**
	 * @brief Sets the tangent of the right side of the node
	 * @param Float with the tangent for the outValue side
	 */
	inline void setOutTangent(float tangent)
	{
		m_outTangent = tangent;
	}

private:
	// Pattern that this node belongs to
	AutomationPattern* m_pattern;

	// Time position of this node
	int m_key;

	// Values of this node
	float m_inValue;
	float m_outValue;

	// Slope at each point for calculating spline
	// We might have discrete jumps between curves, so we possibly have
	// two different tangents for each side of the curve. If inValue and
	// outValue are equal, inTangent and outTangent are equal too.
	float m_inTangent;
	float m_outTangent;
} ;


#endif
