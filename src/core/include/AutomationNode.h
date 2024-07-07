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

#ifndef LMMS_AUTOMATION_NODE_H
#define LMMS_AUTOMATION_NODE_H

namespace lmms
{

class AutomationClip;


// Note: We use the default copy-assignment on the AutomationClip constructor. It's
// fine for now as we don't have dynamic allocated members, but if any are added we should
// have an user-defined one to perform a deep-copy.
class AutomationNode
{
public:
	AutomationNode(); // Dummy constructor for the QMap
	AutomationNode(AutomationClip* clip, float value, int pos);
	AutomationNode(AutomationClip* clip, float inValue, float outValue, int pos);

	AutomationNode& operator+=(float f)
	{
		m_inValue += f;
		m_outValue += f;
		return *this;
	}
	AutomationNode& operator-=(float f)
	{
		m_inValue -= f;
		m_outValue -= f;
		return *this;
	}
	AutomationNode& operator*=(float f)
	{
		m_inValue *= f;
		m_outValue *= f;
		return *this;
	}
	AutomationNode& operator/=(float f)
	{
		m_inValue /= f;
		m_outValue /= f;
		return *this;
	}

	inline const float getInValue() const
	{
		return m_inValue;
	}
	void setInValue(float value);

	inline const float getOutValue() const
	{
		return m_outValue;
	}
	void setOutValue(float value);
	void resetOutValue();

	/**
	 * @brief Gets the offset between inValue and outValue
	 * @return Float representing the offset between inValue and outValue
	 */
	inline const float getValueOffset() const
	{
		return m_outValue - m_inValue;
	}

	/**
	 * @brief Gets the tangent of the left side of the node
	 * @return Float with the tangent from the inValue side
	 */
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

	/**
	 * @brief Checks if the tangents from the node are locked
	 */
	inline const bool lockedTangents() const
	{
		return m_lockedTangents;
	}

	/**
	 * @brief Locks or Unlocks the tangents from this node
	 */
	inline void setLockedTangents(bool b)
	{
		m_lockedTangents = b;
	}

	/**
	 * @brief Sets the clip this node belongs to
	 * @param AutomationClip* clip that m_clip will be
	 * set to
	 */
	inline void setClip(AutomationClip* clip)
	{
		m_clip = clip;
	}

private:
	// Clip that this node belongs to
	AutomationClip* m_clip;

	// Time position of this node (matches the timeMap key)
	int m_pos;

	// Values of this node
	float m_inValue;
	float m_outValue;

	// Slope at each point for calculating spline
	// We might have discrete jumps between curves, so we possibly have
	// two different tangents for each side of the curve. If inValue and
	// outValue are equal, inTangent and outTangent are equal too.
	float m_inTangent;
	float m_outTangent;

	// If the tangents were edited manually, this will be true. That way
	// the tangents from this node will not be recalculated. It's set back
	// to false if the tangents are reset.
	bool m_lockedTangents;
};

} // namespace lmms

#endif // LMMS_AUTOMATION_NODE_H
