/*
 * Timeline.cpp
 *
 * Copyright (c) 2023 Dominic Clark
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
 */

#include "Timeline.h"

#include <algorithm>
#include <tuple>

#include <QDomElement>

namespace lmms {

void Timeline::setLoopBegin(TimePos begin)
{
	std::tie(m_loopBegin, m_loopEnd) = std::minmax(begin, TimePos{m_loopEnd});
}

void Timeline::setLoopEnd(TimePos end)
{
	std::tie(m_loopBegin, m_loopEnd) = std::minmax(TimePos{m_loopBegin}, end);
}

void Timeline::setLoopPoints(TimePos begin, TimePos end)
{
	std::tie(m_loopBegin, m_loopEnd) = std::minmax(begin, end);
}

void Timeline::setLoopEnabled(bool enabled)
{
	if (enabled != m_loopEnabled) {
		m_loopEnabled = enabled;
		emit loopEnabledChanged(m_loopEnabled);
	}
}

void Timeline::setStopBehaviour(StopBehaviour behaviour)
{
	if (behaviour != m_stopBehaviour) {
		m_stopBehaviour = behaviour;
		emit stopBehaviourChanged(m_stopBehaviour);
	}
}

void Timeline::saveSettings(QDomDocument& doc, QDomElement& element)
{
	element.setAttribute("lp0pos", static_cast<int>(loopBegin()));
	element.setAttribute("lp1pos", static_cast<int>(loopEnd()));
	element.setAttribute("lpstate", static_cast<int>(loopEnabled()));
	element.setAttribute("stopbehaviour", static_cast<int>(stopBehaviour()));
}

void Timeline::loadSettings(const QDomElement& element)
{
	setLoopPoints(
		static_cast<TimePos>(element.attribute("lp0pos").toInt()),
		static_cast<TimePos>(element.attribute("lp1pos").toInt())
	);
	setLoopEnabled(static_cast<bool>(element.attribute("lpstate").toInt()));
	setStopBehaviour(static_cast<StopBehaviour>(element.attribute("stopbehaviour", "1").toInt()));
}

} // namespace lmms
